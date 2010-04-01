// $Id$
/*
 * MIDI File Importer
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

#include <ff.h>
#include <string.h>

#include "tasks.h"

#include <seq_midi_out.h>
#include <mid_parser.h>

#include "seq_midimp.h"
#include "seq_midply.h"
#include "seq_core.h"
#include "seq_file.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static u32 SEQ_MIDIMP_read(void *buffer, u32 len);
static s32 SEQ_MIDIMP_eof(void);
static s32 SEQ_MIDIMP_seek(u32 pos);

static s32 SEQ_MIDIMP_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 SEQ_MIDIMP_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static seq_midimp_mode_t seq_midimp_mode;

static u8 seq_midimp_resolution;
static u8 seq_midimp_num_layers;

// filename
#define MIDIFILE_PATH_LEN_MAX 20
static u8 midifile_path[MIDIFILE_PATH_LEN_MAX];

static u32 midifile_pos;
static u32 midifile_len;

static seq_file_t midifile_fi;

static u16 last_step[SEQ_CORE_NUM_TRACKS];
static u32 last_tick[SEQ_CORE_NUM_TRACKS];
static u16 midi_channel_set;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDIMP_Init(u32 mode)
{
  // init default mode
  seq_midimp_mode = SEQ_MIDIMP_MODE_AllNotes;
  seq_midimp_resolution = 0;
  seq_midimp_num_layers = 8;

  midifile_pos = 0;
  midifile_len = 0;
  midifile_path[0] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set mode
/////////////////////////////////////////////////////////////////////////////
seq_midimp_mode_t SEQ_MIDIMP_ModeGet(void)
{
  return seq_midimp_mode;
}

s32 SEQ_MIDIMP_ModeSet(seq_midimp_mode_t mode)
{
  if( mode >= SEQ_MIDIMP_MODE_AllNotes && mode <= SEQ_MIDIMP_MODE_AllDrums ) {
    seq_midimp_mode = mode;
  }  else {
    return -1; // invalid mode
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// get/set resolution
// 0: 16th notes (default), 1: 32th notes, 2: 64th notes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDIMP_ResolutionGet(void)
{
  return seq_midimp_resolution;
}

s32 SEQ_MIDIMP_ResolutionSet(u8 resolution)
{
  if( resolution >= 3 )
    return -1; // invalid setting
  seq_midimp_resolution = resolution;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// get/set number of layers (4, 8 or 16)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDIMP_NumLayersGet(void)
{
  return seq_midimp_num_layers;
}

s32 SEQ_MIDIMP_NumLayersSet(u8 num_layers)
{
  if( num_layers > 16 )
    return -1; // invalid setting
  seq_midimp_num_layers = num_layers;
  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// returns max. number of bars depending on layers and resolution
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDIMP_MaxBarsGet(void)
{
  switch( seq_midimp_resolution ) {
  case 1: return 1024 / (16*2*seq_midimp_num_layers);
  case 2: return 1024 / (16*4*seq_midimp_num_layers);
  }
  return 1024 / (16*seq_midimp_num_layers);
}


/////////////////////////////////////////////////////////////////////////////
// Imports a MIDI file based on selected parameters
// if "analyze" flag is set, midi file will only be validated
// returns 0 on success
// returns < 0 on misc error (see MIOS terminal)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDIMP_ReadFile(char *path, u8 analyze)
{
  // stop MIDI play function (if running)
  SEQ_MIDPLY_RunModeSet(0, 0);

  // install callback functions
  MIOS32_IRQ_Disable();
  MID_PARSER_InstallFileCallbacks(&SEQ_MIDIMP_read, &SEQ_MIDIMP_eof, &SEQ_MIDIMP_seek);
  MID_PARSER_InstallEventCallbacks(&SEQ_MIDIMP_PlayEvent, &SEQ_MIDIMP_PlayMeta);
  MIOS32_IRQ_Enable();

  MUTEX_SDCARD_TAKE;

  s32 status = SEQ_FILE_ReadOpen(&midifile_fi, path);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDIMP_ReadFile] failed to open file, status: %d\n", status);
#endif
    midifile_path[0] = 0; // disable file
  } else {

    // got it
    midifile_pos = 0;
    midifile_len = midifile_fi.fsize;

    strncpy(midifile_path, path, MIDIFILE_PATH_LEN_MAX);
    midifile_path[MIDIFILE_PATH_LEN_MAX-1] = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_MIDIMP_ReadFile] opened '%s' of length %u\n", path, midifile_len);
#endif

    // initialize all mbseq tracks which should be overwritten
    // TODO: currently no dedicated track can be imported
    u8 track;
    int num_steps = 1024 / seq_midimp_num_layers;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( seq_midimp_mode == SEQ_MIDIMP_MODE_AllDrums ) {
	SEQ_PAR_TrackInit(track, num_steps, 1, seq_midimp_num_layers);
	SEQ_TRG_TrackInit(track, num_steps, 1, seq_midimp_num_layers);
	SEQ_CC_Set(track, SEQ_CC_MIDI_EVENT_MODE, SEQ_EVENT_MODE_Drum);

	u8 only_layers = 0;
	u8 all_triggers_cleared = 1;
	u8 init_assignments = 1;
	SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);

	SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Velocity);
	int i, j;
	for(i=0; i<seq_midimp_num_layers; ++i)
	  for(j=0; j<num_steps; ++j)
	    SEQ_PAR_Set(track, j, 0, i, 100);

	for(i=0; i<16; ++i)
	  SEQ_LABEL_CopyPresetDrum(i, (char *)&seq_core_trk[track].name[5*i]);
      } else {
	SEQ_PAR_TrackInit(track, num_steps, seq_midimp_num_layers, 1);
	SEQ_TRG_TrackInit(track, num_steps, seq_midimp_num_layers, 1);
	SEQ_CC_Set(track, SEQ_CC_MIDI_EVENT_MODE, SEQ_EVENT_MODE_Note);

	u8 only_layers = 0;
	u8 all_triggers_cleared = 1;
	u8 init_assignments = 1;
	SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);

	SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1, SEQ_PAR_Type_Note);
	SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A2, SEQ_PAR_Type_Velocity);
	SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A3, SEQ_PAR_Type_Length);
	int i;
	for(i=3; i<16; ++i)
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_Note);

	memset((char *)seq_core_trk[track].name, ' ', 80);
      }

      SEQ_CC_Set(track, SEQ_CC_MIDI_CHANNEL, track); // will be changed once first note is played
      SEQ_CC_Set(track, SEQ_CC_MIDI_PORT, 0); // default port

      switch( seq_midimp_resolution ) {
      case 1: SEQ_CC_Set(track, SEQ_CC_CLK_DIVIDER, 7); break;
      case 2: SEQ_CC_Set(track, SEQ_CC_CLK_DIVIDER, 3); break;
      default: SEQ_CC_Set(track, SEQ_CC_CLK_DIVIDER, 15);
      }
    }

    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      last_step[track] = 0;
      last_tick[track] = 0;
    }
    midi_channel_set = 0;

    // read midifile
    MID_PARSER_Read();

    // fetch all events
    u32 tick;
    s32 fetch_status;
    u32 max_ticks = ((1024 / seq_midimp_num_layers) * 96 * MIDI_PARSER_PPQN_Get()) / 384;
    for(tick=0, fetch_status=1; tick < max_ticks && fetch_status > 0; ++tick)
      fetch_status = MID_PARSER_FetchEvents(tick, 1);
  }

  SEQ_FILE_ReadClose(&midifile_fi);

  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
static u32 SEQ_MIDIMP_read(void *buffer, u32 len)
{
  s32 status;

  if( !midifile_path[0] )
    return SEQ_FILE_ERR_NO_FILE;

  status = SEQ_FILE_ReadBuffer(buffer, len);

  return (status >= 0) ? len : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDIMP_eof(void)
{
  if( midifile_pos >= midifile_len )
    return 1; // end of file reached

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDIMP_seek(u32 pos)
{
  s32 status;

  if( !midifile_path[0] )
    return -1; // end of file reached

  midifile_pos = pos;

  if( midifile_pos >= midifile_len )
    status = -1; // end of file reached
  else {
    status = SEQ_FILE_ReadSeek(pos);    
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// called when a MIDI event should be played at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDIMP_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick)
{
  // check for track selection (TODO: select dedicated track)
  if( track >= SEQ_CORE_NUM_TRACKS )
    return 0;

  u32 step64th = (16 * tick) / MIDI_PARSER_PPQN_Get();
  int step = step64th;
  if( seq_midimp_resolution == 0 )
    step /= 4;
  else if( seq_midimp_resolution == 1 )
    step /= 2;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_MIDIMP_PlayEvent:%u] T%d S64th=%d S16th=%d: %02x %02x %02x\n",
	    tick, track, step64th, step64th/4,
	    midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
#endif

  // check for note on/off
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
    int num_steps = SEQ_PAR_NumStepsGet(track);
    int par_layer = 0;
    u8 instrument = 0;
    u8 take_note = 0;

    if( step < num_steps ) {
      if( seq_midimp_mode == SEQ_MIDIMP_MODE_AllDrums ) {
	int num_instruments = SEQ_TRG_NumInstrumentsGet(track);
	// search for instrument
	u8 *drum_notes = (u8 *)&seq_cc_trk[track].lay_const[0*16];
	for(instrument=0; instrument<num_instruments; ++instrument) {
	  if( midi_package.note == drum_notes[instrument] ) {
	    take_note = 1;
	    break;
	  }
	}

	if( take_note ) {
	  SEQ_TRG_GateSet(track, step, instrument, 1);
	  SEQ_PAR_Set(track, step, par_layer, instrument, midi_package.velocity);
	}

      } else {
	// determine free parameter layer
	if( !SEQ_TRG_GateGet(track, step, instrument) ) {
	  SEQ_TRG_GateSet(track, step, instrument, 1);
	  par_layer = 0;
	  take_note = 1;
	} else {
	  int num_par_layers = SEQ_PAR_NumLayersGet(track);
	  for(par_layer=3; par_layer<num_par_layers; ++par_layer)
	    if( !SEQ_PAR_Get(track, step, par_layer, instrument) ) {
	      take_note = 1;
	      break;
	    }
	}

	if( take_note ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_MIDIMP_PlayEvent:%u] T%d NoteOn %d %d @ step %d/layer %d\n",
		    tick, track, midi_package.note, midi_package.velocity,
		    step, par_layer);
#endif
	  SEQ_PAR_Set(track, step, par_layer, instrument, midi_package.note);
	  if( par_layer == 0 || midi_package.velocity > SEQ_PAR_Get(track, step, 1, instrument) )
	    SEQ_PAR_Set(track, step, 1, instrument, midi_package.velocity);
	}
      }
    }

    if( take_note ) {
      if( last_step[track] != step )
	last_tick[track] = tick;
      last_step[track] = step;

      if( step > SEQ_CC_Get(track, SEQ_CC_LENGTH) ) {
	int length = 16 * (step / 16) + 16;
	if( length > num_steps )
	  length = num_steps;

	// important: set same length for all tracks to avoid unexpected loops
	int track_local;
	for(track_local=0; track_local<SEQ_CORE_NUM_TRACKS; ++track_local)
	  SEQ_CC_Set(track_local, SEQ_CC_LENGTH, length-1);
      }

      // set midi channel on first note
      if( (midi_channel_set & (1 << track)) == 0 ) {
	midi_channel_set |= (1 << track);
	SEQ_CC_Set(track, SEQ_CC_MIDI_CHANNEL, midi_package.chn);
      }
    }
  } else if( midi_package.type == NoteOff || (midi_package.type == NoteOn && midi_package.velocity == 0) ) {
    if( seq_midimp_mode != SEQ_MIDIMP_MODE_AllDrums ) {
      // search note in last step
      int num_par_layers = SEQ_PAR_NumLayersGet(track);
      int par_layer = 0;
      u8 instrument = 0;
      u8 found_note = 0;

      if( midi_package.note == SEQ_PAR_Get(track, last_step[track], par_layer, instrument) )
	found_note = 1;
      else {
	for(par_layer=3; par_layer<num_par_layers; ++par_layer)
	  if( midi_package.note == SEQ_PAR_Get(track, last_step[track], par_layer, instrument) ) {
	    found_note = 1;
	    break;
	  }
      }

      if( found_note ) {
	int num_steps = SEQ_PAR_NumStepsGet(track);

	if( step < num_steps ) {
	  u32 ppqn = 384; // / (1 << seq_midimp_resolution);
	  int len = ((tick-last_tick[track]) * ppqn) / MIDI_PARSER_PPQN_Get();

#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[SEQ_MIDIMP_PlayEvent:%u] T%d NoteOff %d @ step %d/layer %d -> len = %d\n",
		    tick, track, midi_package.note,
		    step, par_layer, len);
#endif
	  int i;
	  for(i=last_step[track]; i<step; ++i) {
	    if( len < 0 )
	      break;
	    SEQ_PAR_Set(track, i, 2, instrument, 96);
	    len -= 96;
	  }
	  if( len > 0 )
	    SEQ_PAR_Set(track, step, 2, instrument, len);
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called when a Meta event should be played/processed at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDIMP_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick)
{
  if( meta == 0x03 ) { // Sequence/Track Name
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_MIDIMP:%d:%u] Meta - Track Name: %s\n", track, tick, buffer);
#endif
  } else if( meta == 0x2f ) { // End of Track
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_MIDIMP:%d:%u] Meta - End of Track\n", track, tick, meta);
#endif
  }

  return 0; // no error
}
