// $Id$
/*
 * Edit page
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
#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_cc_labels.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_chord.h"
#include "seq_record.h"
#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////
seq_ui_edit_view_t seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS;


/////////////////////////////////////////////////////////////////////////////
// Global Defines
/////////////////////////////////////////////////////////////////////////////
#define DATAWHEEL_MODE_SCROLL_CURSOR   0
#define DATAWHEEL_MODE_SCROLL_VIEW     1
#define DATAWHEEL_MODE_CHANGE_VALUE    2
#define DATAWHEEL_MODE_CHANGE_PARLAYER 3
#define DATAWHEEL_MODE_CHANGE_TRGLAYER 4

#define DATAWHEEL_MODE_NUM             5


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u16 selected_steps = 0xffff; // will only be initialized once after startup

static u8 datawheel_mode = 0; // will only be initialized once after startup

// activated by pressing EDIT button: encoder value will be taken over by releasing EDIT button
// mode 0: function not active (EDIT button released)
// mode 1: function activated (EDIT button pressed)
// mode 2: value has been changed while EDIT button pressed (a message will pop up on screen)
static u8 edit_passive_mode;
static u8 edit_passive_value;      // the tmp. edited value
static u8 edit_passive_track;      // to store the track of the edit value
static u8 edit_passive_step;       // to store the step of the edit value
static u8 edit_passive_par_layer;  // to store the layer of the edit value
static u8 edit_passive_instrument; // to store the instrument of the edit value

/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MIDI_LEARN_MODE_OFF = 0,
  MIDI_LEARN_MODE_ON,
} midi_learn_mode_t;

static midi_learn_mode_t midi_learn_mode = MIDI_LEARN_MODE_OFF;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 ChangeSingleEncValue(u8 track, u16 par_step, u16 trg_step, s32 incrementer, s32 forced_value, u8 change_gate, u8 dont_change_gate);
static s32 PassiveEditEnter(void);
static s32 PassiveEditValid(void);
static s32 PassiveEditTakeOver(void);


/////////////////////////////////////////////////////////////////////////////
// LED handler function (globally accessible, since it's re-used by UTIL page)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  
  if( seq_ui_button_state.EDIT_PRESSED ) {
    switch( seq_ui_edit_view ) {
    case SEQ_UI_EDIT_VIEW_STEPS: *gp_leds = (1 << 0); break;
    case SEQ_UI_EDIT_VIEW_TRG: *gp_leds = (1 << 1); break;
    case SEQ_UI_EDIT_VIEW_LAYERS: *gp_leds = (1 << 2); break;
    case SEQ_UI_EDIT_VIEW_303: *gp_leds = (1 << 3); break;
    case SEQ_UI_EDIT_VIEW_STEPSEL: *gp_leds = (1 << 8); break;
    }
  } else {

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPS && seq_ui_button_state.CHANGE_ALL_STEPS && midi_learn_mode == MIDI_LEARN_MODE_OFF && !seq_record_state.ENABLED ) {
      *gp_leds = ui_cursor_flash ? 0x0000 : selected_steps;
    } else if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
      *gp_leds = selected_steps;
    } else {

      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

      if( event_mode != SEQ_EVENT_MODE_Drum &&
	  (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_303) ) {

	if( SEQ_TRG_GateGet(visible_track, ui_selected_step, ui_selected_instrument) )
	  *gp_leds |= (1 << 1);
	if( SEQ_TRG_AccentGet(visible_track, ui_selected_step, ui_selected_instrument) )
	  *gp_leds |= (1 << 2);
	if( SEQ_TRG_GlideGet(visible_track, ui_selected_step, ui_selected_instrument) )
	  *gp_leds |= (1 << 3);

	if( ui_selected_par_layer == 0 )
	  *gp_leds |= (3 << 4);
	else
	  *gp_leds |= (1 << (ui_selected_par_layer+5));

      } else if( event_mode != SEQ_EVENT_MODE_Drum &&
	  (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_LAYERS || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG) ) {

	u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);
	if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG ) {
	  // maximum 7 parameter layers due to "Step" item!
	  if( num_t_layers >= 7 )
	    num_t_layers = 7;
	} else {
	  // single trigger layer (gate)
	  num_t_layers = 1;
	}

	int i;
	for(i=0; i<num_t_layers; ++i)
	  if( SEQ_TRG_Get(visible_track, ui_selected_step, i, ui_selected_instrument) )
	    *gp_leds |= (1 << (i+1));

	if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG ) {
	  *gp_leds |= (1 << (ui_selected_par_layer+8));
	} else {
	  *gp_leds |= (1 << (ui_selected_par_layer+2));
	}
      } else {
	*gp_leds =
	  (SEQ_TRG_Get8(visible_track, 2*ui_selected_step_view+1, ui_selected_trg_layer, ui_selected_instrument) << 8) |
	  (SEQ_TRG_Get8(visible_track, 2*ui_selected_step_view+0, ui_selected_trg_layer, ui_selected_instrument) << 0);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

    switch( datawheel_mode ) {
    case DATAWHEEL_MODE_SCROLL_CURSOR:
      if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, num_steps-1, incrementer) >= 1 ) {
	ui_selected_step_view = ui_selected_step / 16;
	return 1;
      } else
	return 0;

    case DATAWHEEL_MODE_SCROLL_VIEW:
      if( SEQ_UI_Var8_Inc(&ui_selected_step_view, 0, (num_steps-1)/16, incrementer) >= 1 ) {
	if( !seq_ui_button_state.CHANGE_ALL_STEPS ) {
	  // select step within view
	  ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);
	}
	return 1;
      } else {
	return 0;
      }

    case DATAWHEEL_MODE_CHANGE_VALUE:
      break; // drop... continue below with common encoder value change routine

    case DATAWHEEL_MODE_CHANGE_PARLAYER: {
      u8 num_layers = SEQ_PAR_NumLayersGet(visible_track);

      if( SEQ_UI_Var8_Inc(&ui_selected_par_layer, 0, num_layers-1, incrementer) >= 1 )
	return 1;
      else
	return 0;
    } break;

    case DATAWHEEL_MODE_CHANGE_TRGLAYER: {
      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	u8 num_layers = SEQ_TRG_NumInstrumentsGet(visible_track);
	if( SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_layers-1, incrementer) >= 1 )
	  return 1;
	else
	  return 0;
      } else {
	u8 num_layers = SEQ_TRG_NumLayersGet(visible_track);
	if( SEQ_UI_Var8_Inc(&ui_selected_trg_layer, 0, num_layers-1, incrementer) >= 1 )
	  return 1;
	else
	  return 0;
      }
    } break;
    }
  }

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) || encoder == SEQ_UI_ENCODER_Datawheel ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 || encoder == SEQ_UI_ENCODER_Datawheel ) {
#endif

    if( seq_ui_button_state.EDIT_PRESSED ) {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP1: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS; break;
      case SEQ_UI_ENCODER_GP2: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_TRG; break;
      case SEQ_UI_ENCODER_GP3: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_LAYERS; break;
      case SEQ_UI_ENCODER_GP4: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_303; break;
      case SEQ_UI_ENCODER_GP8: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPSEL; break;

      case SEQ_UI_ENCODER_GP9:
      case SEQ_UI_ENCODER_GP10: {
	if( incrementer == 0 ) // button
	  incrementer = (encoder == SEQ_UI_ENCODER_GP9) ? -1 : 1;

	if( SEQ_UI_Var8_Inc(&datawheel_mode, 0, DATAWHEEL_MODE_NUM-1, incrementer) >= 1 )
	  return 1;
	else
	  return 0;
      }

      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12:
	SEQ_UI_PageSet(SEQ_UI_PAGE_TRKREC);
	break;

      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
	SEQ_UI_PageSet(SEQ_UI_PAGE_TRKRND);
	break;

      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16:
	SEQ_UI_PageSet(SEQ_UI_PAGE_TRKEUCLID);
	break;
      }

      seq_ui_button_state.EDIT_PRESSED = 0; // switch back to view
      return 1; // value changed
    }

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
      if( incrementer > 0 )
	selected_steps |= (1 << encoder);
      else
	selected_steps &= ~(1 << encoder);
      return 1; // value changed
    }

    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

    if( event_mode != SEQ_EVENT_MODE_Drum &&
	(seq_ui_edit_view == SEQ_UI_EDIT_VIEW_303) ) {
      u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

      if( encoder == SEQ_UI_ENCODER_GP1 ) {
	if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, num_steps-1, incrementer) >= 1 )
	  return 1;
	else
	  return 0;
      } else if( encoder == SEQ_UI_ENCODER_GP2 ) {
	SEQ_TRG_GateSet(visible_track, ui_selected_step, ui_selected_instrument, incrementer > 0 ? 1 : 0);
	return 1;
      } else if( encoder == SEQ_UI_ENCODER_GP3 ) {
	SEQ_TRG_AccentSet(visible_track, ui_selected_step, ui_selected_instrument, incrementer > 0 ? 1 : 0);
	return 1;
      } else if( encoder == SEQ_UI_ENCODER_GP4 ) {
	SEQ_TRG_GlideSet(visible_track, ui_selected_step, ui_selected_instrument, incrementer > 0 ? 1 : 0);
	return 1;
      } else if( encoder == SEQ_UI_ENCODER_GP5 ) {
	ui_selected_par_layer = 0;
	u8 note = SEQ_PAR_Get(visible_track, ui_selected_step, ui_selected_par_layer, ui_selected_instrument);
	u8 note_octave = note / 12;
	u8 note_key = note % 12;

	if( SEQ_UI_Var8_Inc(&note_octave, 0, 9, incrementer) >= 1 ) {
	  SEQ_PAR_Set(visible_track, ui_selected_step, 0, ui_selected_instrument, 12*note_octave + note_key);
	  return 1;
	}
	return 0;
      } else if( encoder == SEQ_UI_ENCODER_GP6 ) {
	ui_selected_par_layer = 0;
	u8 note = SEQ_PAR_Get(visible_track, ui_selected_step, ui_selected_par_layer, ui_selected_instrument);
	u8 note_octave = note / 12;
	u8 note_key = note % 12;

	if( SEQ_UI_Var8_Inc(&note_key, 0, 11, incrementer) >= 1 ) {
	  SEQ_PAR_Set(visible_track, ui_selected_step, 0, ui_selected_instrument, 12*note_octave + note_key);
	  return 1;
	}
	return 0;
      } else if( encoder <= SEQ_UI_ENCODER_GP16 ) {
	u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
	if( ((int)encoder-5) >= num_p_layers )
	    return 0; // ignore
	  ui_selected_par_layer = encoder-5;
      }

      if( !incrementer ) // button selection only...
	return 1;
    }

    if( event_mode != SEQ_EVENT_MODE_Drum &&
      (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_LAYERS || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG) ) {
      u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

      if( encoder == SEQ_UI_ENCODER_GP1 ) {
	if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, num_steps-1, incrementer) >= 1 )
	  return 1;
	else
	  return 0;
      } else if( encoder == SEQ_UI_ENCODER_GP2 ||
		 (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG && encoder <= SEQ_UI_ENCODER_GP8) ) {
	u8 sel = (u8)encoder-1;
	SEQ_TRG_Set(visible_track, ui_selected_step, sel, ui_selected_instrument, incrementer > 0 ? 1 : 0);
	return 1;
      } else if( encoder <= SEQ_UI_ENCODER_GP16 ) {

	if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG ) {
	  if( encoder <= SEQ_UI_ENCODER_GP8 ) {
	    u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);
	    if( ((int)encoder-2) >= num_t_layers )
	      return 0; // ignore
	    ui_selected_trg_layer = encoder-2;
	  } else {
	    u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
	    if( ((int)encoder-8) >= num_p_layers )
	      return 0; // ignore
	    ui_selected_par_layer = encoder-8;
	  }
	} else {
	  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
	  if( ((int)encoder-2) >= num_p_layers )
	    return 0; // ignore
	  ui_selected_par_layer = encoder-2;
	}

	if( !incrementer ) // button selection only...
	  return 1;
      }
    }

    u8 changed_step;
    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPS ) {
      changed_step = ((encoder == SEQ_UI_ENCODER_Datawheel) ? (ui_selected_step%16) : encoder) + ui_selected_step_view*16;
    } else {
      changed_step = ui_selected_step;
    }

    u8 edit_ramp = 0;
    if( event_mode == SEQ_EVENT_MODE_Drum || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPS ) {

      // in passive edit mode: take over the edit value if step has changed, thereafter switch to new step
      if( ui_selected_step != changed_step && edit_passive_mode ) {
	PassiveEditTakeOver();
	ui_selected_step = changed_step;
	PassiveEditEnter();
      } else {
	// take over new step if "ALL" button not pressed to support "ramp" editing
	if( !seq_ui_button_state.CHANGE_ALL_STEPS ) {
	  ui_selected_step = changed_step;
	} else {
	  if( ui_selected_step != changed_step )
	    edit_ramp = 1;
	}
      }

    }


    // in passive edit mode: change value, but don't take over yet!
    if( edit_passive_mode ) {
      if( SEQ_UI_Var8_Inc(&edit_passive_value, 0, 127, incrementer) >= 1 ) {
	edit_passive_mode = 2; // value has been changed
	return 1;
      } else
	return 0;
    }

    // normal edit mode
    s32 value_changed = 0;
    s32 forced_value = -1;
    u8  change_gate = 1;

    // due to historical reasons (from old times where MBSEQ CS was stuffed with pots): 
    // in arp mode, we increment in steps of 4
    u8 par_type = SEQ_PAR_AssignmentGet(visible_track, ui_selected_par_layer);
    if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator &&
	par_type == SEQ_PAR_Type_Note )
      incrementer *= 4;

    // first change the selected value
    if( seq_ui_button_state.CHANGE_ALL_STEPS && (edit_ramp || seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE) ) {
      u16 num_steps = SEQ_PAR_NumStepsGet(visible_track);
      u16 par_step = changed_step;
      u16 trg_step = changed_step;

      // mirrored layer in drum mode?
      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum && par_step >= num_steps )
	par_step %= num_steps;

      forced_value = ChangeSingleEncValue(visible_track, par_step, trg_step, incrementer, forced_value, change_gate, 0);
      if( forced_value < 0 )
	return 0; // no change
      value_changed |= 1;
    }

    int value_selected_step = SEQ_PAR_Get(visible_track, ui_selected_step, ui_selected_par_layer, ui_selected_instrument);
    int value_changed_step = SEQ_PAR_Get(visible_track, changed_step, ui_selected_par_layer, ui_selected_instrument);

    // change value of all selected steps
    u8 track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( SEQ_UI_IsSelectedTrack(track) ) {
	u16 num_steps = SEQ_PAR_NumStepsGet(track);
	u16 trg_step = (changed_step & ~(num_steps-1));

	u16 par_step;
	for(par_step=0; par_step<num_steps; ++par_step, ++trg_step) {
	  if( !seq_ui_button_state.CHANGE_ALL_STEPS || (!edit_ramp && par_step == changed_step) || (selected_steps & (1 << (par_step % 16))) ) {
	    change_gate = trg_step == changed_step;
	    u8 dont_change_gate = par_step != changed_step;
	    if( change_gate || seq_ui_button_state.CHANGE_ALL_STEPS ) {
	      s32 local_forced_value = edit_ramp ? -1 : forced_value;

	      s32 edit_ramp_num_steps = 0;
	      if( edit_ramp ) {
		if( changed_step > ui_selected_step && par_step > ui_selected_step && par_step < changed_step ) {
		  edit_ramp_num_steps = changed_step - ui_selected_step;
		} else if( changed_step < ui_selected_step && par_step < ui_selected_step && par_step > changed_step ) {
		  edit_ramp_num_steps = ui_selected_step - changed_step;
		}

		if( edit_ramp_num_steps ) {
		  if( par_step == changed_step ) {
		    local_forced_value = value_changed_step;
		  } else {
		    int diff = value_changed_step - value_selected_step;
		    if( diff == 0 ) {
		      local_forced_value = value_changed_step;
		    } else {
		      if( changed_step > ui_selected_step ) {
			local_forced_value = value_selected_step + (((par_step - ui_selected_step) * diff) / edit_ramp_num_steps);
		      } else {
			local_forced_value = value_selected_step + (((ui_selected_step - par_step) * diff) / edit_ramp_num_steps);
		      }
		    }
		  }
		}
	      }

	      if( !edit_ramp || edit_ramp_num_steps ) {
		if( ChangeSingleEncValue(track, par_step, trg_step, incrementer, local_forced_value, change_gate, dont_change_gate) >= 0 )
		  value_changed |= 1;
	      }
	    }
	  }
	}
      }
    }
    
    return value_changed;
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Button_Handler(seq_ui_button_t button, s32 depressed)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif

    if( !seq_ui_button_state.EDIT_PRESSED &&
	((seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPS && seq_ui_button_state.CHANGE_ALL_STEPS) ||
	 seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL) ) {
      if( depressed ) return 0; // ignore when button depressed

      selected_steps ^= (1 << button);
      return 1; // value changed
    }

    // enable/disable MIDI Learn mode
    midi_learn_mode = depressed ? MIDI_LEARN_MODE_OFF : MIDI_LEARN_MODE_ON;

    if( depressed ) return 0; // ignore when button depressed

    if( seq_ui_button_state.EDIT_PRESSED )
      return Encoder_Handler(button, 0);

    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

    if( event_mode != SEQ_EVENT_MODE_Drum &&
	(seq_ui_edit_view == SEQ_UI_EDIT_VIEW_303) ) {

      if( button == SEQ_UI_BUTTON_GP1 ) {
	int next_step = ui_selected_step + 1; // (required, since ui_selected_step is only u8, but we could have up to 256 steps)
	if( next_step >= (SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1) )
	  next_step = 0;
	ui_selected_step = next_step;
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed
      } else if( button == SEQ_UI_BUTTON_GP2 ) {
	u8 trg = SEQ_TRG_GateGet(visible_track, ui_selected_step, ui_selected_instrument);
	return Encoder_Handler(button, trg ? -1 : 1);
      } else if( button == SEQ_UI_BUTTON_GP3 ) {
	u8 trg = SEQ_TRG_AccentGet(visible_track, ui_selected_step, ui_selected_instrument);
	return Encoder_Handler(button, trg ? -1 : 1);
      } else if( button == SEQ_UI_BUTTON_GP4 ) {
	u8 trg = SEQ_TRG_GlideGet(visible_track, ui_selected_step, ui_selected_instrument);
	return Encoder_Handler(button, trg ? -1 : 1);
      } else if( button <= SEQ_UI_BUTTON_GP16 ) {
	return Encoder_Handler(button, 0);
      }
    }


    if( event_mode != SEQ_EVENT_MODE_Drum &&
      (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_LAYERS || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG) ) {

      if( button == SEQ_UI_BUTTON_GP1 ) {
	int next_step = ui_selected_step + 1; // (required, since ui_selected_step is only u8, but we could have up to 256 steps)
	if( next_step >= (SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1) )
	  next_step = 0;
	ui_selected_step = next_step;
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed
      } else if( button == SEQ_UI_BUTTON_GP2 ||
		 (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG && button <= SEQ_UI_BUTTON_GP8) ) {
	u8 trg = SEQ_TRG_Get(visible_track, ui_selected_step, (u8)button-1, ui_selected_instrument);
	return Encoder_Handler(button, trg ? -1 : 1);
      } else if( button <= SEQ_UI_BUTTON_GP16 ) {
	return Encoder_Handler(button, 0);
      }
    }

    ui_selected_step = button + ui_selected_step_view*16;

    // toggle trigger layer
    // if seq_hwcfg_button_beh.all_with_triggers set, we've three cases:
    // a) ALL function active, but ALL button not pressed: invert complete trigger layer
    // b) ALL function active and ALL button pressed: toggle step, set remaining steps to same new value
    // c) ALL function not active: toggle step
    if( seq_hwcfg_button_beh.all_with_triggers && seq_ui_button_state.CHANGE_ALL_STEPS ) {
      if( seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE ) {
	// b) ALL function active and ALL button pressed: toggle step, set remaining steps to same new value
	u16 step = ui_selected_step;
	u8 new_value = SEQ_TRG_Get(visible_track, step, ui_selected_trg_layer, ui_selected_instrument) ? 0 : 1;
	
	u8 track;
	for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
	  if( SEQ_UI_IsSelectedTrack(track) ) {
	    u16 num_steps = SEQ_TRG_NumStepsGet(track);
	    for(step=0; step<num_steps; ++step)
	      SEQ_TRG_Set(track, step, ui_selected_trg_layer, ui_selected_instrument, new_value);
	  }
      } else {
	// a) ALL function active, but ALL button not pressed: invert complete trigger layer
	u8 track;
	u16 step;
	for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	  if( SEQ_UI_IsSelectedTrack(track) ) {
	    u16 num_steps = SEQ_TRG_NumStepsGet(track);
	    for(step=0; step<num_steps; ++step) {
	      u8 new_value = SEQ_TRG_Get(track, step, ui_selected_trg_layer, ui_selected_instrument) ? 0 : 1;
	      SEQ_TRG_Set(track, step, ui_selected_trg_layer, ui_selected_instrument, new_value);
	    }
	  }
	}
      }
    } else {
      // c) ALL function not active: toggle step
      u8 track;
      
      u8 new_value = SEQ_TRG_Get(visible_track, ui_selected_step, ui_selected_trg_layer, ui_selected_instrument) ? 0 : 1;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	if( SEQ_UI_IsSelectedTrack(track) ) {
	  SEQ_TRG_Set(track, ui_selected_step, ui_selected_trg_layer, ui_selected_instrument, new_value);
	}
      }
    }
    
    return 1; // value always changed

  } else {
    switch( button ) {
      case SEQ_UI_BUTTON_Select:
	// toggle MIDI learn
	if( !depressed )
	  midi_learn_mode = (midi_learn_mode == MIDI_LEARN_MODE_ON) ? MIDI_LEARN_MODE_OFF : MIDI_LEARN_MODE_ON;
	return 1; // value always changed

      case SEQ_UI_BUTTON_Right: {
	if( depressed ) return 0; // ignore when button depressed

	int next_step = ui_selected_step + 1; // (required, since ui_selected_step is only u8, but we could have up to 256 steps)
	if( next_step >= (SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1) )
	  next_step = 0;
	ui_selected_step = next_step;
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed
      } break;

      case SEQ_UI_BUTTON_Left:
	if( depressed ) return 0; // ignore when button depressed

	if( ui_selected_step == 0 )
	  ui_selected_step = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
	else
	  --ui_selected_step;

	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed

      case SEQ_UI_BUTTON_Up:
	if( depressed ) return 0; // ignore when button depressed
	return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

      case SEQ_UI_BUTTON_Down:
	if( depressed ) return 0; // ignore when button depressed
	return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

      // this button is currently only notified to EDIT page
      case SEQ_UI_BUTTON_Edit:
	if( !depressed )
	  PassiveEditEnter();
	else {
	  PassiveEditTakeOver();
	  edit_passive_mode = 0;
	}
	return 1;
    }
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Global Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
//     <edit_page>: selects the normal, or copy/paste/move/scroll view
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_LCD_Handler(u8 high_prio, seq_ui_edit_mode_t edit_mode)
{
  if( high_prio )
    return 0; // there are no high-priority updates


  // layout common track:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // G1T1 xxxxxxxxxxxxxxx  PC:Length TA:Gate Step  1   G#1_ Vel:127_Len: 75%    xxxxx
  // ....

  // layout drum track:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // G1T1 xxxxxxxxxxxxxxx  PA:Vel.   TA:Gate Step  1   G#1_ Vel:127_Len: 75%    xxxxx
  // ....

  // layout edit config
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // Step Trg  Layer 303                Step Datawheel:  Record   Random    Euclid   
  // View View View View               Select Scroll     Config  Generator Generator 

  // layout trigger view
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // Step Gate Acc. Roll Glide Skip R.G  R.V Note Vel. Len. Roll Note Note Note Note 
  //   1    *    o    o    o    o    o    o  C-3  100   75% ---- E-3  G-3  ---- ---- 

  // layout layer view
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // Step Gate Note Vel. Len. Roll Note Note Note Note Note Note Note Note Note Note 
  //   1    *  C-3  100   75% ---- E-3  G-3  ---- ---- ---- ---- ---- ---- ---- ---- 

  // layout 303 view
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // Step Gate Acc. Glide Oct, Key Vel. Prob  CC   CC   CC   CC   CC   CC   CC   CC  
  //   1    *   o     o    3    C  100  100%  64   64   64   64   64   64   64   64  

  // layout step selection:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  //        Select the steps which should be  controlled by the ALL function:        
  //   *    *    *    *    *    *    *    *    *    *    *    *    *    *    *    *  

  if( !edit_mode && seq_ui_button_state.EDIT_PRESSED ) {
    const char datawheel_mode_str[DATAWHEEL_MODE_NUM][11] = {
      " Cursor   ",
      " StepView ",
      " Value    ",
      " ParLayer ",
      " TrgLayer ",
    };

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("Step Trg  Layer 303                Step Datawheel:  Record   Random    Euclid   ");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("View View View View               Select");
    SEQ_LCD_PrintString((char *)datawheel_mode_str[datawheel_mode]);
    SEQ_LCD_PrintString("  Config  Generator Generator ");
    return 0; // no error
  }

  if( !edit_mode && seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
    int step;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("       Select the steps which should be  controlled by the ALL function:        ");
    SEQ_LCD_CursorSet(0, 1);
    for(step=0; step<16; ++step)
      SEQ_LCD_PrintFormattedString("  %c  ", (selected_steps & (1 << step)) ? '*' : 'o');
    return 0; // no error
  }


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);


  if( !edit_mode && event_mode != SEQ_EVENT_MODE_Drum &&
      (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_303) ) {
    // we want to show vertical bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

    u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
    // maximum 10 parameter layers
    if( num_p_layers >= 11 )
      num_p_layers = 11;

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("Step Gate Acc. Glide Oct. Key ");
    int i;
    for(i=1; i<num_p_layers; ++i)
	SEQ_LCD_PrintString((char *)SEQ_PAR_AssignedTypeStr(visible_track, i));

    SEQ_LCD_PrintSpaces(80 - (5*num_p_layers));

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintFormattedString((seq_record_state.ENABLED || midi_learn_mode == MIDI_LEARN_MODE_ON) ? "{%3d}" : " %3d ", ui_selected_step+1);
    SEQ_LCD_PrintFormattedString("  %c  ", SEQ_TRG_GateGet(visible_track, ui_selected_step, ui_selected_instrument) ? '*' : 'o');
    SEQ_LCD_PrintFormattedString("  %c  ", SEQ_TRG_AccentGet(visible_track, ui_selected_step, ui_selected_instrument) ? '*' : 'o');
    SEQ_LCD_PrintFormattedString("  %c  ", SEQ_TRG_GlideGet(visible_track, ui_selected_step, ui_selected_instrument) ? '*' : 'o');

    u8 note = SEQ_PAR_Get(visible_track, ui_selected_step, 0, ui_selected_instrument);
    u8 note_octave = note / 12;
    u8 note_key = note % 12;

    SEQ_LCD_PrintFormattedString(" %2d  ", (int)note_octave-2);
    const char note_tab[12][2] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };
    SEQ_LCD_PrintFormattedString("  %c%c ", note_tab[note_key][0], note_tab[note_key][1]);

    for(i=1; i<num_p_layers; ++i)
      if( i == ui_selected_par_layer && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(5);
      else {
	int print_edit_value = PassiveEditValid() ? edit_passive_value : -1;
	SEQ_LCD_PrintLayerEvent(visible_track, ui_selected_step, i, ui_selected_instrument, 0, print_edit_value);
	SEQ_LCD_PrintChar(' ');
      }

    SEQ_LCD_PrintSpaces(80 - (5*num_p_layers));

    return 0;
  }

  if( !edit_mode && event_mode != SEQ_EVENT_MODE_Drum &&
      (seq_ui_edit_view == SEQ_UI_EDIT_VIEW_LAYERS || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG) ) {

    // we want to show vertical bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

    u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
    u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_TRG ) {
      // maximum 7 parameter layers due to "Step" item!
      if( num_t_layers >= 7 )
	num_t_layers = 7;

      // maximum 8 parameter layers
      if( num_p_layers >= 8 )
	num_p_layers = 8;
    } else {
      // single trigger layer (gate)
      num_t_layers = 1;

      // maximum 14 parameter layers due to "Step" and "Gate" item!
      if( num_p_layers >= 14 )
	num_p_layers = 14;
    }

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("Step ");
    int i;
    for(i=0; i<num_t_layers; ++i)
	SEQ_LCD_PrintString(SEQ_TRG_AssignedTypeStr(visible_track, i));
    for(i=0; i<num_p_layers; ++i)
	SEQ_LCD_PrintString((char *)SEQ_PAR_AssignedTypeStr(visible_track, i));

    SEQ_LCD_PrintSpaces(80 - (5*num_p_layers));

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintFormattedString((seq_record_state.ENABLED || midi_learn_mode == MIDI_LEARN_MODE_ON) ? "{%3d}" : " %3d ", ui_selected_step+1);
    for(i=0; i<num_t_layers; ++i)
      SEQ_LCD_PrintFormattedString("  %c  ", SEQ_TRG_Get(visible_track, ui_selected_step, i, ui_selected_instrument) ? '*' : 'o');
    for(i=0; i<num_p_layers; ++i)
      if( i == ui_selected_par_layer && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(5);
      else {
	int print_edit_value = PassiveEditValid() ? edit_passive_value : -1;
	SEQ_LCD_PrintLayerEvent(visible_track, ui_selected_step, i, ui_selected_instrument, 0, print_edit_value);
	SEQ_LCD_PrintChar(' ');
      }

    SEQ_LCD_PrintSpaces(80 - (5*num_p_layers));

    return 0;
  }

  seq_layer_evnt_t layer_event;
  SEQ_LAYER_GetEvntOfLayer(visible_track, ui_selected_step, ui_selected_par_layer, ui_selected_instrument, &layer_event);

  seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(visible_track, ui_selected_par_layer);

  // TODO: tmp. solution to print chord velocity correctly
  if( layer_type == SEQ_PAR_Type_Velocity && (seq_cc_trk[visible_track].link_par_layer_chord == 0) )
    layer_type = SEQ_PAR_Type_Chord;


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  SEQ_LCD_PrintSpaces(1);


  if( ui_page == SEQ_UI_PAGE_EDIT && edit_passive_mode == 2 ) {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(15);
    } else {
      SEQ_LCD_PrintString("PASSIVE EDITING");
    }
  } else if( seq_record_state.ENABLED || edit_mode == SEQ_UI_EDIT_MODE_RECORD || midi_learn_mode == MIDI_LEARN_MODE_ON ) {
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(15);
    } else {
      if( midi_learn_mode == MIDI_LEARN_MODE_ON ) {
	SEQ_LCD_PrintString("EDIT RECORDING ");
      } else if( seq_record_options.STEP_RECORD ) {
	SEQ_LCD_PrintString("STEP RECORDING ");
      } else {
	SEQ_LCD_PrintString("LIVE RECORDING ");
      }
    }
  } else {
    switch( edit_mode ) {
    case SEQ_UI_EDIT_MODE_COPY: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	char str_buffer[10];
	sprintf(str_buffer, "%d-%d", SEQ_UI_UTIL_CopyPasteBeginGet()+1, SEQ_UI_UTIL_CopyPasteEndGet()+1);
	SEQ_LCD_PrintFormattedString("COPY S%-9s", str_buffer);
      }
    } break;

    case SEQ_UI_EDIT_MODE_PASTE: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintFormattedString("PASTE OFFS.%3d ", ui_selected_step+1);
      }
    } break;

    case SEQ_UI_EDIT_MODE_MOVE: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintString("MOVE STEPS     ");
      }
    } break;

    case SEQ_UI_EDIT_MODE_SCROLL: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintString("SCROLL TRACK   ");
      }
    } break;

    case SEQ_UI_EDIT_MODE_RANDOM: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintString("RANDOMIZED     ");
      }
    } break;

    case SEQ_UI_EDIT_MODE_MANUAL: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintString("MANUAL TRIGGER ");
      }
    } break;

    default: {
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintMIDIOutPort(SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT));
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintFormattedString("Chn.%2d  ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
      } else {
	SEQ_LCD_PrintTrackLabel(visible_track, (char *)seq_core_trk[visible_track].name);
      }
    }
    }
  }

  SEQ_LCD_PrintSpaces(2);

  SEQ_LCD_PrintChar('P');
  SEQ_LCD_PrintChar('A' + ui_selected_par_layer);
  SEQ_LCD_PrintChar(':');

  if( layer_type == SEQ_PAR_Type_CC ) {
    if( layer_event.midi_package.cc_number >= 0x80 ) {
      SEQ_LCD_PrintFormattedString("CC#off ");
    } else {
      SEQ_LCD_PrintFormattedString("CC#%3d ", layer_event.midi_package.cc_number);
    }
  } else {
    SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, ui_selected_par_layer));
    SEQ_LCD_PrintSpaces(2);
  }

  SEQ_LCD_PrintFormattedString("T%c:%s", 'A' + ui_selected_trg_layer, SEQ_TRG_AssignedTypeStr(visible_track, ui_selected_trg_layer));


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);

  SEQ_LCD_PrintFormattedString("Step%3d ", ui_selected_step+1);

  if( layer_event.midi_package.event == CC ) {
    mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
    u8 loopback = port == 0xf0;

    if( loopback )
      SEQ_LCD_PrintString((char *)SEQ_CC_LABELS_Get(port, layer_event.midi_package.cc_number));
    else {
      if( layer_event.midi_package.cc_number >= 0x80 ) {
	SEQ_LCD_PrintFormattedString("  CC#off");
      } else {
	SEQ_LCD_PrintFormattedString("  CC#%3d", layer_event.midi_package.cc_number);
      }
    }
    SEQ_LCD_PrintFormattedString(" %3d ", layer_event.midi_package.value);
    SEQ_LCD_PrintVBar(layer_event.midi_package.value >> 4);
  } else {
    SEQ_LCD_PrintSpaces(2);

    if( layer_event.midi_package.note && layer_event.midi_package.velocity && (layer_event.len >= 0) ) {
      if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator ) {
	u8 par_value = PassiveEditValid() ? edit_passive_value : layer_event.midi_package.note;
	SEQ_LCD_PrintArp(par_value);
      } else if( layer_type == SEQ_PAR_Type_Chord ) {
	u8 par_value = PassiveEditValid()
	  ? edit_passive_value
	  : SEQ_PAR_Get(visible_track, ui_selected_step, 0, ui_selected_instrument);

	u8 chord_ix = par_value & 0x1f;
	u8 chord_oct = par_value >> 5;
	SEQ_LCD_PrintString(SEQ_CHORD_NameGet(chord_ix));
	SEQ_LCD_PrintFormattedString("/%d", chord_oct);
      } else {
	u8 par_value = PassiveEditValid() ? edit_passive_value : layer_event.midi_package.note;
	SEQ_LCD_PrintNote(par_value);
      }
      SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
    }
    else {
      SEQ_LCD_PrintString("....");
    }
    SEQ_LCD_PrintFormattedString(" Vel:%3d", layer_event.midi_package.velocity);
  }

  SEQ_LCD_PrintString(" Len:");
  SEQ_LCD_PrintGatelength(layer_event.len);


  // print flashing *LOOPED* at right corner if loop mode activated to remind that steps will be played differntly
  if( (ui_cursor_flash_overrun_ctr & 1) && seq_core_state.LOOP ) {
    SEQ_LCD_PrintString(" *LOOPED*");
  } else if( (ui_cursor_flash_overrun_ctr & 1) && seq_core_trk[visible_track].play_section > 0 ) {
    SEQ_LCD_PrintFormattedString(" *Sect.%c*", 'A'+seq_core_trk[visible_track].play_section);
  } else {
    SEQ_LCD_PrintSpaces(4);
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
    } else {
      SEQ_LCD_PrintTrackCategory(visible_track, (char *)seq_core_trk[visible_track].name);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // Second Line
  ///////////////////////////////////////////////////////////////////////////

  u8 show_drum_triggers = (event_mode == SEQ_EVENT_MODE_Drum && (edit_mode || !ui_hold_msg_ctr));

  // extra handling for gatelength (shows vertical bars)
  if( !show_drum_triggers && layer_type == SEQ_PAR_Type_Length ) {

    // we want to show horizontal bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

    // initial cursor position
    SEQ_LCD_CursorSet(0, 1);

    // determine length of previous step (depends on selected view and track length)
    int previous_step = 16*ui_selected_step_view - 1;
    if( previous_step < 0 )
      previous_step = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);

    seq_layer_evnt_t layer_event;
    SEQ_LAYER_GetEvntOfLayer(visible_track, previous_step, ui_selected_par_layer, ui_selected_instrument, &layer_event);
    u16 previous_length = layer_event.len;

    // show length of 16 steps
    u16 step;
    for(step=0; step<16; ++step) {
      u16 visible_step = step + 16*ui_selected_step_view;
      SEQ_LAYER_GetEvntOfLayer(visible_track, visible_step, ui_selected_par_layer, ui_selected_instrument, &layer_event);

      u8 gate = SEQ_TRG_GateGet(visible_track, visible_step, ui_selected_instrument);

      // muted step? if previous gatelength <= 96, print spaces
      if( (!gate || !layer_event.midi_package.velocity) && previous_length < 96 ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	if( layer_event.len >= 96 )
	  SEQ_LCD_PrintHBar(15); // glide or stretched event
	else
	  SEQ_LCD_PrintHBar(((layer_event.len-1)*16)/100);
      }
      previous_length = ((gate && layer_event.midi_package.velocity) || (previous_length >= 96 && layer_event.len >= 96)) ? layer_event.len : 0;
    }

  } else {

    if( show_drum_triggers ) {
      // we want to show triggers
      SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsBig);
    } else {
      // we want to show vertical bars
      SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);
    }

    // initial cursor position
    SEQ_LCD_CursorSet(0, 1);

    int step_region_begin;
    int step_region_end;
    switch( edit_mode ) {
      case SEQ_UI_EDIT_MODE_COPY:
	step_region_begin = SEQ_UI_UTIL_CopyPasteBeginGet();
	step_region_end = SEQ_UI_UTIL_CopyPasteEndGet();
	break;
      case SEQ_UI_EDIT_MODE_PASTE:
	step_region_begin = ui_selected_step;
	step_region_end = ui_selected_step + SEQ_UI_UTIL_CopyPasteEndGet() - SEQ_UI_UTIL_CopyPasteBeginGet();
	break;
      case SEQ_UI_EDIT_MODE_SCROLL:
	step_region_begin = ui_selected_step;
	step_region_end = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
	break;
      default:
	step_region_begin = ui_selected_step;
	step_region_end = ui_selected_step;
    }

    u16 step;
    for(step=0; step<16; ++step) {
      u16 visible_step = step + 16*ui_selected_step_view;

      if( ui_cursor_flash && 
	  edit_mode != SEQ_UI_EDIT_MODE_NORMAL && 
	  visible_step >= step_region_begin && visible_step <= step_region_end ) {
	SEQ_LCD_PrintSpaces(5);
	continue;
      }

      if( show_drum_triggers ) {
	u8 gate_accent = SEQ_TRG_Get(visible_track, visible_step, 0, ui_selected_instrument);
	if( SEQ_TRG_NumLayersGet(visible_track) >= 2 )
	  gate_accent |= SEQ_TRG_Get(visible_track, visible_step, 1, ui_selected_instrument) << 1;

	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(gate_accent);
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(' ');
      } else {
	int print_edit_value = (visible_step == edit_passive_step && PassiveEditValid()) ? edit_passive_value : -1;
	SEQ_LCD_PrintLayerEvent(visible_track, visible_step, ui_selected_par_layer, ui_selected_instrument, 1, print_edit_value);
      }

      if( !show_drum_triggers ) {
	u8 midi_learn = seq_record_state.ENABLED || midi_learn_mode == MIDI_LEARN_MODE_ON;
	char lbr = midi_learn ? '}' : '<';
	char rbr = midi_learn ? '{' : '>';

	SEQ_LCD_PrintChar((visible_step == step_region_end) ? lbr 
			  : ((visible_step == (step_region_begin-1)) ? rbr : ' '));
      }
      
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_NORMAL);
}


/////////////////////////////////////////////////////////////////////////////
// MIDI IN
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_IN_Handler(mios32_midi_port_t port, mios32_midi_package_t p)
{
  if( midi_learn_mode == MIDI_LEARN_MODE_ON ) {
    u8 visible_track = SEQ_UI_VisibleTrackGet();

    // quick & dirty for evaluation purposes
    seq_record_options_t prev_seq_record_options = seq_record_options;

    seq_record_options.ALL = 0;
    seq_record_options.STEP_RECORD = 1;
    seq_record_options.FWD_MIDI = prev_seq_record_options.FWD_MIDI;

    seq_record_state.ENABLED = 1;
    seq_record_step = ui_selected_step;

    SEQ_RECORD_Receive(p, visible_track);

    if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
      // copy matching par layers into remaining steps
      u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
      u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);

      seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
      seq_par_layer_type_t rec_layer_type = tcc->lay_const[ui_selected_par_layer];

      {
	u8 p_layer;
	for(p_layer=0; p_layer<num_p_layers; ++p_layer) {
	  seq_par_layer_type_t layer_type = tcc->lay_const[p_layer];

	  if( layer_type == rec_layer_type ||
	      ((rec_layer_type == SEQ_PAR_Type_Note || rec_layer_type == SEQ_PAR_Type_Chord) && (layer_type == SEQ_PAR_Type_Velocity || layer_type == SEQ_PAR_Type_Length)) ) {
	    u8 value = SEQ_PAR_Get(visible_track, seq_record_step, p_layer, ui_selected_instrument);

	    u16 step;
	    for(step=0; step<num_steps; ++step) {
	      if( step != seq_record_step && (selected_steps & (1 << (step % 16))) ) {
		SEQ_PAR_Set(visible_track, step, p_layer, ui_selected_instrument, value);
	      }
	    }
	  }
	}
      }
    }

    seq_record_options.ALL = prev_seq_record_options.ALL;
    seq_record_state.ENABLED = 0;

    seq_ui_display_update_req = 1;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Exit
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  midi_learn_mode = MIDI_LEARN_MODE_OFF;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(SEQ_UI_EDIT_Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(SEQ_UI_EDIT_LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallMIDIINCallback(MIDI_IN_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  // disable MIDI learn mode by default
  midi_learn_mode = MIDI_LEARN_MODE_OFF;

  edit_passive_mode = 0;

  if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL )
    seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// help function to change/set a single encoder value
// if forced_value >= 0: new value will be set to the given value
// if forced_value < 0: new value will be changed via incrementer
// returns >= 0 if new value has been set (value change)
// returns < 0 if no change
/////////////////////////////////////////////////////////////////////////////
static s32 ChangeSingleEncValue(u8 track, u16 par_step, u16 trg_step, s32 incrementer, s32 forced_value, u8 change_gate, u8 dont_change_gate)
{
  seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(track, ui_selected_par_layer);
  u8 visible_track = SEQ_UI_VisibleTrackGet();

#if 0
  // disabled in MBSEQ V4.0beta15 due to issue reported by Gridracer:
  // http://midibox.org/forums/index.php?/topic/13137-midibox-seq-v4-beta-release-feedback/page__st__100

  // if note/chord/velocity parameter: only change gate if requested
  if( (layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord || layer_type == SEQ_PAR_Type_Velocity) &&
      !change_gate &&
      !SEQ_TRG_GateGet(track, trg_step, ui_selected_instrument) )
    return -1;
#endif

  if( layer_type == SEQ_PAR_Type_Probability ) {
    // due to another issue reported by Gridracer:
    // invert incrementer so that clockwise move increases probability
    incrementer = -incrementer;
  }


  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    ui_hold_msg_ctr = 1000; // show value for 1 second
  }

  s32 old_value = SEQ_PAR_Get(track, par_step, ui_selected_par_layer, ui_selected_instrument);
  s32 new_value = (forced_value >= 0) ? forced_value : (old_value + incrementer);
  if( new_value < 0 )
    new_value = 0;
  else if( new_value >= 128 )
    new_value = 127;
	    
  // extra for more comfortable editing of multi-note tracks:
  // if assigned parameter layer is Note or Chord, and currently 0, re-start at C-3 resp. A/2
  // when value is incremented
  if( incrementer > 0 && forced_value < 0 && old_value == 0x00 && (layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord) )
    new_value = (layer_type == SEQ_PAR_Type_Note && SEQ_CC_Get(track, SEQ_CC_MODE) != SEQ_CORE_TRKMODE_Arpeggiator) ? 0x3c : 0x40;

  if( !dont_change_gate ) {
    u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);

    // we do this always regardless if value has been changed or not (e.g. increment if value already 127)
    if( event_mode == SEQ_EVENT_MODE_CC && layer_type == SEQ_PAR_Type_CC ) {
      // in this mode gates are used to disable CC
      // if a CC value has been changed, set gate
      SEQ_TRG_GateSet(track, trg_step, ui_selected_instrument, 1);
    }
  }

  // take over if changed
  if( new_value == old_value )
    return -1;

  SEQ_PAR_Set(track, par_step, ui_selected_par_layer, ui_selected_instrument, (u8)new_value);

  if( !dont_change_gate &&
      (layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord || layer_type == SEQ_PAR_Type_Velocity) ) {
    // (de)activate gate depending on value
    if( new_value )
      SEQ_TRG_GateSet(track, trg_step, ui_selected_instrument, 1);
    else {
      // due to another issue reported by Gridracer:
      // if the track plays multiple notes, only clear gate if all notes are 0
      u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
      u8 allNotesZero = 1;
      int i;
      for(i=0; i<num_p_layers; ++i) {
	seq_par_layer_type_t localLayerType = SEQ_PAR_AssignmentGet(track, i);
	if( (localLayerType == SEQ_PAR_Type_Note || localLayerType == SEQ_PAR_Type_Chord) &&
	    SEQ_PAR_Get(track, par_step, i, ui_selected_instrument) > 0 ) {
	  allNotesZero = 0;
	  break;
	}
      }

      if( allNotesZero )
	SEQ_TRG_GateSet(track, trg_step, ui_selected_instrument, 0);
    }
  }

  return new_value;
}


/////////////////////////////////////////////////////////////////////////////
// help functions for "passive edit mode"
/////////////////////////////////////////////////////////////////////////////
static s32 PassiveEditEnter(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(visible_track, ui_selected_par_layer);

  // passive edit mode currently only supported for notes/chords

  if( layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord ) {
    // enter passive edit mode and store track/step/layer/instrument for later checks
    edit_passive_mode = 1;
    edit_passive_track = visible_track;
    edit_passive_step = ui_selected_step;
    edit_passive_par_layer = ui_selected_par_layer;
    edit_passive_instrument = ui_selected_instrument;
    edit_passive_value = SEQ_PAR_Get(edit_passive_track, edit_passive_step, edit_passive_par_layer, edit_passive_instrument);
  } else {
    edit_passive_mode = 0;
  }

  return 0; // no error
}

static s32 PassiveEditValid(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  return (edit_passive_mode == 2 && // if mode is 2, the value has been changed!
	  edit_passive_track == visible_track &&
	  edit_passive_step == ui_selected_step &&
	  edit_passive_par_layer == ui_selected_par_layer &&
	  edit_passive_instrument == ui_selected_instrument) ? 1 : 0;
}

static s32 PassiveEditTakeOver(void)
{

  // only take over changed value if track/step/layer/instrument still passing
  if( PassiveEditValid() ) {
    // take over change
    // handle it like a single increment/decrement so that no code needs to be duplicated
    int current_value = SEQ_PAR_Get(edit_passive_track, edit_passive_step, edit_passive_par_layer, edit_passive_instrument);
    int incrementer = (int)edit_passive_value - current_value;

    seq_ui_button_state.EDIT_PRESSED = 0; // just to avoid any overlay...
    edit_passive_mode = 0; // to avoid recursion in encoder handler
    Encoder_Handler(edit_passive_step % 16, incrementer);
  } else {
    edit_passive_mode = 1;
  }

  return 0; // no error
}
