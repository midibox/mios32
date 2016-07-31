// $Id$
/*
 * Routines for MBHP_BLM_SCALAR access
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
#include "tasks.h"

#include <blm_scalar_master.h>

#include "seq_blm.h"
#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_song.h"
#include "seq_midply.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"
#include "seq_scale.h"
#include "seq_midi_in.h"
#include "seq_record.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Variants
/////////////////////////////////////////////////////////////////////////////

// inverted velocity (piano like)
#define SEQ_BLM_KEYBOARD_INVERTED 1

/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  BLM_MODE_GRID,
  BLM_MODE_TRACKS,
  BLM_MODE_PATTERNS,
  BLM_MODE_KEYBOARD,
  BLM_MODE_303,
} blm_mode_t;


typedef enum {
  BLM_SELECTION_NONE,
  BLM_SELECTION_START,
  BLM_SELECTION_STOP,
  BLM_SELECTION_MUTE,
  BLM_SELECTION_SOLO,
  BLM_SELECTION_ALT,
  BLM_SELECTION_UPPER_TRACKS,
  BLM_SELECTION_GRID,
  BLM_SELECTION_TRACKS,
  BLM_SELECTION_PATTERNS,
  BLM_SELECTION_KEYBOARD,
  BLM_SELECTION_303
} blm_selection_t;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_blm_options_t seq_blm_options;

seq_blm_fader_t seq_blm_fader[SEQ_BLM_NUM_FADERS];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_BLM_ButtonCallback(u8 blm, blm_scalar_master_element_t element_type, u8 button_x, u8 button_y, u8 button_depressed);
static s32 SEQ_BLM_FaderCallback(u8 blm, u8 fader, u8 value);

static u8 SEQ_BLM_BUTTON_Hlp_TransposeNote(u8 track, u8 note);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static blm_mode_t blm_mode;

static mios32_midi_port_t blm_keyboard_port[BLM_SCALAR_MASTER_NUM_ROWS];
static u8 blm_keyboard_chn[BLM_SCALAR_MASTER_NUM_ROWS];
static u8 blm_keyboard_note[BLM_SCALAR_MASTER_NUM_ROWS];
static u8 blm_keyboard_velocity[BLM_SCALAR_MASTER_NUM_ROWS];

static u8 blm_shift_active;
static u8 blm_mute_solo_active; // 0=off, 1=mute, 2=solo
static u8 blm_alt_active;
static u8 blm_root_key;


static const blm_selection_t mode_selections_8rows[16] = {
  BLM_SELECTION_MUTE,
  BLM_SELECTION_SOLO,
  BLM_SELECTION_UPPER_TRACKS,
  BLM_SELECTION_KEYBOARD,
  BLM_SELECTION_PATTERNS,
  BLM_SELECTION_TRACKS,
  BLM_SELECTION_GRID,
  BLM_SELECTION_ALT,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE
};

static const blm_selection_t mode_selections_16rows[16] = {
  BLM_SELECTION_START,
  BLM_SELECTION_STOP,
  BLM_SELECTION_MUTE,
  BLM_SELECTION_SOLO,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_NONE,
  BLM_SELECTION_303,
  BLM_SELECTION_KEYBOARD,
  BLM_SELECTION_PATTERNS,
  BLM_SELECTION_TRACKS,
  BLM_SELECTION_GRID,
  BLM_SELECTION_ALT
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_Init(u32 mode)
{
  // init BLM_SCALAR master driver
  BLM_SCALAR_MASTER_Init(mode);

  // install BLM callbacks
  BLM_SCALAR_MASTER_ButtonCallback_Init(SEQ_BLM_ButtonCallback);
  BLM_SCALAR_MASTER_FaderCallback_Init(SEQ_BLM_FaderCallback);

  // init local variables
  seq_blm_options.ALL = 0;
  seq_blm_options.ALWAYS_USE_FTS = 1; // enabled by default for best "first impression" :)
  seq_blm_options.SWAP_KEYBOARD_COLOURS = 1; // for blue/green LEDs

  blm_mode = BLM_MODE_TRACKS; // for compatibility with 4x16 BLM, will be changed to BLM_MODE_GRID on first connection
  blm_shift_active = 0;
  blm_mute_solo_active = 0;
  blm_alt_active = 0;
  blm_root_key = 0x30;

  {
    int fader_ix;

    seq_blm_fader_t *fader = (seq_blm_fader_t *)&seq_blm_fader[0];
    for(fader_ix=0; fader_ix<SEQ_BLM_NUM_FADERS; ++fader_ix, ++fader) {
      fader->port = DEFAULT; // Trk
      fader->chn = 0; // Trk
      if( fader_ix == 0 )
	fader->send_function = 1; // CC#1
      else
	fader->send_function = 16+fader_ix-1; // CC#16...
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Grid Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateGridMode(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);


  ///////////////////////////////////////////////////////////////////////////
  // prepare red LEDs: clear all
  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<BLM_SCALAR_MASTER_NUM_ROWS; ++i)
    blm_scalar_master_leds_red[i] = 0x0000;


  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display pattern    
  // red LEDs: used in parameter view
  ///////////////////////////////////////////////////////////////////////////
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
    if( num_instruments > BLM_SCALAR_MASTER_NUM_ROWS )
      num_instruments = BLM_SCALAR_MASTER_NUM_ROWS;
    u16 step16 = ui_selected_step_view;
    int i;
    for(i=0; i<num_instruments; ++i) {
      // TODO: how about using red LEDs for accent?
      blm_scalar_master_leds_green[i] = SEQ_TRG_Get16(visible_track, step16, ui_selected_trg_layer, i);
    }
    while( i < BLM_SCALAR_MASTER_NUM_ROWS )
      blm_scalar_master_leds_green[i++] = 0x0000;

    // adjust display offset on limited displays
    {
      u8 row_offset = 0;
      if( num_rows <= 4 )
	row_offset = ui_selected_instrument & 0xfc;
      else if( num_rows <= 8 )
	row_offset = ui_selected_instrument & 0xf8;
      BLM_SCALAR_MASTER_RowOffsetSet(0, row_offset);
    }
  } else {
#if 0
    u8 num_layers = SEQ_TRG_NumLayersGet(visible_track);
    u16 step16 = 16*ui_selected_step_view;
    int i;
    for(i=0; i<num_layers; ++i)
      blm_scalar_master_leds_green[i] = SEQ_TRG_Get16(visible_track, step16, i, 0);
    while( i < BLM_SCALAR_MASTER_NUM_ROWS )
      blm_scalar_master_leds_green[i++] = 0x0000;

    // adjust display offset on limited displays
    {
      u8 row_offset = 0;
      if( num_rows <= 4 )
	row_offset = ui_selected_trg_layer & 0xfc;
      else if( num_rows <= 8 )
	row_offset = ui_selected_trg_layer & 0xf8;
      BLM_SCALAR_MASTER_RowOffsetSet(0, row_offset);
    }
#else
    BLM_SCALAR_MASTER_RotateViewSet(0, 1);

    // branch depending on parameter layer type
    if( (seq_par_layer_type_t)seq_cc_trk[visible_track].lay_const[ui_selected_par_layer] != SEQ_PAR_Type_Note ) {
      u8 instrument = 0;
      int step = 16*ui_selected_step_view;

      if( BLM_SCALAR_MASTER_NumRowsGet(0) <= 8 ) {
	int i;
	for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i, ++step) {
	  if( SEQ_TRG_GateGet(visible_track, step, 0) ) {
	    u8 par = (SEQ_PAR_Get(visible_track, step, ui_selected_par_layer, instrument) >> 4) & 0x07;
	    blm_scalar_master_leds_green[i] = (0xffff80 >> par) & 0xfc;
	    blm_scalar_master_leds_red[i] = (0xffff80 >> par) & 0x1f;
	  } else {
	    blm_scalar_master_leds_green[i] = 0x0000;
	  }
	}
      } else {
	int i;
	for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i, ++step) {
	  if( SEQ_TRG_GateGet(visible_track, step, 0) ) {
	    u8 par = (SEQ_PAR_Get(visible_track, step, ui_selected_par_layer, instrument) >> 3) & 0x0f;
	    blm_scalar_master_leds_green[i] = (0xffff8000 >> par) & 0xfff8;
	    blm_scalar_master_leds_red[i] = (0xffff8000 >> par) & 0x00ff;
	  } else {
	    blm_scalar_master_leds_green[i] = 0x0000;
	  }
	}
      }
    } else {
      u8 use_scale = seq_blm_options.ALWAYS_USE_FTS ? 1 : seq_cc_trk[visible_track].mode.FORCE_SCALE;
      u8 scale, root_selection, root;
      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
      if( root_selection == 0 )
	root = 0; // force root to C (don't use KEYB based root)

      u8 instrument = 0;
      int step = 16*ui_selected_step_view;
      int i;
      for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i, ++step) {
	u16 pattern = 0;
	if( SEQ_TRG_GateGet(visible_track, step, 0) ) {
	  int note = blm_root_key + (use_scale ? root : 0);

	  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
	  u8 *layer_type = (u8 *)&seq_cc_trk[visible_track].lay_const[0];
	  int note_ix;
	  for(note_ix=0; note_ix<num_rows; ++note_ix) {
	    u8 next_note;
	    if( use_scale )
	      next_note = SEQ_SCALE_NextNoteInScale(note, scale, root);
	    else
	      next_note = note + 1;

	    int par_layer;
	    for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	      seq_par_layer_type_t par_type = layer_type[par_layer];
	      if( (par_type == SEQ_PAR_Type_Note) ) {
		u8 stored_note = SEQ_PAR_Get(visible_track, step, par_layer, instrument);
		if( stored_note >= note && stored_note < next_note )
		  pattern |= (1 << (num_rows-1-note_ix));
	      }
	    }

	    if( use_scale )
	      note = next_note;
	    else
	      ++note;
	  }
	}

	blm_scalar_master_leds_green[i] = pattern;
      }
    }
#endif
  }

  ///////////////////////////////////////////////////////////////////////////
  // red LEDs: display position marker
  ///////////////////////////////////////////////////////////////////////////
  u8 sequencer_running = SEQ_BPM_IsRunning();
  if( sequencer_running ) {
    int played_step = seq_core_trk[visible_track].step;
    int step_in_view = played_step % 16;
    if( (played_step / 16) == ui_selected_step_view ) {
      if( BLM_SCALAR_MASTER_RotateViewGet(0) ) {
	blm_scalar_master_leds_red[step_in_view] = 0xffff;
	//blm_scalar_master_leds_green[step_in_view] = 0x0000; // disable green LEDs
      } else {
	u16 mask = 1 << step_in_view;
	for(i=0; i<BLM_SCALAR_MASTER_NUM_ROWS; ++i) {
	  blm_scalar_master_leds_red[i] |= mask;
	  //blm_scalar_master_leds_green[i] &= ~mask; // disable green LEDs
	}
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( blm_shift_active || blm_alt_active == 1 ) {
    blm_scalar_master_leds_extracolumn_green = blm_scalar_master_leds_extracolumn_shift_green;
    blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_shift_red;
    blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  } else {
    if( blm_alt_active == 3 ) {
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
	if( num_instruments <= num_rows ) {
	  blm_scalar_master_leds_extracolumn_green = 0xffff;
	} else {
	  blm_scalar_master_leds_extracolumn_green = (ui_selected_instrument < num_rows) ? 0x0f : 0xf0;
	}
      } else {
	u8 octave = blm_root_key / 12;
	blm_scalar_master_leds_extracolumn_green = (num_rows <= 8) ? (1 << (7-octave)) : (1 << (15-octave));
      }
      blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_green;
    }

    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_scalar_master_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_scalar_master_leds_extrarow_red = 15 << (4*played_step_view);
  }

  return 0; // no error
}

static s32 SEQ_BLM_BUTTON_GP_GridMode(u8 button_row, u8 button_column, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    // change trigger layer/instrument if required
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);

    if( button_row >= num_instruments )
      return 0;

    if( num_rows <= 4 ) {
      if( button_row >= 4 )
	return 0; // invalid channel
      ui_selected_instrument = (ui_selected_instrument & 0xfc) + button_row;
    } else if( num_rows <= 8 ) {
      if( button_row >= 8 )
	return 0; // invalid channel
      ui_selected_instrument = (ui_selected_instrument & 0xf8) + button_row;
    } else {
      ui_selected_instrument = button_row;
    }

    // -> common edit handling
    SEQ_UI_EDIT_Button_Handler(button_column % 16, depressed);
  } else {
    if( depressed ) // ignore depressed key
      return 0;

    u8 instrument = 0;

    // TODO: consider ui_selected_trg_layer?

    // branch depending on parameter layer type
    if( (seq_par_layer_type_t)seq_cc_trk[visible_track].lay_const[ui_selected_par_layer] != SEQ_PAR_Type_Note ) {
      if( button_row < 16 ) {
	ui_selected_step = 16*ui_selected_step_view + button_column;
	u8 par = 1;;
	if( num_rows <= 8 ) {
	  par = (((7-button_row) << 4) + 4) & 0x7f;
	} else {
	  par = (((15-button_row) << 3) + 4) & 0x7f;
	}
	SEQ_PAR_Set(visible_track, ui_selected_step, ui_selected_par_layer, instrument, par);

	if( seq_cc_trk[visible_track].event_mode == SEQ_EVENT_MODE_CC ) {
	  // enable gate in any case (or should we disable it if CC value is 0 or button pressed twice?)
	  SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 1);
	}
      }
    } else {
      u8 use_scale = seq_blm_options.ALWAYS_USE_FTS ? 1 : seq_cc_trk[visible_track].mode.FORCE_SCALE;

      u8 note_start;
      u8 note_next;
      if( use_scale ) {
	u8 scale, root_selection, root;
	SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
	if( root_selection == 0 )
	  root = 0; // force root to C (don't use KEYB based root)

	// determine matching note range in scale
	note_start = blm_root_key + root;
	note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	int i;
	for(i=0; i<(num_rows-1-button_row); ++i) {
	  note_start = note_next;
	  note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	}
      } else {
	note_start = (num_rows <= 8) ? (blm_root_key + 7-button_row) : (blm_root_key + 15-button_row); // C-3/E-2 ..
	note_next = note_start + 1;
      }

      u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
      u8 *layer_type = (u8 *)&seq_cc_trk[visible_track].lay_const[0];
      ui_selected_step = 16*ui_selected_step_view + button_column;
      if( !SEQ_TRG_GateGet(visible_track, ui_selected_step, instrument) ) {
	// set note on first layer
	int par_layer = 0;
	SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, note_start);
	ui_selected_par_layer = par_layer;

	// clear remaining layers assigned to note
	for(par_layer=1; par_layer<num_p_layers; ++par_layer) {
	  seq_par_layer_type_t par_type = layer_type[par_layer];
	  if( par_type == SEQ_PAR_Type_Note )
	    SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, 0x00);
	}

	// enable gate
	SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 1);
      } else {
	// search all layers for matching note. if found, clear this note
	// otherwise use layer where no note has been assigned yet
	// if all layers allocated, use first layer
	// TODO: should we cycle the layer that is used if no free layer has been found?
	int par_layer;
	int unused_layer = -1;
	int num_used_layers = 0;
	int found_note_in_layer = -1;
	for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	  seq_par_layer_type_t par_type = layer_type[par_layer];
	  if( par_type == SEQ_PAR_Type_Note ) {
	    u8 par_value = SEQ_PAR_Get(visible_track, ui_selected_step, par_layer, instrument);
	    if( !par_value ) {
	      if( unused_layer < 0 )
		unused_layer = par_layer;
	    } else {
	      ++num_used_layers;
	      if( par_value >= note_start && par_value < note_next ) {
		if( found_note_in_layer >= 0 )
		  SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, 0x00); // clear redundant note
		else
		  found_note_in_layer = par_layer; // unassigned note in scale
	      }
	    }
	  }
	}

	if( found_note_in_layer >= 0 ) {
	  if( num_used_layers < 2 ) {
	    // only one note: clear gate
	    SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 0);
	    SEQ_CORE_CancelSustainedNotes(visible_track); // cancel sustain if there are no notes played by the track anymore
	  } else {
	    // clear note
	    SEQ_PAR_Set(visible_track, ui_selected_step, found_note_in_layer, instrument, 0x00);
	  }
	} else {
	  if( unused_layer < 0 )
	    unused_layer = 0;
	  SEQ_PAR_Set(visible_track, ui_selected_step, unused_layer, instrument, note_start);
	  ui_selected_par_layer = unused_layer;
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Track Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateTrackMode(void)
{
  int i;
  u8 sequencer_running = SEQ_BPM_IsRunning();
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);

  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display selected trigger layer
  // red LEDs: display position marker
  ///////////////////////////////////////////////////////////////////////////
  for(i=0; i<BLM_SCALAR_MASTER_NUM_ROWS; ++i) {
    blm_scalar_master_leds_green[i] = SEQ_TRG_Get16(i, ui_selected_step_view, ui_selected_trg_layer, ui_selected_instrument);
    blm_scalar_master_leds_red[i] = 0x0000;

    if( sequencer_running ) {
      int played_step = seq_core_trk[i].step;
      int step_in_view = played_step % 16;
      if( (played_step / 16) == ui_selected_step_view ) {
	u16 mask = 1 << step_in_view;
	blm_scalar_master_leds_red[i] = mask;
	blm_scalar_master_leds_green[i] &= ~mask; // disable green LEDs
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // adjust display offset on limited displays
  ///////////////////////////////////////////////////////////////////////////
  {
    u8 row_offset = 0;
    if( num_rows <= 4 )
      row_offset = 4*ui_selected_group;
    else if( num_rows <= 8 )
      row_offset = 8*(ui_selected_group >> 1);
    BLM_SCALAR_MASTER_RowOffsetSet(0, row_offset);
  }

  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( blm_shift_active || blm_alt_active == 1 ) {
    blm_scalar_master_leds_extracolumn_green = blm_scalar_master_leds_extracolumn_shift_green;
    blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_shift_red;
    blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  } else {
    u8 visible_track = SEQ_UI_VisibleTrackGet();

    if( blm_alt_active == 3 ) {
      blm_scalar_master_leds_extracolumn_green = 1 << ui_selected_trg_layer;
      blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_green;
    }

    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_scalar_master_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_scalar_master_leds_extrarow_red = 15 << (4*played_step_view);
  }

  return 0; // no error
}


static s32 SEQ_BLM_BUTTON_GP_TrackMode(u8 button_row, u8 button_column, u8 depressed)
{
  // -> common edit handling
  return SEQ_UI_EDIT_Button_Handler(button_column % 16, depressed);
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Pattern Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdatePatternMode(void)
{
  int i;
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);

  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display selected pattern
  // red LEDs: active if pattern is played
  // if alt function active: only print active pattern
  ///////////////////////////////////////////////////////////////////////////
  for(i=0; i<BLM_SCALAR_MASTER_NUM_ROWS; ++i) {
    u8 group = i / SEQ_CORE_NUM_TRACKS_PER_GROUP;
    u8 pnum = seq_pattern_req[group].pattern;
    int x = pnum & 0xf;
    int y = (pnum >> 4) & 0x3;

    if( blm_alt_active )
      blm_scalar_master_leds_green[i] = 0x0000;
    else
      blm_scalar_master_leds_green[i] = ((i&3) == y) ? (1 << x) : 0x0000;

    pnum = seq_pattern[group].pattern;
    x = pnum & 0xf;
    y = (pnum >> 4) & 0x3;
    blm_scalar_master_leds_red[i] = ((i&3) == y) ? (1 << x) : 0x0000;
  }

  ///////////////////////////////////////////////////////////////////////////
  // adjust display offset on limited displays
  ///////////////////////////////////////////////////////////////////////////
  {
    u8 row_offset = 0;
    if( num_rows <= 4 )
      row_offset = 4*ui_selected_group;
    else if( num_rows <= 8 )
      row_offset = 8*(ui_selected_group >> 1);
    BLM_SCALAR_MASTER_RowOffsetSet(0, row_offset);
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( blm_shift_active || blm_alt_active == 1 ) {
    blm_scalar_master_leds_extracolumn_green = blm_scalar_master_leds_extracolumn_shift_green;
    blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_shift_red;
    blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  } else {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_scalar_master_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_scalar_master_leds_extrarow_red = 15 << (4*played_step_view);
  }

  return 0; // no error
}

static s32 SEQ_BLM_BUTTON_GP_PatternMode(u8 button_row, u8 button_column, u8 depressed)
{
  if( button_column >= 16 )
    return 0; // only key 0..15 supported

  // if ALT selected: store pattern
  u8 force_immediate_change = 0;
  if( blm_alt_active ) {
    force_immediate_change = 1;

    {
      seq_pattern_t new_pattern = seq_pattern[ui_selected_group];
      new_pattern.pattern = ((button_row&0x3) << 4) | button_column;
      
#ifndef MBSEQV4L
      s32 status;
      if( (status=SEQ_PATTERN_Save(ui_selected_group, new_pattern)) < 0 ) {
	SEQ_UI_SDCardErrMsg(2000, status);
      } else {
	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Pattern saved", "on SD Card!");
      }
#endif
      // change pattern number directly
      new_pattern.REQ = 0;
      seq_pattern[ui_selected_group] = seq_pattern_req[ui_selected_group] = new_pattern;
    }

#ifdef MBSEQV4L
    {
      seq_pattern_t new_pattern = seq_pattern[ui_selected_group+1];
      new_pattern.pattern = ((button_row&0x3) << 4) | button_column;
      SEQ_PATTERN_Save(ui_selected_group+1, new_pattern);
      // change pattern number directly
      new_pattern.REQ = 0;
      seq_pattern[ui_selected_group+1] = seq_pattern_req[ui_selected_group+1] = new_pattern;
    }
#endif
  } else {
    // change to pattern
    {
      seq_pattern_t new_pattern = seq_pattern[ui_selected_group];
      new_pattern.pattern = ((button_row&0x3) << 4) | button_column;
      SEQ_PATTERN_Change(ui_selected_group, new_pattern, force_immediate_change);
    }

#ifdef MBSEQV4L
    {
      seq_pattern_t new_pattern = seq_pattern[ui_selected_group+1];
      new_pattern.pattern = ((button_row&0x3) << 4) | button_column;
      SEQ_PATTERN_Change(ui_selected_group+1, new_pattern, force_immediate_change);
    }
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for Keyboard Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_UpdateKeyboardMode(void)
{
  int i;
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 sequencer_running = SEQ_BPM_IsRunning();
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);

  ///////////////////////////////////////////////////////////////////////////
  // green/red LEDs: display played note and velocity
  ///////////////////////////////////////////////////////////////////////////

  BLM_SCALAR_MASTER_RotateViewSet(0, 1);

  u16 *keyboard_leds_green = (u16 *)(seq_blm_options.SWAP_KEYBOARD_COLOURS ? &blm_scalar_master_leds_red : &blm_scalar_master_leds_green);
  u16 *keyboard_leds_red = (u16 *)(seq_blm_options.SWAP_KEYBOARD_COLOURS ? &blm_scalar_master_leds_green : &blm_scalar_master_leds_red);

  if( num_rows <= 8 ) {
    for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i) {
      if( blm_keyboard_velocity[i] ) {
	u8 vel3 = (blm_keyboard_velocity[i] >> 4) & 0x07;
#if SEQ_BLM_KEYBOARD_INVERTED
	// inverted velocity (piano like)
	// 0: 0x01
	// 1: 0x03
	// 2: 0x07
	// 3: 0x0f
	// ..
	// 7: 0xff
	u32 pattern = (0x1ff << vel3) >> 8;

	keyboard_leds_green[i] = pattern & 0x3f;
	keyboard_leds_red[i] = pattern & 0xf8;
#else
	// 0: 0x80
	// 1: 0xc0
	// 2: 0xe0
	// 3: 0xf0
	// ..
	// 7: 0xff
	u32 pattern = 0xffff80 >> vel3;

	keyboard_leds_green[i] = pattern & 0xfc;
	keyboard_leds_red[i] = pattern & 0x1f;
#endif
      } else {
	keyboard_leds_green[i] = 0x0000;
	keyboard_leds_red[i] = 0x0000;
      }
    }
  } else { // blm_num_rows <= 16
    for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i) {
      if( blm_keyboard_velocity[i] ) {
	u8 vel4 = (blm_keyboard_velocity[i] >> 3) & 0x0f;
#if SEQ_BLM_KEYBOARD_INVERTED
	// inverted velocity (piano like)
	// 0: 0x0001
	// 1: 0x0003
	// 2: 0x0007
	// 3: 0x000f
	// ..
	// 15: 0xffff
	u32 pattern = (0x1ffff << vel4) >> 16;

	keyboard_leds_green[i] = pattern & 0x1fff;
	keyboard_leds_red[i] = pattern & 0xff00;
#else
	// 0: 0x8000
	// 1: 0xc000
	// 2: 0xe000
	// 3: 0xf000
	// ..
	// 15: 0xffff
	u32 pattern = 0xffff8000 >> vel4;

	keyboard_leds_green[i] = pattern & 0xfff8;
	keyboard_leds_red[i] = pattern & 0x00ff;
#endif
      } else {
	keyboard_leds_green[i] = 0x0000;
	keyboard_leds_red[i] = 0x0000;
      }
    }
  }

  if( sequencer_running ) {
    int played_step = seq_core_state.ref_step % 16;
    blm_scalar_master_leds_red[played_step] = 0xffff; // set all red LEDs
    blm_scalar_master_leds_green[played_step] = 0x0000; // disable green LEDs
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( blm_shift_active || blm_alt_active == 1 ) {
    blm_scalar_master_leds_extracolumn_green = blm_scalar_master_leds_extracolumn_shift_green;
    blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_shift_red;
    blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  } else {
    if( blm_alt_active == 3 ) {
      u8 octave = blm_root_key / 12;
      blm_scalar_master_leds_extracolumn_green = (num_rows <= 8) ? (1 << (7-octave)) : (1 << (15-octave));
      blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_green;
    }

    u8 transposer_note = 0x3c; // C-3
    seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
    if( tcc->mode.playmode != SEQ_CORE_TRKMODE_Normal )
      transposer_note = SEQ_MIDI_IN_TransposerNoteGet(0, 1); // hold mode

    if( transposer_note >= 0x3c && transposer_note <= 0x4b )
      blm_scalar_master_leds_extrarow_green = 1 << (transposer_note - 0x3c);
    else
      blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  }

  return 0; // no error
}


static s32 SEQ_BLM_BUTTON_GP_KeyboardMode(u8 button_row, u8 button_column, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 play_note = 0;
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);

  u8 velocity = 1;
  if( num_rows <= 8 ) {
#if SEQ_BLM_KEYBOARD_INVERTED
    // inverted velocity (piano like)
    velocity = 16*button_row + 4;
#else
    velocity = 16*(15-button_row) + 4;
#endif
  } else {
#if SEQ_BLM_KEYBOARD_INVERTED
    // inverted velocity (piano like)
    velocity = 8*button_row + 4;
#else
    velocity = 8*(15-button_row) + 4;
#endif
  }

#ifdef MBSEQV4L
  u8 should_be_recorded = seq_record_state.ENABLED && (seq_record_state.ARMED_TRACKS & (1 << visible_track));
#else
  u8 should_be_recorded = seq_record_state.ENABLED; // Armed tracks not properly supported by MBSEQV4 (w/o L)
#endif

  if( depressed ) {
    // play off event - but only if depressed button matches with last one that played the note
    if( blm_keyboard_velocity[button_column] == velocity ) {
      blm_keyboard_velocity[button_column] = 0;
      play_note = 1;
    }
  } else {
    int note_start;
    int note_next;

    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      // each instrument assigned to a button
      u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track); // we assume, that PAR layer has same number of instruments!
      if( button_column < num_instruments ) {
	ui_selected_instrument = button_column;

	if( (note_start=SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument)) ) {
	  play_note = 1;
	  seq_layer_vu_meter[ui_selected_instrument] = velocity;
	}
      }
    } else {
      u8 use_scale = 1; // should we use this only for force-to-scale mode? I don't think so - for best "first impression" :)

      if( use_scale ) {
	u8 scale, root_selection, root;
	SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
	if( root_selection == 0 )
	  root = 0; // force root to C (don't use KEYB based root)

	// determine matching note range in scale
	note_start = 0x3c; // C-3
	seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
	if( tcc->mode.playmode != SEQ_CORE_TRKMODE_Normal )
	  note_start = SEQ_MIDI_IN_TransposerNoteGet(0, 1); // hold mode

	note_start += (blm_root_key - 0x3c);
	if( note_start < 0 ) note_start = 0;
	else if( note_start > 127 ) note_start = 127;

	note_start = SEQ_BLM_BUTTON_Hlp_TransposeNote(visible_track, note_start); // transpose this note based on track settings

	// FIX: ensure that this note is in the selected scale!
	note_start -= 1;
	if( note_start < 0 ) note_start = 0;
	note_start = SEQ_SCALE_NextNoteInScale(note_start, scale, root);

	note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	int i;
	for(i=0; i<button_column; ++i) {
	  note_start = note_next;
	  note_start = SEQ_BLM_BUTTON_Hlp_TransposeNote(visible_track, note_start); // transpose this note based on track settings
	  note_next = SEQ_SCALE_NextNoteInScale(note_start, scale, root);
	}
      } else {
	note_start = (BLM_SCALAR_MASTER_NumRowsGet(0) <= 8) ? (blm_root_key + 7-button_row) : (blm_root_key + 15-button_row); // C-3/E-2 ..
	note_start = SEQ_BLM_BUTTON_Hlp_TransposeNote(visible_track, note_start); // transpose this note based on track settings
	note_next = note_start;
      }

      play_note = 1;
    }

    // play off event if note still active (e.g. different velocity of same note played)
    if( blm_keyboard_velocity[button_column] ) {
      mios32_midi_package_t p;
      p.ALL = 0;
      p.cin = NoteOn;
      p.event = NoteOn;
      p.chn = blm_keyboard_chn[button_column];
      p.note = blm_keyboard_note[button_column];
      p.velocity = 0x00;

      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendPackage(blm_keyboard_port[button_column], p);
      blm_keyboard_velocity[button_column] = 0x00; // to notify that note-off has been played
      MUTEX_MIDIOUT_GIVE;

      if( should_be_recorded ) {
	u8 fwd_midi = seq_record_options.FWD_MIDI;
	seq_record_options.FWD_MIDI = 0;
	SEQ_RECORD_Receive(p, SEQ_UI_VisibleTrackGet());
	seq_record_options.FWD_MIDI = fwd_midi;
      }
    }
    
    // set new port/channel/note/velocity
    if( play_note ) {
      blm_keyboard_port[button_column] = seq_cc_trk[visible_track].midi_port;
      blm_keyboard_chn[button_column] = seq_cc_trk[visible_track].midi_chn;
      blm_keyboard_note[button_column] = note_start;
      blm_keyboard_velocity[button_column] = velocity;
    }
  }

  if( play_note ) {
    mios32_midi_package_t p;
    p.ALL = 0;
    p.cin = NoteOn;
    p.event = NoteOn;
    p.chn = blm_keyboard_chn[button_column];
    p.note = blm_keyboard_note[button_column];
    p.velocity = blm_keyboard_velocity[button_column];

    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendPackage(blm_keyboard_port[button_column], p);
    MUTEX_MIDIOUT_GIVE;

    if( should_be_recorded ) {
      u8 fwd_midi = seq_record_options.FWD_MIDI;
      seq_record_options.FWD_MIDI = 0;
      SEQ_RECORD_Receive(p, SEQ_UI_VisibleTrackGet());
      seq_record_options.FWD_MIDI = fwd_midi;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// All notes off for all keyboard notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_KeyboardAllNotesOff(void)
{
  int i;

  for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i) {
    if( blm_keyboard_velocity[i] ) {
      blm_keyboard_velocity[i] = 0;

      mios32_midi_package_t p;
      p.ALL = 0;
      p.cin = NoteOn;
      p.event = NoteOn;
      p.chn = blm_keyboard_chn[i];
      p.note = blm_keyboard_note[i];
      p.velocity = blm_keyboard_velocity[i];

      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendPackage(blm_keyboard_port[i], p);
      MUTEX_MIDIOUT_GIVE;
    }
  }

  // just to ensure...
  SEQ_RECORD_AllNotesOff();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED Update/Button Handler for 303 Mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_LED_Update303Mode(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // drums are handled like in grid mode
  if( event_mode == SEQ_EVENT_MODE_Drum )
    return SEQ_BLM_LED_UpdateGridMode();

  ///////////////////////////////////////////////////////////////////////////
  // green LEDs: display pattern
  // red LEDs: display accent, slide and octave
  ///////////////////////////////////////////////////////////////////////////
  BLM_SCALAR_MASTER_RotateViewSet(0, 1);

  u8 instrument = 0;
  u8 par_layer = 0; // always take from first layer
  int step = 16*ui_selected_step_view;
  int i;
  for(i=0; i<BLM_SCALAR_MASTER_NUM_COLUMNS; ++i, ++step) {
    u16 red_pattern = 0;
    u16 green_pattern = 0;

    if( SEQ_TRG_AccentGet(visible_track, step, instrument) ) {
      red_pattern |= (1 << 0);
    }

    if( SEQ_TRG_GlideGet(visible_track, step, instrument) ) {
      red_pattern |= (1 << 1);
    }

    u8 note = SEQ_PAR_Get(visible_track, step, par_layer, instrument);
    u8 note_octave = note / 12;
    u8 note_key = note % 12;

    if( note_octave <= 3 )
      red_pattern |= (0 << 2);
    else if( note_octave <= 4 )
      red_pattern |= (2 << 2);
    else if( note_octave <= 5 )
      red_pattern |= (1 << 2);
    else
      red_pattern |= (3 << 2);
    
    if( SEQ_TRG_GateGet(visible_track, step, instrument) ) {
      green_pattern |= (1 << (BLM_SCALAR_MASTER_NUM_COLUMNS-1-note_key));
    }

    blm_scalar_master_leds_red[i] = red_pattern;
    blm_scalar_master_leds_green[i] = green_pattern;
  }

  ///////////////////////////////////////////////////////////////////////////
  // red LEDs: display position marker
  ///////////////////////////////////////////////////////////////////////////
  u8 sequencer_running = SEQ_BPM_IsRunning();
  if( sequencer_running ) {
    int played_step = seq_core_trk[visible_track].step;
    int step_in_view = played_step % 16;
    if( (played_step / 16) == ui_selected_step_view ) {
      if( BLM_SCALAR_MASTER_RotateViewGet(0) ) {
	blm_scalar_master_leds_red[step_in_view] = 0xffff;
	blm_scalar_master_leds_green[step_in_view] = 0x0000; // disable green LEDs
      } else {
	u16 mask = 1 << step_in_view;
	for(i=0; i<BLM_SCALAR_MASTER_NUM_ROWS; ++i) {
	  blm_scalar_master_leds_red[i] ^= mask;
	  blm_scalar_master_leds_green[i] &= ~mask; // disable green LEDs
	}
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // extra LEDs
  ///////////////////////////////////////////////////////////////////////////
  if( blm_shift_active || blm_alt_active == 1 ) {
    blm_scalar_master_leds_extracolumn_green = blm_scalar_master_leds_extracolumn_shift_green;
    blm_scalar_master_leds_extracolumn_red = blm_scalar_master_leds_extracolumn_shift_red;
    blm_scalar_master_leds_extrarow_green = 0x0000;
    blm_scalar_master_leds_extrarow_red = 0x0000;
  } else {
    int num_steps = SEQ_TRG_NumStepsGet(visible_track);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_green = 1 << ui_selected_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_green = 3 << (2*ui_selected_step_view);
    else
      blm_scalar_master_leds_extrarow_green = 15 << (4*ui_selected_step_view);

    u8 played_step_view = (seq_core_trk[visible_track].step / 16);
    if( num_steps > 128 )
      blm_scalar_master_leds_extrarow_red = 1 << played_step_view;
    else if( num_steps > 64 )
      blm_scalar_master_leds_extrarow_red = 3 << (2*played_step_view);
    else
      blm_scalar_master_leds_extrarow_red = 15 << (4*played_step_view);
  }

  return 0; // no error
}

static s32 SEQ_BLM_BUTTON_GP_303Mode(u8 button_row, u8 button_column, u8 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // drums are handled like in grid mode
  if( event_mode == SEQ_EVENT_MODE_Drum )
    return SEQ_BLM_BUTTON_GP_GridMode(button_row, button_column, depressed);

  if( depressed ) // ignore depressed key
    return 0;

  u8 instrument = 0;
  u8 par_layer = 0; // always take from first layer
  ui_selected_step = 16*ui_selected_step_view + button_column;

  if( button_row == 0 ) {
    u8 accent = SEQ_TRG_AccentGet(visible_track, ui_selected_step, instrument);
    SEQ_TRG_AccentSet(visible_track, ui_selected_step, instrument, accent ? 0 : 1);
  } else if( button_row == 1 ) {
    u8 glide = SEQ_TRG_GlideGet(visible_track, ui_selected_step, instrument);
    SEQ_TRG_GlideSet(visible_track, ui_selected_step, instrument, glide ? 0 : 1);
  } else if( button_row <= 3 ) {
    u8 note = SEQ_PAR_Get(visible_track, ui_selected_step, par_layer, instrument);
    u8 note_key = note % 12;
    u8 note_octave = note / 12;

    // bit tricks
    int osel = note_octave - 3;
    if( osel < 0 ) osel = 0; else if( osel > 3 ) osel = 3;
    if( button_row == 2 )
      osel ^= 2;
    else
      osel ^= 1;

    note_octave = osel + 3;
    note = 12*note_octave + note_key;
    SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, note);
  } else {
    u8 note = SEQ_PAR_Get(visible_track, ui_selected_step, par_layer, instrument);
    u8 note_key = (16 - 1 - button_row);

    if( SEQ_TRG_GateGet(visible_track, ui_selected_step, instrument) && (note % 12) == note_key ) {
      // clear gate
      SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 0);
      SEQ_CORE_CancelSustainedNotes(visible_track); // cancel sustain if there are no notes played by the track anymore
    } else {
      // set gate and note new note
      SEQ_TRG_GateSet(visible_track, ui_selected_step, instrument, 1);

      u8 note_octave = note / 12;
      note = 12*note_octave + note_key;
      SEQ_PAR_Set(visible_track, ui_selected_step, par_layer, instrument, note);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// LED update routine of BLM
// periodically called from low-prio task
//
// MEMO: issue with using blm_num_columns: if somebody would ever use a directly
// connected BLM together with a MIDI accessed BLM_SCALAR, and if he would use a BLM_SCALAR
// with smaller size (very unlikely), this update wouldn't be complete!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM_LED_Update(void)
{
  static blm_scalar_master_connection_state_t prev_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE;
  static u8 prev_num_rows = 0;
  blm_scalar_master_connection_state_t connection_state = BLM_SCALAR_MASTER_ConnectionStateGet(0);
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);


  ///////////////////////////////////////////////////////////////////////////
  // once the BLM has been connected:
  ///////////////////////////////////////////////////////////////////////////
  if( connection_state != prev_connection_state ) {
    prev_connection_state = connection_state;

    // change to GRID mode
    if( connection_state != BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE ) {
      blm_mode = BLM_MODE_GRID;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // change default root note on layout changes
  ///////////////////////////////////////////////////////////////////////////
  if( prev_num_rows != num_rows ) {
    prev_num_rows = num_rows;
    blm_root_key = (num_rows <= 8) ? 0x3c : 0x30;
  }


  ///////////////////////////////////////////////////////////////////////////
  // call LED update handler depending on current mode
  ///////////////////////////////////////////////////////////////////////////

  // will be set to 1 if view should be rotated
  BLM_SCALAR_MASTER_RotateViewSet(0, 0);

  // row offset is used if BLM supports less than 16 rows
  BLM_SCALAR_MASTER_RowOffsetSet(0, 0);

  // (always present)
  {
    blm_scalar_master_leds_extracolumn_shift_green = 0x0000;

    // TODO: dirty - actually we should differ between mode_selections_8rows/16rows
    if( num_rows <= 8 ) {
      blm_scalar_master_leds_extracolumn_shift_red = (blm_alt_active ? 0x80 : 0x00) | (1 << (6-blm_mode)) | (blm_mute_solo_active & 3);

      if( ui_selected_group >= 2 )
	blm_scalar_master_leds_extracolumn_shift_red |= 0x04;
      else
	blm_scalar_master_leds_extracolumn_shift_green |= 0x04;
    } else {
      blm_scalar_master_leds_extracolumn_shift_red = (blm_alt_active ? 0x8000 : 0x0000) | (1 << (14-blm_mode)) | ((blm_mute_solo_active & 3) << 2) | (SEQ_BPM_IsRunning() ? 0x0001 : 0x0002);
    }
  }

  // can be overwritten by SEQ_BLM_LED_Update* functions
  {
    if( blm_mute_solo_active ) { // Mute or Solo Tracks
#if 0
      // LED Colour coding:
      // - LED off: Track won't play (due to Mute or Solo)
      // - LED green: Solo not active and track not muted
      // - LED yellow: Track soloed
      u16 unmuted_or_solo = (seq_core_trk_soloed ? 0x0000 : (seq_core_trk_muted ^ 0xffff)) | seq_core_trk_soloed;
      blm_scalar_master_leds_extracolumn_green = 0xffff & unmuted_or_solo;
      blm_scalar_master_leds_extracolumn_red = seq_core_trk_soloed & unmuted_or_solo;
#else
      // LED Colour coding (compliant with LED handling in MUTE page)
      // - LED off: Track neither muted nor soloed
      // - LED green: Track muted
      // - LED yellow: Track soloed
      // - LED red: Track muted and soloed - solo has higher priority, therefore track will be played
      if( seq_core_trk_soloed ) {
	u16 muted_and_solo = seq_core_trk_muted & seq_core_trk_soloed;
	blm_scalar_master_leds_extracolumn_green = (seq_core_trk_muted | seq_core_trk_soloed) & (muted_and_solo ^ 0xffff);
	blm_scalar_master_leds_extracolumn_red = seq_core_trk_soloed | muted_and_solo;
      } else {
	blm_scalar_master_leds_extracolumn_green = seq_core_trk_muted;
	blm_scalar_master_leds_extracolumn_red = 0x0000;
      }
#endif
    } else {
      blm_scalar_master_leds_extracolumn_green = ui_selected_tracks;
      blm_scalar_master_leds_extracolumn_red = seq_core_trk_muted;
    }

    if( num_rows <= 8 ) {
      if( ui_selected_group >= 2 ) {
	blm_scalar_master_leds_extracolumn_green >>= 8;
	blm_scalar_master_leds_extracolumn_red >>= 8;
      }
      blm_scalar_master_leds_extracolumn_green &= 0xff;
      blm_scalar_master_leds_extracolumn_red &= 0xff;
    }
  }

  switch( blm_mode ) {
    case BLM_MODE_GRID:
      SEQ_BLM_LED_UpdateGridMode();
      break;

    case BLM_MODE_PATTERNS:
      SEQ_BLM_LED_UpdatePatternMode();
      break;

    case BLM_MODE_KEYBOARD:
      SEQ_BLM_LED_UpdateKeyboardMode();
      break;

    case BLM_MODE_303:
      SEQ_BLM_LED_Update303Mode();
      break;

    default: // BLM_MODE_TRACKS
      SEQ_BLM_LED_UpdateTrackMode();
  }

  // finally update BLM
  BLM_SCALAR_MASTER_Periodic_mS();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a BLM button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_ButtonCallback(u8 blm, blm_scalar_master_element_t element_id, u8 button_x, u8 button_y, u8 button_depressed)
{
  u8 num_rows = BLM_SCALAR_MASTER_NumRowsGet(0);
  u8 num_columns = BLM_SCALAR_MASTER_NumColumnsGet(0);

  switch( element_id ) {
  case BLM_SCALAR_MASTER_ELEMENT_GRID: {

    // change display view if required
    if( blm_mode == BLM_MODE_TRACKS || blm_mode == BLM_MODE_GRID || blm_mode == BLM_MODE_303 ) {
      if( num_columns <= 16 ) {
	// don't change view
      } else if( num_columns <= 32 ) {
	ui_selected_step_view = (ui_selected_step_view & 0xfe) + ((button_x/16) & 0x01);
      } else if( num_columns <= 64 ) {
	ui_selected_step_view = (ui_selected_step_view & 0xfc) + ((button_x/16) & 0x03);
      } else if( num_columns <= 128 ) {
	ui_selected_step_view = (ui_selected_step_view & 0xf8) + ((button_x/16) & 0x07);
      } else {
	ui_selected_step_view = (button_x>>8) & 0x0f;
      }
    }

    // change track if required
    if( blm_mode == BLM_MODE_TRACKS || blm_mode == BLM_MODE_PATTERNS ) {
      u8 new_track = button_y;

#ifdef MBSEQV4L
      if( blm_mode != BLM_MODE_PATTERNS )
	ui_selected_tracks = (new_track >= 8) ? 0xff00 : 0x00ff;
      ui_selected_group = (new_track >= 8) ? 2 : 0;
      
      // armed tracks handling: alternate and select all tracks depending on seq selection
      // can be changed in RecArm page
      if( ui_selected_tracks & 0xff00 ) {
	if( (seq_record_state.ARMED_TRACKS & 0x00ff) )
	  seq_record_state.ARMED_TRACKS = 0xff00;
      } else {
	if( (seq_record_state.ARMED_TRACKS & 0xff00) )
	  seq_record_state.ARMED_TRACKS = 0x00ff;
      }
#else
      if( num_rows <= 4 ) {
	if( new_track >= 4 )
	  return 0; // invalid channel
      } else if( num_rows <= 8 ) {
	if( new_track >= 8 )
	  return 0; // invalid channel
	ui_selected_group = (ui_selected_group & 2) + (new_track / 4);
      } else {
	ui_selected_group = (new_track / 4);
      }
      if( blm_mode != BLM_MODE_PATTERNS )
	ui_selected_tracks = 1 << (4*ui_selected_group + (new_track % 4));
#endif
    }

    // request display update
    seq_ui_display_update_req = 1;
    ui_cursor_flash_ctr = ui_cursor_flash_overrun_ctr = 0;

    // call button handler
    switch( blm_mode ) {
    case BLM_MODE_PATTERNS:
      SEQ_BLM_BUTTON_GP_PatternMode(button_y, button_x, button_depressed);
      break;

    case BLM_MODE_GRID:
      SEQ_BLM_BUTTON_GP_GridMode(button_y, button_x, button_depressed);
      break;

    case BLM_MODE_KEYBOARD:
      SEQ_BLM_BUTTON_GP_KeyboardMode(button_y, button_x, button_depressed);
      break;

    case BLM_MODE_303:
      SEQ_BLM_BUTTON_GP_303Mode(button_y, button_x, button_depressed);
      break;

    default: // BLM_MODE_TRACKS
      SEQ_BLM_BUTTON_GP_TrackMode(button_y, button_x, button_depressed);
    }

    return 1; // MIDI event has been taken

  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW: {
    if( blm_shift_active || blm_alt_active ) {
      // no function yet
      return 1; // MIDI event has been taken
    } else {
      if( blm_mode == BLM_MODE_KEYBOARD ) {
	// forward to loopback port
	mios32_midi_package_t p;
	p.type = NoteOn;
	p.event = NoteOn;
	p.chn = Chn1;
	p.note = button_x + 0x3c;
	p.velocity = button_depressed ? 0 : 0x7f;
	SEQ_MIDI_IN_BusReceive(0, p, 1);
      } else {
	if( !button_depressed ) {
	  u8 visible_track = SEQ_UI_VisibleTrackGet();
	  int num_steps = SEQ_TRG_NumStepsGet(visible_track);
	  
	  // select new step view
	  ui_selected_step_view = (button_x * (num_steps/16)) / 16;
	  // select step within view
	  ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);
	}
      }
      return 1; // MIDI event has been taken
    }
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN: {
    u8 came_from_extra_shift_row = button_x >= 0x10;

    // Overlay alt functions for various modes
    if( blm_alt_active == 3 && !came_from_extra_shift_row ) {
      switch( blm_mode ) {
      case BLM_MODE_GRID: {
	u8 visible_track = SEQ_UI_VisibleTrackGet();
	u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
	  if( num_instruments > num_rows ) {
	    if( button_y >= 4 ) {
	      ui_selected_instrument |= 8;
	    } else {
	      ui_selected_instrument &= ~8;
	    }
	  }
	} else {
	  u8 octave = num_rows - 1 - button_y;
	  blm_root_key = octave*12;
	}
	  
	BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	return 1; // MIDI event has been taken
      }

      case BLM_MODE_KEYBOARD: {
	u8 octave = num_rows - 1 - button_y;
	blm_root_key = octave*12;
	BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	return 1; // MIDI event has been taken
      }
      case BLM_MODE_TRACKS: {
	u8 visible_track = SEQ_UI_VisibleTrackGet();
	u8 trg_layer = button_y;
	if( trg_layer < SEQ_TRG_NumLayersGet(visible_track) )
	  ui_selected_trg_layer = trg_layer;
	return 1; // MIDI event has been taken
      }
      }
    }

    if( !button_depressed ) {
      // disable all keyboard notes (important when switching between modes or tracks!)
      SEQ_BLM_KeyboardAllNotesOff();
    }

    // Extra Column
    if( !blm_shift_active && !came_from_extra_shift_row ) {
      if( !button_depressed ) {
	u8 new_track = button_y;

	if( num_rows <= 8 ) {
	  new_track = (new_track & 0x7) | ((ui_selected_group & 2) << 2);
	}

	u16 track_mask = 1 << new_track;
	
	if( blm_mute_solo_active == 1 ) { // Mute Tracks
	  seq_core_trk_muted ^= track_mask;
	} else if( blm_mute_solo_active == 2 ) { // Solo Tracks
	  seq_core_trk_soloed ^= track_mask;
	  seq_ui_button_state.SOLO = seq_core_trk_soloed ? 1 : 0;
	} else {
#ifdef MBSEQV4L
	  ui_selected_tracks = (new_track >= 8) ? 0xff00 : 0x00ff;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);

	  // armed tracks handling: alternate and select all tracks depending on seq selection
	  // can be changed in RecArm page
	  if( ui_selected_tracks & 0xff00 ) {
	    if( (seq_record_state.ARMED_TRACKS & 0x00ff) )
	      seq_record_state.ARMED_TRACKS = 0xff00;
	  } else {
	    if( (seq_record_state.ARMED_TRACKS & 0xff00) )
	      seq_record_state.ARMED_TRACKS = 0x00ff;
	  }
#else
	  ui_selected_tracks = track_mask;
	  ui_selected_group = (new_track / 4);
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);

	  // set/clear encoder fast function if required
	  SEQ_UI_InitEncSpeed(1); // auto config
#endif
	}
      }
      return 1; // MIDI event has been taken
    } else {
      blm_selection_t selection = (num_rows <= 8) ? mode_selections_8rows[button_y] : mode_selections_16rows[button_y];
      switch( selection ) {
      case BLM_SELECTION_START:
	if( !button_depressed ) {
	  // if in auto mode and BPM generator is clocked in slave mode:
	  // change to master mode
	  SEQ_BPM_CheckAutoMaster();
	  // always restart sequencer
	  SEQ_BPM_Start();
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_STOP:
	if( !button_depressed ) {
	  // if sequencer running: stop it
	  // if sequencer already stopped: reset song position
	  if( SEQ_BPM_IsRunning() )
	    SEQ_BPM_Stop();
	  else {
	    SEQ_SONG_Reset(0);
	    SEQ_CORE_Reset(0);
	    SEQ_MIDPLY_Reset();
	  }
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_MUTE:
	if( !button_depressed ) {
	  if( blm_alt_active ) {
	    seq_core_trk_muted = 0;
	  } else {
	    // if mute already active: clear mute mode
	    // otherwise select mute mode
	    blm_mute_solo_active = (blm_mute_solo_active == 1) ? 0 : 1;
	  }
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_SOLO:
	if( !button_depressed ) {
	  if( blm_alt_active ) {
	    seq_core_trk_soloed = 0;
	    seq_ui_button_state.SOLO = 0;
	  } else {
	    // if solo already active: clear solo mode
	    // otherwise select solo mode
	    blm_mute_solo_active = (blm_mute_solo_active == 2) ? 0 : 2;
	  }
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_UPPER_TRACKS:
	if( !button_depressed ) {
	  u16 selected_tracks = ui_selected_tracks;
	  ui_selected_group ^= 2;
	  if( ui_selected_group >= 2 ) {
	    ui_selected_tracks = selected_tracks << 8;
	  } else {
	    ui_selected_tracks = selected_tracks >> 8;
	  }
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_GRID:
	if( !button_depressed ) {
	  blm_mode = BLM_MODE_GRID;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_TRACKS:
	if( !button_depressed ) {
	  blm_mode = BLM_MODE_TRACKS;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_PATTERNS:
	if( !button_depressed ) {
	  blm_mode = BLM_MODE_PATTERNS;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_KEYBOARD:
	if( !button_depressed ) {
	  blm_mode = BLM_MODE_KEYBOARD;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_303:
	if( !button_depressed ) {
	  blm_mode = BLM_MODE_303;
	  BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	}
	return 1; // MIDI event has been taken

      case BLM_SELECTION_ALT:
	if( !button_depressed ) {
	  // set to 3 if ALT pressed from extra shift row of a virtual BLM (not available in original BLM16x16+X hardware)
	  blm_alt_active = came_from_extra_shift_row ? 3 : 1;
	} else {
	  blm_alt_active = 0;
	}
	BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
	return 1; // MIDI event has been taken
      }
    }
  } break;
    
  case BLM_SCALAR_MASTER_ELEMENT_SHIFT: {
    // shift button
    blm_shift_active = button_depressed ? 0 : 1;
    blm_alt_active = 0;
    BLM_SCALAR_MASTER_ForceDisplayUpdate(0);
    return 1; // MIDI event has been taken
  } break;
    
  default:
    return -1; // unexpected element_id
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a BLM fader has been moved
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BLM_FaderCallback(u8 blm, u8 fader, u8 value)
{
  seq_blm_fader_t *f = &seq_blm_fader[fader];
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];

  mios32_midi_port_t port = (f->port == 0) ? tcc->midi_port : f->port;
  u8 chn = (f->chn == 0) ? tcc->midi_chn : (f->chn - 1);

  if( f->send_function < 128 ) {
    // send CC
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendCC(port, chn, f->send_function & 0x7f, value);
    MUTEX_MIDIOUT_GIVE;      
  } else if( f->send_function < 256 ) {
    // send inverted CC
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendCC(port, chn, f->send_function & 0x7f, 127 - value);
    MUTEX_MIDIOUT_GIVE;      
  } else {
    // special functions
    // TODO
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to transpose a note based settings of given track
/////////////////////////////////////////////////////////////////////////////
static u8 SEQ_BLM_BUTTON_Hlp_TransposeNote(u8 track, u8 note)
{
  int tr_note = note;
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  int inc_oct = tcc->transpose_oct;
  if( inc_oct >= 8 )
    inc_oct -= 16;

  int inc_semi = tcc->transpose_semi;
  if( inc_semi >= 8 )
    inc_semi -= 16;

  // apply transpose octave/semitones parameter
  if( inc_oct ) {
    tr_note += 12 * inc_oct;
  }

  if( inc_semi ) {
    tr_note += inc_semi;
  }

  // ensure that note is in the 0..127 range
  tr_note = SEQ_CORE_TrimNote(tr_note, 0, 127);

  return tr_note;
}
