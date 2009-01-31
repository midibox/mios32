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
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 ChangeSingleEncValue(u8 track, u16 step, s32 incrementer, s32 forced_value, u8 change_gate);


/////////////////////////////////////////////////////////////////////////////
// LED handler function (globally accessible, since it's re-used by UTIL page)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_LED_Handler(u16 *gp_leds)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  *gp_leds =
    (SEQ_TRG_Get8(visible_track, 2*ui_selected_step_view+1, ui_selected_trg_layer, ui_selected_instrument) << 8) |
    (SEQ_TRG_Get8(visible_track, 2*ui_selected_step_view+0, ui_selected_trg_layer, ui_selected_instrument) << 0);

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
    ui_selected_step = ((encoder == SEQ_UI_ENCODER_Datawheel) ? (ui_selected_step%16) : encoder) + ui_selected_step_view*16;

    u8 visible_track = SEQ_UI_VisibleTrackGet();

    s32 value_changed = 0;
    s32 forced_value = -1;
    u8  change_gate = 1;

    // due to historical reasons (from old times where MBSEQ CS was stuffed with pots): 
    // in arp mode, we increment in steps of 4
    if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator &&
	SEQ_PAR_AssignmentGet(visible_track, ui_selected_par_layer) == SEQ_PAR_Type_Note )
      incrementer *= 4;


    // first change the selected value
    if( seq_ui_button_state.CHANGE_ALL_STEPS && seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE ) {
      forced_value = ChangeSingleEncValue(visible_track, ui_selected_step, incrementer, forced_value, change_gate);
      if( forced_value < 0 )
	return 0; // no change
      value_changed |= 1;
    }

    // change value of all selected steps
    u8 track;
    u16 step;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( SEQ_UI_IsSelectedTrack(track) ) {
	u16 num_steps = SEQ_PAR_NumStepsGet(track);
	for(step=0; step<num_steps; ++step) {
	  change_gate = step == ui_selected_step;
	  if( change_gate || seq_ui_button_state.CHANGE_ALL_STEPS ) {
	    if( ChangeSingleEncValue(track, step, incrementer, forced_value, change_gate) >= 0 )
	      value_changed |= 1;
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
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

  u8 visible_track = SEQ_UI_VisibleTrackGet();

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    ui_selected_step = button + ui_selected_step_view*16;
    // toggle trigger layer
    // we've three cases:
    // a) ALL function active, but ALL button not pressed: invert complete trigger layer
    // b) ALL function active and ALL button pressed: toggle step, set remaining steps to same new value
    // c) ALL function not active: toggle step
    if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
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
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	if( SEQ_UI_IsSelectedTrack(track) ) {
	  u8 new_value = SEQ_TRG_Get(track, ui_selected_step, ui_selected_trg_layer, ui_selected_instrument) ? 0 : 1;
	  SEQ_TRG_Set(track, ui_selected_step, ui_selected_trg_layer, ui_selected_instrument, new_value);
	}
      }
    }
    return 1; // value always changed

  } else {
    switch( button ) {
      case SEQ_UI_BUTTON_Select:
      case SEQ_UI_BUTTON_Right: {
	int next_step = ui_selected_step + 1; // (required, since ui_selected_step is only u8, but we could have up to 256 steps)
	if( next_step >= (SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1) )
	  next_step = 0;
	ui_selected_step = next_step;
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed
      } break;

      case SEQ_UI_BUTTON_Left:
	if( ui_selected_step == 0 )
	  ui_selected_step = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed

      case SEQ_UI_BUTTON_Up:
	return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

      case SEQ_UI_BUTTON_Down:
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


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  seq_layer_evnt_t layer_event;
  SEQ_LAYER_GetEvntOfLayer(visible_track, ui_selected_step, ui_selected_par_layer, &layer_event);

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
	SEQ_LCD_PrintString("RECORDING MODE ");
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

  if( layer_type == SEQ_PAR_Type_CC ) {
    SEQ_LCD_PrintFormattedString("CC#%3d ", layer_event.midi_package.cc_number);
  } else {
    SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, ui_selected_par_layer));
    SEQ_LCD_PrintSpaces(2);
  }

  SEQ_LCD_PrintFormattedString("T%c:%s", 'A' + ui_selected_trg_layer, SEQ_TRG_AssignedTypeStr(visible_track, ui_selected_trg_layer));


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);

  SEQ_LCD_PrintFormattedString("Step%3d   ", ui_selected_step+1);

  if( layer_event.midi_package.event == CC ) {
    SEQ_LCD_PrintFormattedString("CC#%3d %3d",
				    layer_event.midi_package.cc_number,
				    layer_event.midi_package.value);
    SEQ_LCD_PrintVBar(layer_event.midi_package.value >> 4);
  } else {
    if( layer_event.midi_package.note && layer_event.midi_package.velocity && (layer_event.len >= 0) ) {
      if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator ) {
	SEQ_LCD_PrintArp(layer_event.midi_package.note);
      } else if( layer_type == SEQ_PAR_Type_Chord ) {
	u8 par_value = SEQ_PAR_Get(visible_track, ui_selected_step, 0, ui_selected_instrument);
	u8 chord_ix = par_value & 0x0f;
	u8 chord_oct = par_value >> 4;
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
  SEQ_LCD_PrintSpaces(4);

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
  } else {
    SEQ_LCD_PrintTrackCategory(visible_track, (char *)seq_core_trk[visible_track].name);
  }

  ///////////////////////////////////////////////////////////////////////////

  // extra handling for gatelength (shows vertical bars)
  if( layer_type == SEQ_PAR_Type_Length ) {

    // we want to show horizontal bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

    // initial cursor position
    SEQ_LCD_CursorSet(0, 1);

    // determine length of previous step (depends on selected view and track length)
    int previous_step = 16*ui_selected_step_view - 1;
    if( previous_step < 0 )
      previous_step = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);

    seq_layer_evnt_t layer_event;
    SEQ_LAYER_GetEvntOfLayer(visible_track, previous_step, ui_selected_par_layer, &layer_event);
    u16 previous_length = layer_event.len;

    u8 previous_step_was_long = 0; // hard to determine here... TODO

    // show length of 16 steps
    u8 step;
    for(step=0; step<16; ++step) {
      u8 visible_step = step + 16*ui_selected_step_view;
      SEQ_LAYER_GetEvntOfLayer(visible_track, visible_step, ui_selected_par_layer, &layer_event);

      if( previous_step_was_long ) {
	// continued long event
	// previous step took 96 steps
	previous_length -= 96;
	// print warning (!!!) if current step activated and overlapped by long event
	if( previous_step_was_long && !layer_event.midi_package.velocity )
	  SEQ_LCD_PrintString(">>>> ");
	else
	  SEQ_LCD_PrintString("!!!! ");
	// still long event if > 96
	previous_step_was_long = previous_length > 96;
      } else {
	previous_step_was_long = 0;
	// muted step? if previous gatelength <= 96, print spaces
	if( !layer_event.midi_package.velocity && previous_length <= 96 ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  if( layer_event.len >= 96 )
	    SEQ_LCD_PrintHBar(15); // glide
	  else
	    SEQ_LCD_PrintHBar(((layer_event.len-1)*16)/100);
	}
	previous_length = (layer_event.midi_package.velocity || (previous_length > 96 && layer_event.len > 96)) ? layer_event.len : 0;
      }
    }

#if 0
    u8 previous_step_was_long_multi = 0; // hard to determine here... TODO

    // show length of 16 steps
    u8 step;
    for(step=0; step<16; ++step) {
      u8 visible_step = step + 16*ui_selected_step_view;
      SEQ_LAYER_GetEvntOfLayer(visible_track, visible_step, ui_selected_par_layer, &layer_event);

      if( previous_step_was_long_multi || layer_event.len >= 32 ) { // multi note trigger?
	if( !previous_step_was_long_multi ) {
	  if( previous_length <= 24 && !layer_event.midi_package.velocity ) {
	    SEQ_LCD_PrintSpaces(5);
	    previous_length = 0;
	  } else {
	    SEQ_LCD_PrintGatelength(layer_event.len);
	    SEQ_LCD_PrintChar(layer_event.midi_package.velocity ? ' ' : '?'); // notify about multi-triggered events 
	    // with gate=0 and previous length >= 24 - we don't know how the core will react here

	    // calculate total length of events -> previous_length
	    previous_length = (layer_event.len>>5) * (layer_event.len & 0x1f);
	    // long event if >= 24
	    previous_step_was_long_multi = previous_length >= 24;
	  }
	} else {
	  // continued long event
	  // previous step took 24 steps
	  previous_length -= 24;
	  // print warning (!!!) if current step activated and overlapped by long multi trigger
	  if( previous_step_was_long_multi && !layer_event.midi_package.velocity )
	    SEQ_LCD_PrintString(">>>> ");
	  else
	    SEQ_LCD_PrintString("!!!! ");
	  // still long event if >= 24
	  previous_step_was_long_multi = previous_length >= 24;
	}
      } else {
	previous_step_was_long_multi = 0;
	// muted step? if previous gatelength <= 24, print spaces
	if( !layer_event.midi_package.velocity && previous_length <= 24 ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  if( layer_event.len >= 24 )
	    SEQ_LCD_PrintHBar(15); // glide
	  else
	    SEQ_LCD_PrintHBar((layer_event.len-1) >> 1);
	}
	previous_length = (layer_event.midi_package.velocity || (previous_length > 24 && layer_event.len > 24)) ? layer_event.len : 0;
      }
    }
#endif

  } else {

    if( event_mode == SEQ_EVENT_MODE_Drum ) {
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
	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  step_region_begin = -1;
	  step_region_end = -1;
	} else {
	  step_region_begin = ui_selected_step;
	  step_region_end = ui_selected_step;
	}
    }

    u8 step;
    for(step=0; step<16; ++step) {
      u8 visible_step = step + 16*ui_selected_step_view;

      if( ui_cursor_flash && 
	  edit_mode != SEQ_UI_EDIT_MODE_NORMAL && 
	  visible_step >= step_region_begin && visible_step <= step_region_end ) {
	SEQ_LCD_PrintSpaces(5);
	continue;
      }

      seq_layer_evnt_t layer_event;
      SEQ_LAYER_GetEvntOfLayer(visible_track, visible_step, ui_selected_par_layer, &layer_event);

      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	u8 gate_accent = SEQ_TRG_Get(visible_track, step + 16*ui_selected_step_view, 0, ui_selected_instrument);
	if( SEQ_TRG_NumLayersGet(visible_track) >= 2 )
	  gate_accent |= SEQ_TRG_Get(visible_track, step + 16*ui_selected_step_view, 1, ui_selected_instrument) << 1;
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(gate_accent);
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(' ');
      } else {
	switch( layer_type ) {
          case SEQ_PAR_Type_None:
	    SEQ_LCD_PrintString("None");
	    break;

          case SEQ_PAR_Type_Note:
          case SEQ_PAR_Type_Velocity:
	    if( layer_event.midi_package.note && layer_event.midi_package.velocity ) {
	      if( SEQ_CC_Get(visible_track, SEQ_CC_MODE) == SEQ_CORE_TRKMODE_Arpeggiator )
		SEQ_LCD_PrintArp(layer_event.midi_package.note);
	      else
		SEQ_LCD_PrintNote(layer_event.midi_package.note);
	      SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
	    } else {
	      SEQ_LCD_PrintString("----");
	    }
	    break;

	  case SEQ_PAR_Type_Chord:
	    if( layer_event.midi_package.note && layer_event.midi_package.velocity ) {
	      u8 par_value = SEQ_PAR_Get(visible_track, step, ui_selected_par_layer, ui_selected_instrument);
	      u8 chord_ix = par_value & 0x0f;
	      u8 chord_oct = par_value >> 4;
	      SEQ_LCD_PrintFormattedString("%X/%d", chord_ix, chord_oct);
	      SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
	    } else {
	      SEQ_LCD_PrintString("----");
	    }
	    break;
	  
	  //case SEQ_PAR_Type_Length:
	  //break;
	  // extra handling -- see code above

	  case SEQ_PAR_Type_CC:
	  case SEQ_PAR_Type_PitchBend: {
	    u8 value = layer_event.midi_package.value;
	    SEQ_LCD_PrintFormattedString("%3d", value);
	    SEQ_LCD_PrintVBar(value >> 4);
	  } break;

	  case SEQ_PAR_Type_Probability:
	    SEQ_LCD_PrintProbability(SEQ_PAR_ProbabilityGet(visible_track, step, ui_selected_instrument));
	    break;

	  case SEQ_PAR_Type_Delay:
	    SEQ_LCD_PrintStepDelay(SEQ_PAR_StepDelayGet(visible_track, step, ui_selected_instrument));
	    break;

	  case SEQ_PAR_Type_Roll:
	    SEQ_LCD_PrintRollMode(SEQ_PAR_RollModeGet(visible_track, step, ui_selected_instrument));
	    break;

          default:
	    SEQ_LCD_PrintString("????");
	    break;
	}
      }

      if( step_region_begin != -1 && step_region_end != -1 ) {
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
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(SEQ_UI_EDIT_LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// help function to change/set a single encoder value
// if forced_value >= 0: new value will be set to the given value
// if forced_value < 0: new value will be changed via incrementer
// returns >= 0 if new value has been set (value change)
// returns < 0 if no change
/////////////////////////////////////////////////////////////////////////////
static s32 ChangeSingleEncValue(u8 track, u16 step, s32 incrementer, s32 forced_value, u8 change_gate)
{
  seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(track, ui_selected_par_layer);

  // if note/chord/velocity parameter: only change gate if requested
  if( (layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord || layer_type == SEQ_PAR_Type_Velocity) &&
      !change_gate &&
      !SEQ_TRG_GateGet(track, step, ui_selected_instrument) )
    return -1;

  s32 old_value = SEQ_PAR_Get(track, step, ui_selected_par_layer, ui_selected_instrument);
  s32 new_value = (forced_value >= 0) ? forced_value : (old_value + incrementer);
  if( new_value < 0 )
    new_value = 0;
  else if( new_value >= 128 )
    new_value = 127;
	    
  // take over if changed
  if( new_value == old_value )
    return -1;

  SEQ_PAR_Set(track, step, ui_selected_par_layer, ui_selected_instrument, (u8)new_value);

  if( layer_type == SEQ_PAR_Type_Note || layer_type == SEQ_PAR_Type_Chord || layer_type == SEQ_PAR_Type_Velocity ) {
    // (de)activate gate depending on value
    if( new_value )
      SEQ_TRG_GateSet(track, step, ui_selected_instrument, 1);
    else
      SEQ_TRG_GateSet(track, step, ui_selected_instrument, 0);
  }

  return new_value;
}
