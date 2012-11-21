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
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <mid_parser.h>

#include "seq.h"
#include "mid_file.h"


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

static s32 SEQ_PlayFile(u32 next);

static s32 SEQ_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 SEQ_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the pattern position
static u8 seq_step_pos;

// pause mode (will be controlled from user interface)
static u8 seq_pause = 0;

// for FFWD function
static u8 ffwd_silent_mode;

// next tick at which the prefetch should take place
static u32 next_prefetch;

// already prefetched ticks
static u32 prefetch_offset;

// request to play the next file
static u8 next_file_req;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Init(u32 mode)
{
  // init MIDI file handler
  MID_FILE_Init(0);

  // init MIDI parser module
  MID_PARSER_Init(0);

  // install callback functions
  MID_PARSER_InstallFileCallbacks(&MID_FILE_read, &MID_FILE_eof, &MID_FILE_seek);
  MID_PARSER_InstallEventCallbacks(&SEQ_PlayEvent, &SEQ_PlayMeta);

  // reset sequencer
  SEQ_Reset(0);

  // init BPM generator
  SEQ_BPM_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Handler(void)
{
  // a lower priority task requested to play the next file
  if( next_file_req ) {
    SEQ_PlayFile(next_file_req-1);
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

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      SEQ_Tick(bpm_tick);
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
    MIOS32_MIDI_SendPackage(DEFAULT, midi_package);
    midi_package.evnt1 = 121; // Controller Reset
    MIOS32_MIDI_SendPackage(DEFAULT, midi_package);
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
  u16 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays the first .mid file if next == 0, the next file if next != 0
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayFile(u32 next)
{
  // play off events before loading new file
  SEQ_PlayOffEvents();

  char next_file[13];
  if( MID_FILE_FindNext(next ? MID_FILE_UI_NameGet() : NULL, next_file) == 1 ||
      MID_FILE_FindNext(NULL, next_file) == 1 ) { // if next file not found, try first file
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] next file found '%s'\n", next_file);
#endif
    SEQ_BPM_Stop();                  // stop BPM generator
    if( MID_FILE_open(next_file) ) { // try to open next file
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ] file %s cannot be opened (wrong directory?)\n", next_file);
#endif
      return -1; // file cannot be opened
    }
    if( MID_PARSER_Read() < 0 ) { // read file, stop on failure
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ] file %s is invalid!\n", next_file);
#endif
      return -2; // file is invalid
    } 
    SEQ_BPM_Start();          // start BPM generator
  } else {
    SEQ_BPM_Stop();           // stop BPM generator

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] no file found\n");
#endif
    return -1; // file not found
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Allows to request to play the next file from a lower priority task
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PlayFileReq(u32 next)
{
  // stop generator
  SEQ_BPM_Stop();

  // request next file
  next_file_req = next ? 2 : 1;

  return 0; // no error
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
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ] End of song reached after %u ticks - loading next file!\n", bpm_tick);
#endif
      SEQ_PlayFileReq(1);
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
	u32 seq_number = (buffer[0] << 8) | buffer[1];
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
	u32 tempo_us = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
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
