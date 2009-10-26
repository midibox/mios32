// $Id$
/*
 * MIDI File Player
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
static u8 seq_midply_run_mode;
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

// for synched start
static u32 synched_start_tick;

// to track Note On events:
// each note of all 16 channels has a dedicated array of note flags (packed in 32bit format)
#define UNPLAYED_NOTE_OFF_CHN_NUM 16 // for all 16 MIDI channels assigned to seq_midply_port
static u32 unplayed_note_off[UNPLAYED_NOTE_OFF_CHN_NUM][128/32];

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
  seq_midply_mode = SEQ_MIDPLY_MODE_Parallel;
  seq_midply_run_mode = 0;
  seq_midply_loop_mode = 1;
  seq_midply_port = DEFAULT;
  midifile_path[0] = 0;

  // init MIDI parser module
  MID_PARSER_Init(0);

  // install callback functions
  MID_PARSER_InstallFileCallbacks(&SEQ_MIDPLY_read, &SEQ_MIDPLY_eof, &SEQ_MIDPLY_seek);
  MID_PARSER_InstallEventCallbacks(&SEQ_MIDPLY_PlayEvent, &SEQ_MIDPLY_PlayMeta);

  // no unplayed note off events
  int chn, i;
  for(chn=0; chn<UNPLAYED_NOTE_OFF_CHN_NUM; ++chn)
    for(i=0; i<(128/32); ++i)
      unplayed_note_off[chn][i] = 0;

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
  if( mode >= SEQ_MIDPLY_MODE_Exclusive && mode <= SEQ_MIDPLY_MODE_Parallel ) {
    seq_midply_mode = mode;
  }  else {
    return -1; // invalid mode
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set runmode (on=Play, off=Stop)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_RunModeGet(void)
{
  return seq_midply_run_mode;
}

s32 SEQ_MIDPLY_RunModeSet(u8 on, u8 synched_start)
{
  // exit if no song loaded
  if( !midifile_path[0] )
    return -1;

  MUTEX_MIDIOUT_TAKE;

  // by default no synched start
  synched_start_tick = 0;

  // flush events if run mode turned off or sequencer already running
  if( !on || seq_midply_run_mode )
    SEQ_MIDPLY_PlayOffEvents();

  seq_midply_run_mode = on;

  if( seq_midply_run_mode ) {
    // reset sequencer
    loop_range = 0;
    loop_offset = 0;
    SEQ_MIDPLY_Reset();

    if( synched_start ) {
      // calculate start tick based on steps per measure
      u32 tick = SEQ_BPM_TickGet();
      u32 ticks_per_step = 384 / 4;
      // u32 ticks_per_measure = ticks_per_step * ((int)seq_core_steps_per_measure+1);
      u32 ticks_per_measure = ticks_per_step * 16; // using seq_core_steps_per_measure could be too confusing if e.g. value is set to 256 steps!
      u32 measure = (tick / ticks_per_measure);

      synched_start_tick = (MIDI_PARSER_PPQN_Get() * (ticks_per_measure * (measure + 1))) / 384;
    }
  }

  MUTEX_MIDIOUT_GIVE;

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
// get Start Tick if synched start (returns 0 if no synched start requested)
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_MIDPLY_SynchTickGet(void)
{
  return synched_start_tick;
}


/////////////////////////////////////////////////////////////////////////////
// reads new MIDI file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_ReadFile(char *path)
{
  s32 status;

  // ensure exclusive access to parser (this routine could be interrupted by sequencer handler)
  MUTEX_MIDIOUT_TAKE;

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_ReadOpen(&midifile_fi, path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDPLY_ReadFile] failed to open file, status: %d\n", status);
#endif
    midifile_path[0] = 0; // disable file
  } else {

    // got it
    midifile_pos = 0;
    midifile_len = midifile_fi.filelen;

    strncpy(midifile_path, path, MIDIFILE_PATH_LEN_MAX);
    midifile_path[MIDIFILE_PATH_LEN_MAX-1] = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDPLY_ReadFile] opened '%s' of length %u\n", path, midifile_len);
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
// disables the current MIDI file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_DisableFile(void)
{
  SEQ_MIDPLY_RunModeSet(0, 0);
  midifile_path[0] = 0;

  return 0;
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
  int chn, i, bit;
  mios32_midi_package_t midi_package;

  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // send Note Off based on note tracker flags
  // no unplayed note off events
  for(chn=0; chn<UNPLAYED_NOTE_OFF_CHN_NUM; ++chn)
    for(i=0; i<(128/32); ++i)
      if( unplayed_note_off[chn][i] ) {
	for(bit=0; bit<32; ++bit)
	  if( unplayed_note_off[chn][i] & (1 << bit) ) {
	    midi_package.type = NoteOn;
	    midi_package.event = NoteOn;
	    midi_package.chn = chn;
	    midi_package.note = i*32 + bit;
	    midi_package.velocity = 0;
	    MIOS32_MIDI_SendPackage(seq_midply_port, midi_package);
	  }
	unplayed_note_off[chn][i] = 0;
      }

  // send Controller Reset to all channels
  // + send Note Off CC (if CC Damper Pedal has been sent!)
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
  // Player running?
  if( !seq_midply_run_mode || !midifile_path[0] )
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
  synched_start_tick = 0;

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
s32 SEQ_MIDPLY_SongPos(u16 new_song_pos, u8 from_midi)
{
  // Player running?
  if( !seq_midply_run_mode || !midifile_path[0] )
    return 0;

  // if position has been changed via MIDI command: reset loop and synched start offsets
  if( from_midi ) {
    loop_range = 0;
    loop_offset = 0;
    synched_start_tick = 0;
  }

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
    MID_PARSER_FetchEvents(0, new_tick - 1);
    ffwd_silent_mode = 0;
  }

  // when do we expect the next prefetch:
  next_prefetch = new_tick;
  prefetch_offset = new_tick;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDPLY_Tick(u32 bpm_tick)
{
  // Player running?
  if( !seq_midply_run_mode || !midifile_path[0] )
    return 0;

  // calculate tick based on MIDI file ppqn
  u32 ppqn = MIDI_PARSER_PPQN_Get();
  bpm_tick = (ppqn * bpm_tick) / 384;

  // synched start
  if( synched_start_tick ) {
    if( bpm_tick < synched_start_tick )
      return 0;
    loop_offset = synched_start_tick;
    next_prefetch = 0; // ASAP
    prefetch_offset = 0;
    synched_start_tick = 0;
  }

  u32 midi_file_tick = bpm_tick - loop_offset;
  if( loop_range )
    midi_file_tick %= loop_range;

  if( midi_file_tick >= next_prefetch ) {
    // get number of prefetch ticks depending on current BPM
    u32 prefetch_ticks = SEQ_BPM_TicksFor_mS(PREFETCH_TIME_MS);

    if( midi_file_tick >= prefetch_offset ) {
      // buffer underrun - fetch more!
      prefetch_ticks += (midi_file_tick - prefetch_offset);
      next_prefetch = midi_file_tick; // ASAP
    } else if( (prefetch_offset - midi_file_tick) < prefetch_ticks ) {
      // close to a buffer underrun - fetch more!
      prefetch_ticks *= 2;
      next_prefetch = midi_file_tick; // ASAP
    } else {
      next_prefetch += prefetch_ticks;
    }

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ] Prefetch started at tick %u (prefetching %u..%u)\n", midi_file_tick, prefetch_offset, prefetch_offset+prefetch_ticks-1);
#endif

    if( MID_PARSER_FetchEvents(prefetch_offset, prefetch_ticks) == 0 ) {
      if( !seq_midply_loop_mode ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - stopped playback.\n", midi_file_tick);
#endif
	loop_range = 0;
	loop_offset = 0;
	loop_req = 0;
	synched_start_tick = 0;
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ] End of song reached after %u ticks - restart request!\n", midi_file_tick);
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
      loop_range = midi_file_tick;
    loop_offset += loop_range;
    SEQ_MIDPLY_SongPos(0, 0);
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ] Song restarted at tick %u, loop range: %u\n", midi_file_tick, loop_range);
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
  if( midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0) ) {
    event_type = SEQ_MIDI_OUT_OffEvent;
    unplayed_note_off[midi_package.chn][midi_package.note / 32] &= ~(1 << (midi_package.note % 32));
  } else if( midi_package.event == NoteOn && midi_package.velocity > 0 )
    unplayed_note_off[midi_package.chn][midi_package.note / 32] |= (1 << (midi_package.note % 32));
  else if( midi_package.event == CC )
    event_type = SEQ_MIDI_OUT_CCEvent;

  // Note On tracker (to play Note Off if sequencer is stopped)

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

