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
#include <midi_router.h>

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
static s32 SEQ_CheckSongFinished(u32 bpm_tick);

static s32 Hook_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package);

static s32 SEQ_PlayNextFile(s8 next);

static s32 SEQ_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 SEQ_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u16 seq_play_enabled_ports;
u16 seq_rec_enabled_ports;

u8 seq_play_enable_dout;
u8 seq_rec_enable_din;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// for FFWD function
static u8 ffwd_silent_mode;

// next tick at which the prefetch should take place
static u32 next_prefetch;

// end of file reached
static u32 end_of_file;

// already prefetched ticks
static u32 prefetch_offset;

// request to play the next file
static s8 next_file_req;

// the MIDI play mode
static u8 midi_play_mode;

// pause mode
static u8 seq_pause;

// lock BPM, so that it can't be changed from MIDI player
static u8 seq_clk_locked;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Init(u32 mode)
{
  // play mode
  midi_play_mode = SEQ_MIDI_PLAY_MODE_ALL;
  seq_clk_locked = 0;

  // play over USB0 and UART0/1
  seq_play_enabled_ports = 0x01 | (0x03 << 4);

  // record over USB0 and UART0/1
  seq_rec_enabled_ports = 0x01 | (0x03 << 4);

  // play/record over DOUT/DIN
  seq_play_enable_dout = 1;
  seq_rec_enable_din = 1;
  
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
  SEQ_SetPauseMode(1);

  // init BPM generator
  SEQ_BPM_Init(0);
  SEQ_BPM_Set(120.0);

  // scheduler should send packages to private hook
  SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(Hook_MIDI_SendPackage);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// set/get Clock mode
// adds a fourth mode which locks the BPM so that it can't be modified by the MIDI file
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_ClockModeGet(void)
{
  if( seq_clk_locked )
    return 3;

  return SEQ_BPM_ModeGet();
}

s32 SEQ_ClockModeSet(u8 mode)
{
  if( mode > 3 )
    return -1; // invalid mode

  if( mode == 3 ) {
    SEQ_BPM_ModeSet(SEQ_BPM_MODE_Master);
    seq_clk_locked = 1;
  } else {
    SEQ_BPM_ModeSet(mode);
    seq_clk_locked = 0;
  }

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
      MID_FILE_SetRecordMode(0);

      MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release pause mode
      SEQ_SetPauseMode(0);

      MIDI_ROUTER_SendMIDIClockEvent(0xfb, 0);
    }

    if( SEQ_BPM_ChkReqStart() ) {
      MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);
      SEQ_Reset(1);
      SEQ_SongPos(0);
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      SEQ_SongPos(new_song_pos);
    }

    if( !MID_FILE_RecordingEnabled() ) {
      u32 bpm_tick;
      if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {

	// check if song is finished
	if( SEQ_CheckSongFinished(bpm_tick) >= 1 ) {
	  bpm_tick = 0;
	}

	// set initial BPM according to MIDI spec
	if( bpm_tick == 0 && !seq_clk_locked )
	  SEQ_BPM_Set(120.0);

	if( bpm_tick == 0 ) // send start (again) to synchronize with new MIDI songs
	  MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);

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
  SEQ_SetPauseMode(0);
  ffwd_silent_mode = 0;
  next_prefetch = 0;
  end_of_file = 0;
  prefetch_offset = 0;

  // set initial BPM (according to MIDI file spec)
  SEQ_BPM_PPQN_Set(384); // not specified
  //SEQ_BPM_Set(120.0);
  // will be done at tick 0 to avoid overwrite in record mode!

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  // restart song
  MID_PARSER_RestartSong();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets new song position (new_song_pos resolution: 16th notes)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_SongPos(u16 new_song_pos)
{
  if( MID_FILE_RecordingEnabled() )
    return 0; // nothing to do

  u32 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

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
  u8 pause = SEQ_PauseEnabled();
  SEQ_SetPauseMode(0);

  if( new_song_pos > 1 ) {
    // (silently) fast forward to requested position
    ffwd_silent_mode = 1;
    MID_PARSER_FetchEvents(0, new_tick-1);
    ffwd_silent_mode = 0;
  }

  // when do we expect the next prefetch:
  end_of_file = 0;
  next_prefetch = new_tick;
  prefetch_offset = new_tick;

  // restore pause mode
  SEQ_SetPauseMode(pause);

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
  end_of_file = next_prefetch = prefetch_offset = 0;

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
  if( !SEQ_PauseEnabled() )
    SEQ_BPM_Start();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays the first .mid file if next == 0, the next file if next > 0, the
// 0: plays the current .mid file
// 1: plays the next .mid file
// -1: plays the previous .mid file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PlayNextFile(s8 next)
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
       MID_FILE_FindPrev(NULL, next_file) == 1) ) { // if previous file not found, try last file
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
// enables/disables pause
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SetPauseMode(u8 enable)
{
  seq_pause = enable;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// returns 1 if pause enabled
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PauseEnabled(void)
{
  return seq_pause;
}


/////////////////////////////////////////////////////////////////////////////
// performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_Tick(u32 bpm_tick)
{
  // send MIDI clock depending on ppqn
  if( (bpm_tick % (SEQ_BPM_PPQN_Get()/24)) == 0 )
    MIDI_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);

  if( !end_of_file && bpm_tick >= next_prefetch ) {
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
      end_of_file = 1;
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
// handles song restart
// returns 1 if song has been restarted, otherwise 0
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CheckSongFinished(u32 bpm_tick)
{
  // synchronized switch to next file
  if( end_of_file &&
      ((bpm_tick+1) % SEQ_BPM_PPQN_Get()) == 0 ) {

    if( midi_play_mode == SEQ_MIDI_PLAY_MODE_SINGLE ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ] End of song reached after %u ticks - stopping sequencer!\n", bpm_tick);
#endif

      SEQ_BPM_Stop();
      SEQ_Reset(1);
      SEQ_SetPauseMode(1);
    } else if( midi_play_mode == SEQ_MIDI_PLAY_MODE_SINGLE_LOOP ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ] End of song reached after %u ticks - restarting song!\n", bpm_tick);
#endif
      SEQ_Reset(1);
    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ] End of song reached after %u ticks - loading next file!\n", bpm_tick);
#endif

      SEQ_PlayFileReq(1, 0);
    }

    return 1;
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

  // In order to support an unlimited SysEx stream length, we pass them as single bytes directly w/o the sequencer!
  if( midi_package.type == 0xf ) {
    Hook_MIDI_SendPackage(DEFAULT, midi_package);
    return 0;
  }

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

	if( !seq_clk_locked ) {
	  // set tempo immediately on first tick
	  if( tick == 0 ) {
	    SEQ_BPM_Set(bpm);
	  } else {
	    // put tempo change request into the queue
	    mios32_midi_package_t tempo_package; // or Softis?
	    tempo_package.ALL = (u32)bpm;
	    SEQ_MIDI_OUT_Send(DEFAULT, tempo_package, SEQ_MIDI_OUT_TempoEvent, tick, 0);
	  }
	}

#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ:%d:%u] Meta - Tempo to %u uS -> %u BPM%s\n", track, tick, tempo_us, (u32)bpm,
		  seq_clk_locked ? " IGNORED (locked)" : "");
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
  // realtime events are already scheduled by MIDI_ROUTER_SendMIDIClockEvent()
  if( package.evnt0 >= 0xf8 ) {
    MIOS32_MIDI_SendPackage(port, package);
  } else {
    // forward to MIDIO
    if( seq_play_enable_dout )
      MIDIO_DOUT_MIDI_NotifyPackage(port, package);

    // forward to enabled MIDI ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( seq_play_enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	MIOS32_MIDI_SendPackage(port, package);
      }
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
    SEQ_SetPauseMode(1);
    MID_FILE_SetRecordMode(0);
  } else {
    if( SEQ_PauseEnabled() ) {
      // continue sequencer
      SEQ_SetPauseMode(0);
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
    SEQ_SetPauseMode(1);
    MID_FILE_SetRecordMode(0);
  } else {
    SEQ_SetPauseMode(0);

    // if in auto mode and BPM generator is clocked in slave mode:
    // change to master mode
    SEQ_BPM_CheckAutoMaster();

    // enter record mode
    if( MID_FILE_SetRecordMode(1) >= 0 ) {
      // reset sequencer
      SEQ_Reset(1);

      // start sequencer
      SEQ_BPM_Start();
    }
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

