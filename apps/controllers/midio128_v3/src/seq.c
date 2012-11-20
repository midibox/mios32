// $Id$
/*
 * Sequencer Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "tasks.h"
#include <seq_bpm.h>
#include <seq_midi_out.h>
#include <midi_port.h>
#include <file.h>
#include <osc_client.h>

#include <mid_parser.h>

#include "seq.h"
#include "mid_file.h"
#include "midio_file.h"

#include "midio_dout.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// how much time has to be bridged between prefetch cycles (time in mS)
#define PREFETCH_TIME_MS 50 // mS


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_PlayOffEvents(void);
static s32 SEQ_SongPos(u16 new_song_pos);
static s32 SEQ_Tick(u32 bpm_tick);

static s32 Hook_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package);

static s32 SEQ_PlayNextFile(s8 next);

static s32 SEQ_SetRecordMode(u8 enable);

static s32 SEQ_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 SEQ_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// for FFWD function
static u8 ffwd_silent_mode;

// next tick at which the prefetch should take place
static u32 next_prefetch;

// already prefetched ticks
static u32 prefetch_offset;

// request to play the next file
static s8 next_file_req;

// output port flags
static u16 enabled_ports;

// the MIDI play mode
static u8 midi_play_mode;

// pause mode
static u8 seq_pause;

// for recording
static u8 seq_record;
static char seq_record_filename[13];
static u32 seq_record_last_tick;
static mios32_midi_port_t seq_record_last_port;
static mios32_midi_package_t seq_record_last_event;
static u32 seq_record_trk_header_filepos;
static u32 seq_record_trk_size;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Init(u32 mode)
{
  // play mode
  midi_play_mode = SEQ_MIDI_PLAY_MODE_ALL;

  // play over USB0 and UART0/1
  enabled_ports = 0x01 | (0x03 << 4);
  
  // init MIDI file handler
  MID_FILE_Init(0);

  // init MIDI parser module
  MID_PARSER_Init(0);

  // install callback functions
  MID_PARSER_InstallFileCallbacks(&MID_FILE_read, &MID_FILE_eof, &MID_FILE_seek);
  MID_PARSER_InstallEventCallbacks(&SEQ_PlayEvent, &SEQ_PlayMeta);

  // reset sequencer
  SEQ_Reset(0);

  // start with pause after power-on
  seq_pause = 1;

  // recording disabled
  seq_record = 0;

  // init BPM generator
  SEQ_BPM_Init(0);

  // scheduler should send packages to private hook
  SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(Hook_MIDI_SendPackage);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// set/get MIDI play mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MidiPlayModeGet(void)
{
  return midi_play_mode;
}

s32 SEQ_MidiPlayModeSet(u8 mode)
{
  if( mode >= SEQ_MIDI_PLAY_MODE_NUM )
    return -1;

  midi_play_mode = mode;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Handler(void)
{
  // a lower priority task requested to play the next file
  if( next_file_req != 0 ) {
    SEQ_PlayNextFile(next_file_req & (s8)~0x40);
    next_file_req = 0;
  };


  // handle BPM requests
  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    // note: don't remove any request check - clocks won't be propagated
    // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      SEQ_PlayOffEvents();
      SEQ_SetRecordMode(0);
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release pause mode
      seq_pause = 0;
    }

    if( SEQ_BPM_ChkReqStart() ) {
      SEQ_Reset(1);
      SEQ_SongPos(0);
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      SEQ_SongPos(new_song_pos);
    }

    if( !SEQ_RecordingEnabled() ) {
      u32 bpm_tick;
      if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
	again = 1; // check all requests again after execution of this part
	SEQ_Tick(bpm_tick);
      }
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // send Note Off to all channels
  // TODO: howto handle different ports?
  // TODO: should we also send Note Off events? Or should we trace Note On events and send Off if required?
  int chn;
  mios32_midi_package_t midi_package;
  midi_package.type = CC;
  midi_package.event = CC;
  midi_package.evnt2 = 0;
  for(chn=0; chn<16; ++chn) {
    midi_package.chn = chn;
    midi_package.evnt1 = 123; // All Notes Off
    Hook_MIDI_SendPackage(DEFAULT, midi_package);
    midi_package.evnt1 = 121; // Controller Reset
    Hook_MIDI_SendPackage(DEFAULT, midi_package);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Reset(u8 play_off_events)
{
  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  if( play_off_events )
    SEQ_PlayOffEvents();

  // release pause and FFWD mode
  seq_pause = 0;
  ffwd_silent_mode = 0;
  next_prefetch = 0;
  prefetch_offset = 0;

  // restart song
  MID_PARSER_RestartSong();

  // set initial BPM (according to MIDI file spec)
  SEQ_BPM_PPQN_Set(384); // not specified
  SEQ_BPM_Set(120.0);

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets new song position (new_song_pos resolution: 16th notes)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_SongPos(u16 new_song_pos)
{
  if( SEQ_RecordingEnabled() )
    return 0; // nothing to do

  u16 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

  portENTER_CRITICAL();

  // set new tick value
  SEQ_BPM_TickSet(new_tick);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ] Setting new song position %u (-> %u ticks)\n", new_song_pos, new_tick);
#endif

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_PlayOffEvents();

  // restart song
  MID_PARSER_RestartSong();

  // release pause
  seq_pause = 0;

  if( new_song_pos > 1 ) {
    // (silently) fast forward to requested position
    ffwd_silent_mode = 1;
    MID_PARSER_FetchEvents(0, new_tick-1);
    ffwd_silent_mode = 0;
  }

  // when do we expect the next prefetch:
  next_prefetch = new_tick;
  prefetch_offset = new_tick;

  portEXIT_CRITICAL();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays the given .mid file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PlayFile(char *midifile)
{
  SEQ_BPM_Stop();                  // stop BPM generator

  // play off events before loading new file
  SEQ_PlayOffEvents();

  // reset BPM tick (to ensure that next file will start at 0 if we are currently in pause mode)
  SEQ_BPM_TickSet(0);
  next_prefetch = prefetch_offset = 0;

  if( MID_FILE_open(midifile) ) { // try to open next file
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] file %s cannot be opened (wrong directory?)\n", midifile);
#endif
    return -1; // file cannot be opened
  }
  if( MID_PARSER_Read() < 0 ) { // read file, stop on failure
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] file %s is invalid!\n", midifile);
#endif
    return -2; // file is invalid
  } 

  // restart BPM generator if not in pause mode
  if( !seq_pause )
    SEQ_BPM_Start();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays the first .mid file if next == 0, the next file if next > 0, the
// 0: plays the first .mid file
// 1: plays the next .mid file
// -1: plays the previous .mid file
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayNextFile(s8 next)
{
  char next_file[13];
  next_file[0] = 0;

  if( next == 0 && MID_FILE_UI_NameGet()[0] != 0 ) {
    memcpy(next_file, MID_FILE_UI_NameGet(), 13);
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] play current file '%s'\n", next_file);
#endif
  } else if( next < 0 &&
      (MID_FILE_FindPrev(MID_FILE_UI_NameGet(), next_file) == 1 ||
       MID_FILE_FindNext(NULL, next_file) == 1) ) { // if previous file not found, try first file
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] previous file found '%s'\n", next_file);
#endif
  } else if( MID_FILE_FindNext(next ? MID_FILE_UI_NameGet() : NULL, next_file) == 1 ||
      MID_FILE_FindNext(NULL, next_file) == 1 ) { // if next file not found, try first file
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] next file found '%s'\n", next_file);
#endif
  }

  if( next_file[0] == 0 ) {
    if( next < 0 )
      return 0; // ignore silently

    SEQ_BPM_Stop();           // stop BPM generator

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] no file found\n");
#endif
    return -1; // file not found
  } else {
    SEQ_PlayFile(next_file);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Allows to request to play the next file from a lower priority task
// 0: request first
// 1: request next
// -1: request previous
//
// if force is set, the next/previous song will be played regardless of current MIDI play mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PlayFileReq(s8 next, u8 force)
{
  if( force || !next || midi_play_mode == SEQ_MIDI_PLAY_MODE_ALL ) {
    // stop generator
    SEQ_BPM_Stop();

    // request next file
    next_file_req = next | 0x40; // ensure that next_file is always != 0
  } else {
    // play current MIDI file again
    SEQ_Reset(1);
    SEQ_SongPos(0);
  }



  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help functions
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MidiRecWriteWord(u32 word, u8 len)
{
  int i;
  s32 status = 0;

  // ensure big endian coding, therefore byte writes
  for(i=0; i<len; ++i)
    status |= FILE_WriteByte((u8)(word >> (8*(len-1-i))));

  return (status < 0) ? status : len;
}


static s32 SEQ_MidiRecWriteVarLen(u32 value)
{
  // based on code example from MIDI file spec
  s32 status = 0;
  u32 buffer;

  buffer = value & 0x7f;
  while( (value >>= 7) > 0 ) {
    buffer <<= 8;
    buffer |= 0x80 | (value & 0x7f);
  }

  int num_bytes = 0;
  while( 1 ) {
    ++num_bytes;
    status |= FILE_WriteByte((u8)(buffer & 0xff));
    if( buffer & 0x80 )
      buffer >>= 8;
    else
      break;
  }

  return (status < 0) ? status : num_bytes;
}


/////////////////////////////////////////////////////////////////////////////
// called from when a MIDI event is received
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MidiRecReceive(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // ignore if recording mode not enabled
  if( !SEQ_RecordingEnabled() )
    return 0;

  // ignore realtime events (like MIDI clock)
  if( package.evnt0 >= 0xf8 )
    return 0;

  u32 seq_tick = SEQ_BPM_TickGet();

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ:%u] P:%s  M:%02X %02X %02X\n",
	    seq_tick,
	    MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(port)),
	    package.evnt0, package.evnt1, package.evnt2);
#endif

  u32 word = 0;
  u8 num_bytes = 0;
  switch( package.event ) {
    case NoteOff:
    case NoteOn:
    case PolyPressure:
    case CC:
    case PitchBend:
      word = ((u32)package.evnt0 << 16) | ((u32)package.evnt1 << 8) | ((u32)package.evnt2 << 0);
      num_bytes = 3;
      break;

    case ProgramChange:
    case Aftertouch:
      word = ((u32)package.evnt0 << 8) | ((u32)package.evnt1 << 0);
      num_bytes = 2;
      break;
  }

  if( num_bytes ) {
    u32 delta = seq_tick - seq_record_last_tick;

    MUTEX_SDCARD_TAKE;
    seq_record_trk_size += SEQ_MidiRecWriteVarLen(delta);
    seq_record_trk_size += SEQ_MidiRecWriteWord(word, num_bytes);
    MUTEX_SDCARD_GIVE;

    seq_record_last_tick = seq_tick;
    seq_record_last_port = port;
    seq_record_last_event = package;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// enters/exits record mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_SetRecordMode(u8 enable)
{
  s32 status = 0;

  if( (!seq_record && !enable) || (seq_record && enable) )
    return 0; // nothing to do

  seq_record = enable;

  MUTEX_SDCARD_TAKE;

  if( seq_record ) {
    // start recording
    seq_record_trk_size = 0;
    seq_record_last_tick = 0;
    seq_record_last_port = 0;
    seq_record_last_event.ALL = 0;

    // determine filename
    u32 rec_num = 0;
    while( 1 ) {
      ++rec_num;
      sprintf(seq_record_filename, "REC%d.MID", rec_num);
      if( !FILE_FileExists(seq_record_filename) )
	break;
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] Recording to '%s' started\n", seq_record_filename);
#endif

    if( (status=FILE_WriteOpen(seq_record_filename, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ] Failed to open/create %s, status: %d\n", seq_record_filename, status);
#endif
    } else {
      // write file header
      u32 header_size = 6;
      status |= FILE_WriteBuffer((u8*)"MThd", 4);
      status |= SEQ_MidiRecWriteWord(header_size, 4);
      status |= SEQ_MidiRecWriteWord(1, 2); // MIDI File Format
      status |= SEQ_MidiRecWriteWord(1, 2); // Number of Tracks
      status |= SEQ_MidiRecWriteWord(SEQ_BPM_PPQN_Get(), 2); // PPQN

      // write Track header
      seq_record_trk_header_filepos = FILE_WriteGetCurrentSize();
      status |= FILE_WriteBuffer((u8*)"MTrk", 4);
      status |= SEQ_MidiRecWriteWord(seq_record_trk_size, 4); // Placeholder
    }

    // disable recording on error
    if( status < 0 ) {
      seq_record = 0;
      MIDIO_FILE_StatusMsgSet("SDCard Err!");
    }
  } else {
    // stop recording
    status |= FILE_WriteClose();

    if( seq_record_trk_size ) {
      // switch back to first byte of track and write final track size
      if( (status=FILE_WriteOpen(seq_record_filename, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ] Failed to open %s again, status: %d\n", seq_record_filename, status);
#endif
	MIDIO_FILE_StatusMsgSet("SDCard Err!");
	status = -3; // file re-open error
      } else {
	status |= FILE_WriteSeek(seq_record_trk_header_filepos + 4);
	status |= SEQ_MidiRecWriteWord(seq_record_trk_size, 4);
	status |= FILE_WriteClose();

	// take over new filename, but pause sequencer
	seq_pause = 1;
	SEQ_PlayFile(seq_record_filename);
	MIDIO_FILE_StatusMsgSet(NULL);
      }
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] Recording to '%s' stopped. Track size: %d bytes\n", seq_record_filename, seq_record_trk_size);
#endif
  }

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if pause enabled
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PauseEnabled(void)
{
  return seq_pause;
}

/////////////////////////////////////////////////////////////////////////////
// returns 1 if record mode enabled
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RecordingEnabled(void)
{
  return seq_record;
}


/////////////////////////////////////////////////////////////////////////////
// returns the record filename (up to 12 chars)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_RecordingFilename(void)
{
  return seq_record_filename;
}

/////////////////////////////////////////////////////////////////////////////
// returns timestamp of last recorded MIDI event
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_LastRecordedTick(void)
{
  return seq_record_last_tick;
}

/////////////////////////////////////////////////////////////////////////////
// returns the last recorded MIDI port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t SEQ_LastRecordedPort(void)
{
  return seq_record_last_port;
}

/////////////////////////////////////////////////////////////////////////////
// returns the last recorded MIDI event
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t SEQ_LastRecordedEvent(void)
{
  return seq_record_last_event;
}

/////////////////////////////////////////////////////////////////////////////
// performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_Tick(u32 bpm_tick)
{
  if( bpm_tick >= next_prefetch ) {
    // get number of prefetch ticks depending on current BPM
    u32 prefetch_ticks = SEQ_BPM_TicksFor_mS(PREFETCH_TIME_MS);

    if( bpm_tick >= prefetch_offset ) {
      // buffer underrun - fetch more!
      prefetch_ticks += (bpm_tick - prefetch_offset);
      next_prefetch = bpm_tick; // ASAP
    } else if( (prefetch_offset - bpm_tick) < prefetch_ticks ) {
      // close to a buffer underrun - fetch more!
      prefetch_ticks *= 2;
      next_prefetch = bpm_tick; // ASAP
    } else {
      next_prefetch += prefetch_ticks;
    }

#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ] Prefetch started at tick %u (prefetching %u..%u)\n", bpm_tick, prefetch_offset, prefetch_offset+prefetch_ticks-1);
#endif

    if( MID_PARSER_FetchEvents(prefetch_offset, prefetch_ticks) == 0 ) {

      if( midi_play_mode == SEQ_MIDI_PLAY_MODE_SINGLE ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - stopping sequencer!\n", bpm_tick);
#endif

	SEQ_BPM_Stop();
	SEQ_Reset(1);
	seq_pause = 1;
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - loading next file!\n", bpm_tick);
#endif

	SEQ_PlayFileReq(1, 0);
      }
    } else {
      prefetch_offset += prefetch_ticks;
    }

#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ] Prefetch finished at tick %u\n", SEQ_BPM_TickGet());
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called when a MIDI event should be played at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick)
{
  // ignore all events in silent mode (for SEQ_SongPos function)
  // we could implement a more intelligent parser, which stores the sent CC/program change, etc...
  // and sends the last received values before restarting the song...
  if( ffwd_silent_mode )
    return 0;

  seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnEvent;
  if( midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0) )
    event_type = SEQ_MIDI_OUT_OffEvent;

  // output events on DEFAULT port
  u32 status = 0;
  status |= SEQ_MIDI_OUT_Send(DEFAULT, midi_package, event_type, tick, 0);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// called when a Meta event should be played/processed at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick)
{
  switch( meta ) {
    case 0x00: // Sequence Number
      if( len == 2 ) {
	u32 seq_number = (*buffer++ << 8) | (*buffer);
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Sequence Number %u\n", track, tick, seq_number);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Sequence Number with %d bytes -- ERROR: expecting 2 bytes!\n", track, tick, len);
#endif
      }
      break;

    case 0x01: // Text Event
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Text: %s\n", track, tick, buffer);
#endif
      break;

    case 0x02: // Copyright Notice
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Copyright: %s\n", track, tick, buffer);
#endif
      break;

    case 0x03: // Sequence/Track Name
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Track Name: %s\n", track, tick, buffer);
#endif
      break;

    case 0x04: // Instrument Name
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Instr. Name: %s\n", track, tick, buffer);
#endif
      break;

    case 0x05: // Lyric
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Lyric: %s\n", track, tick, buffer);
#endif
      break;

    case 0x06: // Marker
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Marker: %s\n", track, tick, buffer);
#endif
      break;

    case 0x07: // Cue Point
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - Cue Point: %s\n", track, tick, buffer);
#endif
      break;

    case 0x20: // Channel Prefix
      if( len == 1 ) {
	u32 prefix = *buffer;
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Channel Prefix %u\n", track, tick, prefix);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Channel Prefix with %d bytes -- ERROR: expecting 1 byte!\n", track, tick, len);
#endif
      }
      break;

    case 0x2f: // End of Track
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ:%d:%u] Meta - End of Track\n", track, tick, meta);
#endif
      break;

    case 0x51: // Set Tempo
      if( len == 3 ) {
	u32 tempo_us = (*buffer++ << 16) | (*buffer++ << 8) | *buffer;
	float bpm = 60.0 * (1E6 / (float)tempo_us);
	SEQ_BPM_PPQN_Set(MIDI_PARSER_PPQN_Get());

	// set tempo immediately on first tick
	if( tick == 0 ) {
	  SEQ_BPM_Set(bpm);
	} else {
	  // put tempo change request into the queue
	  mios32_midi_package_t tempo_package; // or Softis?
	  tempo_package.ALL = (u32)bpm;
	  SEQ_MIDI_OUT_Send(DEFAULT, tempo_package, SEQ_MIDI_OUT_TempoEvent, tick, 0);
	}

#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Tempo to %u uS -> %u BPM\n", track, tick, tempo_us, (u32)bpm);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Tempo with %u bytes -- ERROR: expecting 3 bytes!\n", track, tick, len);
#endif
      }
      break;

    // other known events which are not handled here:
    // 0x54: SMPTE offset
    // 0x58: Time Signature
    // 0x59: Key Signature
    // 0x7f: Sequencer Specific Meta Event

#if DEBUG_VERBOSE_LEVEL >= 2
    default:
      DEBUG_MSG("[SEQ:%d:%u] Meta Event 0x%02x with length %u not processed\n", track, tick, meta, len);
#endif
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// this hook is called when the MIDI scheduler sends a package
/////////////////////////////////////////////////////////////////////////////
static s32 Hook_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // forward to MIDIO
  MIDIO_DOUT_MIDI_NotifyPackage(port, package);

  // forward to enabled MIDI ports
  int i;
  u16 mask = 1;
  for(i=0; i<16; ++i, mask <<= 1) {
    if( enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, package);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// To control the play/stop button function
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PlayStopButton(void)
{
  if( SEQ_BPM_IsRunning() ) {
    SEQ_BPM_Stop();          // stop sequencer
    seq_pause = 1;
    SEQ_SetRecordMode(0);
  } else {
    if( seq_pause ) {
      // continue sequencer
      seq_pause = 0;
      SEQ_BPM_Cont();
    } else {
      MUTEX_SDCARD_TAKE;

      // if in auto mode and BPM generator is clocked in slave mode:
      // change to master mode
      SEQ_BPM_CheckAutoMaster();

      // request to play currently selected file
      SEQ_PlayFileReq(0, 1);

      // reset sequencer
      SEQ_Reset(1);

      // start sequencer
      SEQ_BPM_Start();

      MUTEX_SDCARD_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// To control the rec/stop button function
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RecStopButton(void)
{
  if( SEQ_BPM_IsRunning() ) {
    SEQ_BPM_Stop();          // stop sequencer
    seq_pause = 1;
    SEQ_SetRecordMode(0);
  } else {
    seq_pause = 0;

    MUTEX_SDCARD_TAKE;

    // if in auto mode and BPM generator is clocked in slave mode:
    // change to master mode
    SEQ_BPM_CheckAutoMaster();

    // enter record mode
    SEQ_SetRecordMode(1);

    // reset sequencer
    SEQ_Reset(1);

    // start sequencer
    SEQ_BPM_Start();

    MUTEX_SDCARD_GIVE;
  }

  return 0; // no error
}

s32 SEQ_FFwdButton(void)
{
  u32 tick = SEQ_BPM_TickGet();
  u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
  u32 ticks_per_measure = ticks_per_step * 16;

  int measure = tick / ticks_per_measure;
  int song_pos = 16 * (measure + 1);
  if( song_pos > 65535 )
    song_pos = 65535;

  return SEQ_SongPos(song_pos);
}

s32 SEQ_FRewButton(void)
{
  u32 tick = SEQ_BPM_TickGet();
  u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
  u32 ticks_per_measure = ticks_per_step * 16;

  int measure = tick / ticks_per_measure;
  int song_pos = 16 * (measure - 1);
  if( song_pos < 0 )
    song_pos = 0;

  return SEQ_SongPos(song_pos);
}

