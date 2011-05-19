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
#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////
seq_ui_edit_view_t seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS;


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 show_edit_config_page;

static u16 selected_steps = 0xffff; // will only be initialized once after startup

static u16 hq_ix;
static u16 hq_pos;
static u16 hq_timestamp_step;
static u16 hq_timestamp_tick;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 ChangeSingleEncValue(u8 track, u16 par_step, u16 trg_step, s32 incrementer, s32 forced_value, u8 change_gate, u8 dont_change_gate);
static s32 HQ_StepUpdate(void);


/////////////////////////////////////////////////////////////////////////////
// LED handler function (globally accessible, since it's re-used by UTIL page)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  
  if( show_edit_config_page ) {
    switch( seq_ui_edit_view ) {
    case SEQ_UI_EDIT_VIEW_STEPS: *gp_leds = (1 << 0); break;
    case SEQ_UI_EDIT_VIEW_TRG: *gp_leds = (1 << 1); break;
    case SEQ_UI_EDIT_VIEW_LAYERS: *gp_leds = (1 << 2); break;
    case SEQ_UI_EDIT_VIEW_303: *gp_leds = (1 << 3); break;
    case SEQ_UI_EDIT_VIEW_STEPSEL: *gp_leds = (1 << 8); break;
    }
  } else {

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) || encoder == SEQ_UI_ENCODER_Datawheel ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 || encoder == SEQ_UI_ENCODER_Datawheel ) {
#endif

    if( show_edit_config_page ) {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP1: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS; break;
      case SEQ_UI_ENCODER_GP2: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_TRG; break;
      case SEQ_UI_ENCODER_GP3: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_LAYERS; break;
      case SEQ_UI_ENCODER_GP4: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_303; break;
      case SEQ_UI_ENCODER_GP9: seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPSEL; break;
      }
      show_edit_config_page = 0; // switch back to view
      return 1; // value changed
    }

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
      if( incrementer > 0 )
	selected_steps |= (1 << encoder);
      else
	selected_steps &= ~(1 << encoder);
      return 1; // value changed
    }

    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

    // special handling in HQ mode
    if( event_mode == SEQ_EVENT_MODE_HQ ) {
      if( encoder == SEQ_UI_ENCODER_GP9 || encoder == SEQ_UI_ENCODER_Datawheel ) {
	if( SEQ_UI_Var16_Inc(&hq_pos, 0, seq_par_max_hq_bytes-1, incrementer) >= 1 ) {
	  HQ_StepUpdate();
	  return 1;
	} else
	  return 0;
      }

      // TODO
      return 0;
    }

    if( event_mode != SEQ_EVENT_MODE_Drum &&
	(seq_ui_edit_view == SEQ_UI_EDIT_VIEW_303) ) {
      u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

      if( encoder == SEQ_UI_ENCODER_GP1 ) {
	if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, num_steps, incrementer) >= 1 )
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
	if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, num_steps, incrementer) >= 1 )
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

    if( event_mode == SEQ_EVENT_MODE_Drum || seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPS )
      ui_selected_step = ((encoder == SEQ_UI_ENCODER_Datawheel) ? (ui_selected_step%16) : encoder) + ui_selected_step_view*16;

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
    if( seq_ui_button_state.CHANGE_ALL_STEPS && seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE ) {
      u16 num_steps = SEQ_PAR_NumStepsGet(visible_track);
      u16 par_step = ui_selected_step;
      u16 trg_step = ui_selected_step;

      // mirrored layer in drum mode?
      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum && par_step >= num_steps )
	par_step %= num_steps;

      forced_value = ChangeSingleEncValue(visible_track, par_step, trg_step, incrementer, forced_value, change_gate, 0);
      if( forced_value < 0 )
	return 0; // no change
      value_changed |= 1;
    }

    // change value of all selected steps
    u8 track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( SEQ_UI_IsSelectedTrack(track) ) {
	u16 num_steps = SEQ_PAR_NumStepsGet(track);
	u16 trg_step = (ui_selected_step & ~(num_steps-1));
	
	u16 par_step;
	for(par_step=0; par_step<num_steps; ++par_step, ++trg_step) {
	  if( !seq_ui_button_state.CHANGE_ALL_STEPS || par_step == ui_selected_step || (selected_steps & (1 << (par_step % 16))) ) {
	    change_gate = trg_step == ui_selected_step;
	    u8 dont_change_gate = par_step != ui_selected_step;
	    if( change_gate || seq_ui_button_state.CHANGE_ALL_STEPS ) {
	      if( ChangeSingleEncValue(track, par_step, trg_step, incrementer, forced_value, change_gate, dont_change_gate) >= 0 )
		value_changed |= 1;
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
    if( depressed ) return 0; // ignore when button depressed

    if( show_edit_config_page )
      return Encoder_Handler(button, 0);

    if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL ) {
      selected_steps ^= (1 << button);
      return 1; // value changed
    }

    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

    // special handling in HQ mode
    if( event_mode == SEQ_EVENT_MODE_HQ ) {
      switch( button ) {
      case SEQ_UI_BUTTON_Select:
	show_edit_config_page = depressed ? 0 : 1;
	return 1; // value always changed

      case SEQ_UI_BUTTON_Right: {
	if( depressed ) return 0; // ignore when button depressed

	int next_pos = hq_pos + 1;
	if( next_pos >= seq_par_max_hq_bytes )
	  next_pos = 0;
	hq_pos = next_pos;
	HQ_StepUpdate();
	return 1; // value always changed
      } break;

      case SEQ_UI_BUTTON_Left:
	if( depressed ) return 0; // ignore when button depressed

	if( hq_pos == 0 )
	  hq_pos = seq_par_max_hq_bytes-1;
	else
	  --hq_pos;
	HQ_StepUpdate();
	return 1; // value always changed

      case SEQ_UI_BUTTON_Up:
	if( depressed ) return 0; // ignore when button depressed
	return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

      case SEQ_UI_BUTTON_Down:
	if( depressed ) return 0; // ignore when button depressed
	return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
      }

      // TODO
      return 0;
    }

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
	show_edit_config_page = depressed ? 0 : 1;
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

  // layout HQ track:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // G1T1 xxxxxxxxxxxxxxx  PA:HQ     TA:Gate Pos. Timestamp  Event              xxxxx
  //                                           1    001.00   G#1_  Vel:127  Len:  96 

  // layout edit config
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // Step Trg  Layer 303                     Step                                    
  // View View View View                     Select                                  

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

  if( show_edit_config_page ) {
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("Step Trg  Layer 303                     Step                                    ");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("View View View View                     Select                                  ");
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


  if( !edit_mode && event_mode != SEQ_EVENT_MODE_Drum && event_mode != SEQ_EVENT_MODE_HQ &&
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
    SEQ_LCD_PrintFormattedString("%3d  ", ui_selected_step+1);
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
	SEQ_LCD_PrintLayerEvent(visible_track, ui_selected_step, i, ui_selected_instrument, 0);
	SEQ_LCD_PrintChar(' ');
      }

    SEQ_LCD_PrintSpaces(80 - (5*num_p_layers));

    return 0;
  }

  if( !edit_mode && event_mode != SEQ_EVENT_MODE_Drum && event_mode != SEQ_EVENT_MODE_HQ &&
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
    SEQ_LCD_PrintFormattedString("%3d  ", ui_selected_step+1);
    for(i=0; i<num_t_layers; ++i)
      SEQ_LCD_PrintFormattedString("  %c  ", SEQ_TRG_Get(visible_track, ui_selected_step, i, ui_selected_instrument) ? '*' : 'o');
    for(i=0; i<num_p_layers; ++i)
      if( i == ui_selected_par_layer && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(5);
      else {
	SEQ_LCD_PrintLayerEvent(visible_track, ui_selected_step, i, ui_selected_instrument, 0);
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

    case SEQ_UI_EDIT_MODE_RECORD: {
      if( ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(15);
      } else {
	SEQ_LCD_PrintString("RECORDING      ");
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

  SEQ_LCD_PrintSpaces(2);

  SEQ_LCD_PrintChar('P');
  SEQ_LCD_PrintChar('A' + ui_selected_par_layer);
  SEQ_LCD_PrintChar(':');

  if( event_mode == SEQ_EVENT_MODE_HQ ) {
    SEQ_LCD_PrintString("HQ     ");
  } else if( layer_type == SEQ_PAR_Type_CC ) {
    SEQ_LCD_PrintFormattedString("CC#%3d ", layer_event.midi_package.cc_number);
  } else {
    SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, ui_selected_par_layer));
    SEQ_LCD_PrintSpaces(2);
  }

  SEQ_LCD_PrintFormattedString("T%c:%s", 'A' + ui_selected_trg_layer, SEQ_TRG_AssignedTypeStr(visible_track, ui_selected_trg_layer));


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);

  if( event_mode == SEQ_EVENT_MODE_HQ ) {
    SEQ_LCD_PrintString("Pos.  Timestamp Event          ");
  } else {
    SEQ_LCD_PrintFormattedString("Step%3d ", ui_selected_step+1);

    if( layer_event.midi_package.event == CC ) {
      mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
      u8 loopback = port == 0xf0;

      if( loopback )
	SEQ_LCD_PrintString((char *)SEQ_CC_LABELS_Get(port, layer_event.midi_package.cc_number));
      else
	SEQ_LCD_PrintFormattedString("  CC#%3d", layer_event.midi_package.cc_number);
      SEQ_LCD_PrintFormattedString(" %3d ", layer_event.midi_package.value);
      SEQ_LCD_PrintVBar(layer_event.midi_package.value >> 4);
    } else {
      SEQ_LCD_PrintSpaces(2);

      if( layer_event.midi_package.note && layer_event.midi_package.velocity && (layer_event.len >= 0) ) {
	if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator ) {
	  SEQ_LCD_PrintArp(layer_event.midi_package.note);
	} else if( layer_type == SEQ_PAR_Type_Chord ) {
	  u8 par_value = SEQ_PAR_Get(visible_track, ui_selected_step, 0, ui_selected_instrument);
	  u8 chord_ix = par_value & 0x1f;
	  u8 chord_oct = par_value >> 5;
	  SEQ_LCD_PrintString(SEQ_CHORD_NameGet(chord_ix));
	  SEQ_LCD_PrintFormattedString("/%d", chord_oct);
	} else {
	  SEQ_LCD_PrintNote(layer_event.midi_package.note);
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
  }


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

  if( event_mode == SEQ_EVENT_MODE_HQ ) {
    // we want to show vertical bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

    // initial cursor position
    SEQ_LCD_CursorSet(0, 1);

    SEQ_LCD_PrintSpaces(40); // currently empty

    SEQ_LCD_PrintFormattedString("%3d    %03d.%02d   ",
				 hq_pos+1,
				 hq_timestamp_step+1,
				 hq_timestamp_tick+1);

    if( hq_ix == 0xffff ) {
      SEQ_LCD_PrintString("----  -------   ------- ");
    } else {
      u8 *layer_ptr = (u8 *)&seq_par_layer_value[visible_track][hq_ix];
      u8 b0 = *layer_ptr++;
      u8 b1 = *layer_ptr++;
      mios32_midi_event_t event_type = b0 >> 4;

      switch( event_type ) {
      case NoteOff:
      case NoteOn: {
	u8 note = b1;
	u8 velocity = *layer_ptr++;
	u8 l0 = *layer_ptr++;
	u8 l1 = *layer_ptr++;
	u16 length = l0 | ((u16)l1 << 8);

	if( note && velocity && length > 0 ) {
	  if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator ) {
	    SEQ_LCD_PrintArp(note);
	  } else {
	    SEQ_LCD_PrintNote(note);
	  }
	  SEQ_LCD_PrintVBar(velocity >> 4);
	}
	else {
	  SEQ_LCD_PrintString("....");
	}
	SEQ_LCD_PrintFormattedString("  Vel:%3d", velocity);
	
	SEQ_LCD_PrintFormattedString("  Len:%4d ", length);
      } break;
	
      case PolyPressure: {
	u8 note = b1;
	u8 value = *layer_ptr++;

	SEQ_LCD_PrintFormattedString("A.Touch: ");
	SEQ_LCD_PrintNote(note);

	SEQ_LCD_PrintFormattedString("    Val:%3d", value);
	SEQ_LCD_PrintVBar(value >> 4);
      } break;

      case CC: {
	u8 cc_number = b1;
	u8 value = *layer_ptr++;
	mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
	u8 loopback = port == 0xf0;

	if( loopback )
	  SEQ_LCD_PrintString((char *)SEQ_CC_LABELS_Get(port, cc_number));
	else
	  SEQ_LCD_PrintFormattedString("CC#%3d", cc_number);
	SEQ_LCD_PrintFormattedString("  %3d ", value);
	SEQ_LCD_PrintVBar(value >> 4);
	
	SEQ_LCD_PrintSpaces(11);
      } break;
	
      case PitchBend: {
	u8 lsb = b1;
	u8 msb = *layer_ptr++;
	u16 pitch = (msb << 7) | lsb;

	SEQ_LCD_PrintFormattedString("Pitch: %5d", pitch);
	SEQ_LCD_PrintVBar(msb >> 4);
	
	SEQ_LCD_PrintSpaces(11);
	SEQ_LCD_PrintChar('x');
      } break;
	
      case ProgramChange: {
	u8 pc = b1;

	SEQ_LCD_PrintFormattedString("P.Chng: %3d", pc);
	SEQ_LCD_PrintVBar(pc >> 4);

	SEQ_LCD_PrintSpaces(12);
      } break;

      case Aftertouch: {
	u8 value = b1;

	SEQ_LCD_PrintFormattedString("A.Touch: %3d", value);
	SEQ_LCD_PrintVBar(value >> 4);

	SEQ_LCD_PrintSpaces(11);
      } break;

      default: { // 2 bytes, e.g. MIDI clock? ;)
	SEQ_LCD_PrintFormattedString("0x%02x 0x%02x", b0, b1);
	SEQ_LCD_PrintSpaces(15);
      }
      }
    }

    return 0; // no error
  }


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
	SEQ_LCD_PrintLayerEvent(visible_track, visible_step, ui_selected_par_layer, ui_selected_instrument, 1);
      }

      if( !show_drum_triggers ) {
	SEQ_LCD_PrintChar((visible_step == step_region_end) ? '<' 
			  : ((visible_step == (step_region_begin-1)) ? '>' : ' '));
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
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(SEQ_UI_EDIT_Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(SEQ_UI_EDIT_LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  show_edit_config_page = 0;

  if( seq_ui_edit_view == SEQ_UI_EDIT_VIEW_STEPSEL )
    seq_ui_edit_view = SEQ_UI_EDIT_VIEW_STEPS;

  HQ_StepUpdate();

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


static s32 HQ_StepUpdate(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  hq_ix = 0;

  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  if( event_mode != SEQ_EVENT_MODE_HQ )
    return -1;

  u8 *layer_ptr = (u8 *)&seq_par_layer_value[visible_track];
  int pos = 0;
  u16 tick = 0;
  while( hq_pos > pos || (layer_ptr[1] & 0x80) ) {
    // get first bytes
    u8 b0 = *layer_ptr++;
    u8 b1 = *layer_ptr++;
    hq_ix += 2;

    // check for end marker
    if( hq_ix >= seq_par_max_hq_bytes || (b0 == 0xff && b1 == 0xff) ) {
      hq_ix = 0xffff;
      return 0;
    } else if( b1 & 0x80 ) { // delay marker?
      tick += b0 | ((b1 & 0x7f) << 8);
    } else {
      // we got a MIDI event
      mios32_midi_event_t event_type = b0 >> 4;
      switch( event_type ) {
      case NoteOff:
      case NoteOn: {
	layer_ptr += 3;
	hq_ix += 3;
	++pos;
      } break;

      case PolyPressure:
      case CC:
      case PitchBend: {
	layer_ptr += 1;
	hq_ix += 1;
	++pos;
      } break;

      case ProgramChange:
      case Aftertouch: {
	++pos;
      } break;
      default: { // 2 byte in b0/b1, e.g. for MIDI clock? ;)
	++pos;
      }
      }
    }
  }

  seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];
  u16 step_length = ((tcc->clkdiv.value+1) * (tcc->clkdiv.TRIPLETS ? 4 : 6));

  hq_timestamp_step = tick / step_length;
  hq_timestamp_tick = tick % step_length;

  return 0;
}
