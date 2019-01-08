// $Id: seq_midexp.c 1538 2012-11-21 22:02:21Z tk $
/*
 * MIDI File Exporter
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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

#include "seq_midexp.h"
#include "seq_midply.h"
#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_song.h"
#include "seq_midi_port.h"
#include "seq_midi_router.h"
#include "seq_ui.h"

#include "file.h"
#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static seq_midexp_mode_t seq_midexp_mode;

static u16 export_measures;
static u8 export_steps_per_measure;

static s8 export_track;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDEXP_Init(u32 mode)
{
  // init default settings
  seq_midexp_mode = SEQ_MIDEXP_MODE_AllGroups;
  export_measures = 0; // 1 measure
  export_steps_per_measure = 15; // 16 steps

  // contains 0..15 while track exported
  export_track = -1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set functions
/////////////////////////////////////////////////////////////////////////////
seq_midexp_mode_t SEQ_MIDEXP_ModeGet(void)
{
  return seq_midexp_mode;
}

s32 SEQ_MIDEXP_ModeSet(seq_midexp_mode_t mode)
{
  if( mode >= SEQ_MIDEXP_MODE_AllGroups && mode <= SEQ_MIDEXP_MODE_Song ) {
    seq_midexp_mode = mode;
  }  else {
    return -1; // invalid mode
  }

  return 0; // no error
}

s32 SEQ_MIDEXP_ExportMeasuresGet(void)
{
  return export_measures;
}
s32 SEQ_MIDEXP_ExportMeasuresSet(u16 measures)
{
  export_measures = measures;
  return 0; // no error
}

s32 SEQ_MIDEXP_ExportStepsPerMeasureGet(void)
{
  return export_steps_per_measure;
}
s32 SEQ_MIDEXP_ExportStepsPerMeasureSet(u8 steps_per_measure)
{
  export_steps_per_measure = steps_per_measure;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help functions
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDEXP_WriteWord(u32 word, u8 len)
{
  int i;
  s32 status = 0;

  // ensure big endian coding, therefore byte writes
  for(i=0; i<len; ++i)
    status |= FILE_WriteByte((u8)(word >> (8*(len-1-i))));

  return (status < 0) ? status : len;
}


static s32 SEQ_MIDEXP_WriteVarLen(u32 value)
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
// Private hooks for MIDI Scheduler
/////////////////////////////////////////////////////////////////////////////
static u32 export_tick;
static u32 export_trk_size;
static u32 export_trk_tick;

static s32 Hook_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // ignore realtime events (like MIDI clock)
  if( package.evnt0 >= 0xf8 )
    return 0;

  u8 track = package.cable; // cable field contains the track number

  // check for matching track number
  if( track != export_track )
    return 0;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_MIDEXP:%u] T:G%dT%d  P:%s  M:%02X %02X %02X\n",
	    export_tick,
	    (track / SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	    (track % SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	    SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(port)),
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
    u32 delta = export_tick - export_trk_tick;
    export_trk_size += SEQ_MIDEXP_WriteVarLen(delta);
    export_trk_size += SEQ_MIDEXP_WriteWord(word, num_bytes);
    export_trk_tick = export_tick;
  }

  return 0; // no error
}

static s32 Hook_BPM_IsRunning(void)
{
  return 1; // always running
}

static u32 Hook_BPM_TickGet(void)
{
  return export_tick;
}

static s32 Hook_BPM_Set(float bpm)
{
  // ignored
  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Export to MIDI file based on selected parameters
// returns 0 on success
// returns < 0 on misc error (see MIOS terminal)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDEXP_GenerateFile(char *path)
{
  s32 status = 0;

  u32 ppqn = SEQ_BPM_PPQN_Get();
  u32 ticks_per_measure = ((int)export_steps_per_measure + 1) * (ppqn/4);
  u32 number_ticks = ((int)export_measures + 1) * ticks_per_measure;

  u8 first_track, last_track;

  switch( seq_midexp_mode ) {
    case SEQ_MIDEXP_MODE_Track:
      first_track = SEQ_UI_VisibleTrackGet();
      last_track = first_track;
      break;

    case SEQ_MIDEXP_MODE_Group:
      first_track = ui_selected_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
      last_track = ((ui_selected_group+1) * SEQ_CORE_NUM_TRACKS_PER_GROUP) - 1;
      break;

    default:
      first_track = 0;
      last_track = SEQ_CORE_NUM_TRACKS-1;
  }

  // request control over SD Card and MIDI Out
  MUTEX_SDCARD_TAKE;
  MUTEX_MIDIOUT_TAKE;

  // install private hooks for MIDI Scheduler
  SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(Hook_MIDI_SendPackage);
  SEQ_MIDI_OUT_Callback_BPM_IsRunning_Set(Hook_BPM_IsRunning);
  SEQ_MIDI_OUT_Callback_BPM_TickGet_Set(Hook_BPM_TickGet);
  SEQ_MIDI_OUT_Callback_BPM_Set_Set(Hook_BPM_Set);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_MIDEXP_WriteFile] Export to '%s' started\n", path);
#endif

  if( (status=FILE_WriteOpen(path, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDEXP_WriteFile] Failed to open/create %s, status: %d\n", path, status);
#endif
    status = -1; // file error
    goto error;
  }

  // write file header
  u32 header_size = 6;
  status |= FILE_WriteBuffer((u8*)"MThd", 4);
  status |= SEQ_MIDEXP_WriteWord(header_size, 4);
  status |= SEQ_MIDEXP_WriteWord(1, 2); // MIDI File Format
  status |= SEQ_MIDEXP_WriteWord(last_track-first_track+1, 2); // Number of Tracks
  status |= SEQ_MIDEXP_WriteWord(ppqn, 2); // PPQN
  status |= FILE_WriteClose();

  // check file status
  if( status < 0 ) {
    // File Access Error
    status = -2;
    goto error;
  }


  // stop sequencer
  SEQ_BPM_Stop();
  SEQ_SONG_Reset(0);
  SEQ_CORE_Reset(0);
  SEQ_MIDPLY_Reset();
  SEQ_MIDPLY_DisableFile(); // ensure that MIDI file won't be played in parallel... just disable it

  // play off events
  SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
  SEQ_CORE_PlayOffEvents();
  SEQ_MIDPLY_PlayOffEvents();

  // select song mode if required
  SEQ_SONG_ActiveSet(seq_midexp_mode == SEQ_MIDEXP_MODE_Song);

  // generate events track by track
  for(export_track=first_track; export_track<=last_track; ++export_track) {

    // print message on screen
    char str_buffer[21];
    sprintf(str_buffer, "Exporting G%dT%d to",
	    (export_track / SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	    (export_track % SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1);

    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, str_buffer, path);

#ifndef MIOS32_FAMILY_EMULATION
    // workaround: give UI some time to update screen!
    // background: buttons have higher priority than LCD output, especially with MUTEX_MIDI_OUT the priority
    // will be even higher, so that the LCD update task is starving.
    // waiting for some mS ensures that the other tasks are serviced.
    vTaskDelay(100 / portTICK_RATE_MS);
#endif

    // reset sequencer
    SEQ_SONG_Reset(0);
    SEQ_CORE_Reset(0);

    // open file again
    if( (status=FILE_WriteOpen(path, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_MIDEXP_WriteFile] Failed to open %s again, status: %d\n", path, status);
#endif
      status = -3; // file re-open error
      goto error;
    }

    // write Track header
    u32 track_header_filepos = FILE_WriteGetCurrentSize();
    status |= FILE_WriteSeek(track_header_filepos);
    export_trk_size = 0;
    export_trk_tick = 0;
    status |= FILE_WriteBuffer((u8*)"MTrk", 4);
    status |= SEQ_MIDEXP_WriteWord(export_trk_size, 4); // Placeholder

    // add track name as meta event
    {
      char buffer[20];

      export_trk_size += SEQ_MIDEXP_WriteVarLen(0);
      buffer[0] = 0xff; // Meta
      export_trk_size += SEQ_MIDEXP_WriteWord(buffer[0], 1);
      buffer[0] = 0x03; // Sequence/Track Name
      export_trk_size += SEQ_MIDEXP_WriteWord(buffer[0], 1);
      export_trk_size += SEQ_MIDEXP_WriteVarLen(4); // String Length (4 chars)
      sprintf(buffer, "G%dT%d",
	      (export_track / SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	      (export_track % SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1);
      status |= FILE_WriteBuffer((u8*)buffer, 4);
      export_trk_size += 4;
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    // send debug message
    DEBUG_MSG("[SEQ_MIDEXP_WriteFile] generating track G%dT%d at filepos %d\n",
	      (export_track / SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	      (export_track % SEQ_CORE_NUM_TRACKS_PER_GROUP) + 1,
	      track_header_filepos - 4);
#endif

    // start export of selected track
    for(export_tick=0; export_tick < number_ticks; ++export_tick) {
      // propagate tick
      SEQ_CORE_Tick(export_tick, export_track, 0);

      // load new songpos/pattern if reference step reached measure
      if( seq_core_state.ref_step == seq_core_steps_per_pattern && (export_tick % 96) == 20 ) {
	if( SEQ_SONG_ActiveGet() ) {
	  SEQ_SONG_NextPos();
	} else if( seq_core_options.SYNCHED_PATTERN_CHANGE ) {
	  SEQ_PATTERN_Handler();
	}
      }

      // forward MIDI events to Hook_MIDI_SendPackage()
      SEQ_MIDI_OUT_Handler();
    }

    // close file
    status |= FILE_WriteClose();

    if( export_trk_size ) {
      // switch back to first byte of track and write final track size
      if( (status=FILE_WriteOpen(path, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_MIDEXP_WriteFile] Failed to open %s again, status: %d\n", path, status);
#endif
	status = -3; // file re-open error
	goto error;
      }
      status |= FILE_WriteSeek(track_header_filepos + 4);
      status |= SEQ_MIDEXP_WriteWord(export_trk_size, 4);
      status |= FILE_WriteClose();
    }
  }

error:
  // MIDI scheduler: restore default MIDI/BPM handlers
  SEQ_MIDI_OUT_Callback_MIDI_SendPackage_Set(NULL);
  SEQ_MIDI_OUT_Callback_BPM_IsRunning_Set(NULL);
  SEQ_MIDI_OUT_Callback_BPM_TickGet_Set(NULL);
  SEQ_MIDI_OUT_Callback_BPM_Set_Set(NULL);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_MIDEXP_WriteFile] Export to '%s' finished with status %d\n", path, status);
#endif

  // no track exported anymore
  export_track = -1;

  // give back control over SD Card and MIDI Out
  MUTEX_MIDIOUT_GIVE;
  MUTEX_SDCARD_GIVE;

  return 0; // no error
}
