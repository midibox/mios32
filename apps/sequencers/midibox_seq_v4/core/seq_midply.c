// $Id$
/*
 * MIDI file player
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
#include <mid_parser.h>
#include <dosfs.h>

#include "seq_midply.h"
#include "seq_core.h"
#include "seq_file.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// how much time has to be bridged between prefetch cycles (time in mS)
#define PREFETCH_TIME_MS 50 // mS



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static u32 SEQ_MIDPLY_read(void *buffer, u32 len);
static s32 SEQ_MIDPLY_eof(void);
static s32 SEQ_MIDPLY_seek(u32 pos);

static s32 SEQ_MIDPLY_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 SEQ_MIDPLY_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_midply_mode_t seq_midply_mode;
static u8 seq_midply_loop_mode;

static mios32_midi_port_t seq_midply_port;

static u32 midifile_pos;
static u32 midifile_len;

// for FFWD function
static u8 ffwd_silent_mode;

// next tick at which the prefetch should take place
static u32 next_prefetch;

// already prefetched ticks
static u32 prefetch_offset;

// loop range in number of bpm_ticks
static u8 loop_req;
static u32 loop_range;
static u32 loop_offset;

// filename
#define MIDIFILE_PATH_LEN_MAX 20
static u8 midifile_path[MIDIFILE_PATH_LEN_MAX];

// DOS FS variables
static FILEINFO midifile_fi;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_Init(u32 mode)
{
  // init default mode and port
  seq_midply_mode = SEQ_MIDPLY_MODE_Off;
  seq_midply_loop_mode = 1;
  seq_midply_port = DEFAULT;
  midifile_path[0] = 0;

  // init MIDI parser module
  MID_PARSER_Init(0);

  // install callback functions
  MID_PARSER_InstallFileCallbacks(&SEQ_MIDPLY_read, &SEQ_MIDPLY_eof, &SEQ_MIDPLY_seek);
  MID_PARSER_InstallEventCallbacks(&SEQ_MIDPLY_PlayEvent, &SEQ_MIDPLY_PlayMeta);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set MIDI play mode
/////////////////////////////////////////////////////////////////////////////
seq_midply_mode_t SEQ_MIDPLY_ModeGet(void)
{
  return seq_midply_mode;
}

s32 SEQ_MIDPLY_ModeSet(seq_midply_mode_t mode)
{
  if( mode >= SEQ_MIDPLY_MODE_Off && mode <= SEQ_MIDPLY_MODE_Parallel ) {
    MUTEX_MIDIOUT_TAKE;

    // before switching to new mode, finish current mode properly
    switch( seq_midply_mode ) {
      case SEQ_MIDPLY_MODE_Exclusive:
      case SEQ_MIDPLY_MODE_Parallel:
	if( mode == SEQ_MIDPLY_MODE_Off ) {
	  SEQ_MIDPLY_PlayOffEvents();
	}
    }

    seq_midply_mode = mode;
    MUTEX_MIDIOUT_GIVE;
  }  else {
    return -1; // invalid mode
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set MIDI play loop mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_LoopModeGet(void)
{
  return seq_midply_loop_mode;
}

s32 SEQ_MIDPLY_LoopModeSet(u8 loop_mode)
{
  seq_midply_loop_mode = loop_mode;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set MIDI port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t SEQ_MIDPLY_PortGet(void)
{
  return seq_midply_port;
}

s32 SEQ_MIDPLY_PortSet(mios32_midi_port_t port)
{
  seq_midply_port = port;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// play new MIDI file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_PlayFile(char *path)
{
  s32 status;

  // ensure exclusive access to parser (this routine could be interrupted by sequencer handler)
  MUTEX_MIDIOUT_TAKE;

  // play off events if required
  SEQ_MIDPLY_PlayOffEvents();

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_ReadOpen(&midifile_fi, path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDPLY_PlayFile] failed to open file, status: %d\n", status);
#endif
    midifile_path[0] = 0; // disable file
  } else {

    // got it
    midifile_pos = 0;
    midifile_len = midifile_fi.filelen;

    strncpy(midifile_path, path, MIDIFILE_PATH_LEN_MAX);
    midifile_path[MIDIFILE_PATH_LEN_MAX-1] = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDPLY_PlayFile] opened '%s' of length %u\n", path, midifile_len);
#endif

    // read midifile
    MID_PARSER_Read();

    // reset sequencer
    loop_range = 0;
    loop_offset = 0;
    SEQ_MIDPLY_Reset();
  }

  MUTEX_MIDIOUT_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// return path of played file
/////////////////////////////////////////////////////////////////////////////
char *SEQ_MIDPLY_PathGet(void)
{
  return midifile_path;
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_PlayOffEvents(void)
{
  // Player disabled?
  if( seq_midply_mode == SEQ_MIDPLY_MODE_Off )
    return 0;

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
    MIOS32_MIDI_SendPackage(seq_midply_port, midi_package);
    midi_package.evnt1 = 121; // Controller Reset
    MIOS32_MIDI_SendPackage(seq_midply_port, midi_package);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_Reset(void)
{
  // Player disabled?
  if( seq_midply_mode == SEQ_MIDPLY_MODE_Off || !midifile_path[0] )
    return 0;

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_MIDPLY_PlayOffEvents();

  // release pause and FFWD mode
  ui_seq_pause = 0;
  ffwd_silent_mode = 0;
  next_prefetch = 0;
  prefetch_offset = 0;
  loop_req = 0;
  loop_offset = 0;

  // restart song
  MID_PARSER_RestartSong();

  // set initial BPM (according to MIDI file spec)
#if 0
  // done in SEQ_CORE
  SEQ_BPM_PPQN_Set(384); // not specified
  SEQ_BPM_Set(120.0);

  // reset BPM tick
  SEQ_BPM_TickSet(0);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets new song position (new_song_pos resolution: 16th notes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_SongPos(u16 new_song_pos)
{
  // Player disabled?
  if( seq_midply_mode == SEQ_MIDPLY_MODE_Off || !midifile_path[0] )
    return 0;

  // calculate tick based on MIDI file ppqn
  u16 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);
  new_tick = (MIDI_PARSER_PPQN_Get() * new_tick) / 384;

  if( loop_range )
    new_song_pos %= loop_range;

#if 0
  // done by SEQ_CORE
  // set new tick value
  SEQ_BPM_TickSet(new_tick);
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_MIDPLY] Setting new song position %u (-> %u ticks)\n", new_song_pos, new_tick);
#endif

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  if( !loop_range )
    SEQ_MIDPLY_PlayOffEvents();

  // restart song
  MID_PARSER_RestartSong();

#if 0
  // controlled by SEQ_CORE
  // release pause
  ui_seq_pause = 0;
#endif

  if( new_song_pos > 1 ) {
    // (silently) fast forward to requested position
    ffwd_silent_mode = 1;
    MID_PARSER_FetchEvents(0, new_tick-1);
    ffwd_silent_mode = 0;
  }

  // when do we expect the next prefetch:
  next_prefetch = new_tick + loop_offset;
  prefetch_offset = new_tick + loop_offset;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_Tick(u32 bpm_tick)
{
  // Player disabled?
  if( seq_midply_mode == SEQ_MIDPLY_MODE_Off || !midifile_path[0] )
    return 0;

  // request UI update if in exclusive mode (normaly done by SEQ_CORE)
  if( seq_midply_mode == SEQ_MIDPLY_MODE_Exclusive && (bpm_tick % (384/4)) == 0 )
    seq_core_step_update_req = 1;

  // calculate tick based on MIDI file ppqn
  u32 ppqn = MIDI_PARSER_PPQN_Get();
  bpm_tick = (ppqn * bpm_tick) / 384;

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

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] Prefetch started at tick %u (prefetching %u..%u)\n", bpm_tick, prefetch_offset, prefetch_offset+prefetch_ticks-1);
#endif

    u32 looped_prefetch_offset = loop_range ? (prefetch_offset % loop_range) : prefetch_offset;
    if( MID_PARSER_FetchEvents(looped_prefetch_offset, prefetch_ticks) == 0 ) {
      if( !seq_midply_loop_mode ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - stopped playback.\n", bpm_tick);
#endif
	loop_range = 0;
	loop_offset = 0;
	loop_req = 0;
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - restart request!\n", bpm_tick);
#endif
	loop_req = 1;
      }
    } else {
      prefetch_offset += prefetch_ticks;
    }

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] Prefetch finished at tick %u\n", SEQ_BPM_TickGet());
#endif
  }

  if( loop_req && ((bpm_tick % (4*ppqn)) == 0) ) {
    loop_req = 0;
    if( !loop_range )
      loop_range = bpm_tick;
    loop_offset = bpm_tick;
    SEQ_MIDPLY_SongPos(0);
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] Song restarted at tick %u, loop range: %u\n", bpm_tick, loop_range);
#endif
  }



  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_MIDPLY_read(void *buffer, u32 len)
{
  s32 status;

  if( !midifile_path[0] )
    return SEQ_FILE_ERR_NO_FILE;

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_ReadBuffer(&midifile_fi, buffer, len);
  MUTEX_SDCARD_GIVE;

  return (status >= 0) ? len : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDPLY_eof(void)
{
  if( midifile_pos >= midifile_len )
    return 1; // end of file reached

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDPLY_seek(u32 pos)
{
  s32 status;

  if( !midifile_path[0] )
    return -1; // end of file reached

  midifile_pos = pos;

  MUTEX_SDCARD_TAKE;

  if( midifile_pos >= midifile_len )
    status = -1; // end of file reached
  else
    status = SEQ_FILE_Seek(&midifile_fi, pos);    

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// called when a MIDI event should be played at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDPLY_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  u32 old_tick = tick;
#endif
  // calculate tick based on 384 ppqn
  tick += loop_offset;
  tick = (384 * tick) / MIDI_PARSER_PPQN_Get();

#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("Play %u -> %u (current: %u)\n", old_tick, tick, SEQ_BPM_TickGet());
#endif

  // ignore all events in silent mode (for SEQ_MIDPLY_SongPos function)
  // we could implement a more intelligent parser, which stores the sent CC/program change, etc...
  // and sends the last received values before restarting the song...
  if( ffwd_silent_mode )
    return 0;
  
  seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnEvent;
  if( midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0) )
    event_type = SEQ_MIDI_OUT_OffEvent;

  // use tag of track #16 - unfortunately more than 16 tags are not supported
  midi_package.cable = 15; 

#if 0
  return SEQ_MIDI_OUT_Send(seq_midply_port, midi_package, event_type, tick, 0);
#else
  // adding +1 to the tick leads to better results when events are pre-generated like here
  // otherwise it can happen that the scheduler is called while the tick reached
  // the BPM_Tick, and that SEQ_CORE will be generated and sent 1 mS later
  return SEQ_MIDI_OUT_Send(seq_midply_port, midi_package, event_type, tick+1, 0);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// called when a Meta event should be played/processed at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDPLY_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick)
{
  // calculate tick based on 384 ppqn
  tick += loop_offset;
  tick = (384 * tick) / MIDI_PARSER_PPQN_Get();

  switch( meta ) {
    case 0x00: // Sequence Number
      if( len == 2 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	u32 seq_number = (*buffer++ << 8) | *buffer;
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Sequence Number %u\n", track, tick, seq_number);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Sequence Number with %d bytes -- ERROR: expecting 2 bytes!\n", track, tick, len);
#endif
      }
      break;

    case 0x01: // Text Event
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Text: %s\n", track, tick, buffer);
#endif
      break;

    case 0x02: // Copyright Notice
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Copyright: %s\n", track, tick, buffer);
#endif
      break;

    case 0x03: // Sequence/Track Name
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Track Name: %s\n", track, tick, buffer);
#endif
      break;

    case 0x04: // Instrument Name
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Instr. Name: %s\n", track, tick, buffer);
#endif
      break;

    case 0x05: // Lyric
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Lyric: %s\n", track, tick, buffer);
#endif
      break;

    case 0x06: // Marker
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Marker: %s\n", track, tick, buffer);
#endif
      break;

    case 0x07: // Cue Point
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Cue Point: %s\n", track, tick, buffer);
#endif
      break;

    case 0x20: // Channel Prefix
      if( len == 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	u32 prefix = *buffer;
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Channel Prefix %u\n", track, tick, prefix);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Channel Prefix with %d bytes -- ERROR: expecting 1 byte!\n", track, tick, len);
#endif
      }
      break;

    case 0x2f: // End of Track
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - End of Track\n", track, tick, meta);
#endif
      break;

    case 0x51: // Set Tempo
      if( len == 3 ) {
	u32 tempo_us = (*buffer++ << 16) | (*buffer++ << 8) | *buffer;
	float bpm = 60.0 * (1E6 / (float)tempo_us);
#if 0
	// tempo handled by SEQ_CORE !
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
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Tempo to %u uS -> %u BPM\n", track, tick, tempo_us, (u32)bpm);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta - Tempo with %u bytes -- ERROR: expecting 3 bytes!\n", track, tick, len);
#endif
      }
      break;

    // other known events which are not handled here:
    // 0x54: SMPTE offset
    // 0x58: Time Signature
    // 0x59: Key Signature
    // 0x7f: Sequencer Specific Meta Event

#if DEBUG_VERBOSE_LEVEL >= 1
    default:
      DEBUG_MSG("[SEQ_MIDPLY:%d:%u] Meta Event 0x%02x with length %u not processed\n", track, tick, meta, len);
#endif
  }

  return 0;
}

