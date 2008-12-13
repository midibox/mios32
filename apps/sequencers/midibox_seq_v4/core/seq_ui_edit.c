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

#include "seq_midi.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_trg.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  *gp_leds =
    (trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+1] << 8) |
    trg_layer_value[visible_track][ui_selected_trg_layer][2*ui_selected_step_view+0];

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
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 step = ((encoder == SEQ_UI_ENCODER_Datawheel) ? (ui_selected_step%16) : encoder) + ui_selected_step_view*16;

    // select step
    ui_selected_step = step;

    // add to absolute value
    s32 old_value = SEQ_PAR_Get(visible_track, ui_selected_par_layer, step);
    s32 new_value = old_value + incrementer;
    if( new_value < 0 )
      new_value = 0;
    else if( new_value >= 128 )
      new_value = 127;

    // take over if changed
    if( new_value == old_value ) {
      return 0; // value not changed
    }

    SEQ_PAR_Set(visible_track, ui_selected_par_layer, step, (u8)new_value);

    if( SEQ_LAYER_GetVControlType(visible_track, ui_selected_par_layer) != SEQ_LAYER_ControlType_Length ) {
      // (de)activate gate depending on value
      if( new_value )
	trg_layer_value[visible_track][0][step>>3] |= (1 << (step&7));
      else
	trg_layer_value[visible_track][0][step>>3] &= ~(1 << (step&7));
    }

    return 1; // value changed
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

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // toggle trigger layer
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 step = button + ui_selected_step_view*16;
    trg_layer_value[visible_track][ui_selected_trg_layer][step>>3] ^= (1 << (step&7));
    return 1; // value always changed
  } else {
    switch( button ) {
      case SEQ_UI_BUTTON_Select:
      case SEQ_UI_BUTTON_Right:
	if( ++ui_selected_step >= SEQ_CORE_NUM_STEPS )
	  ui_selected_step = 0;
	ui_selected_step_view = ui_selected_step / 16;
	return 1; // value always changed

      case SEQ_UI_BUTTON_Left:
	if( ui_selected_step == 0 )
	  ui_selected_step = SEQ_CORE_NUM_STEPS-1;
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
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 step = ui_selected_step + ui_selected_step_view*16;

  seq_layer_evnt_t layer_event;
  SEQ_LAYER_GetEvntOfParLayer(visible_track, step, ui_selected_par_layer, &layer_event);


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  SEQ_LCD_PrintSpaces(2);

  MIOS32_LCD_PrintChar('A' + ui_selected_par_layer);
  MIOS32_LCD_PrintChar(':');

  switch( SEQ_LAYER_GetVControlType(visible_track, ui_selected_par_layer) ) {
    case SEQ_LAYER_ControlType_Note:
      MIOS32_LCD_PrintString("Note   ");
      break;

    case SEQ_LAYER_ControlType_Velocity:
      MIOS32_LCD_PrintString("Vel.   ");
      break;

    case SEQ_LAYER_ControlType_Chord1:
      MIOS32_LCD_PrintString("Chord1 ");
      break;

    case SEQ_LAYER_ControlType_Chord2:
      MIOS32_LCD_PrintString("Chord2 ");
      break;

    case SEQ_LAYER_ControlType_Length:
      MIOS32_LCD_PrintString("Length ");
      break;

    case SEQ_LAYER_ControlType_CC:
      MIOS32_LCD_PrintFormattedString("CC#%3d ", layer_event.midi_package.cc_number);
      break;

    default:
      MIOS32_LCD_PrintString("???    ");
      break;
  }

  MIOS32_LCD_PrintFormattedString("Chn%2d", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
  MIOS32_LCD_PrintChar('/');
  SEQ_LCD_PrintMIDIPort(SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT));
  SEQ_LCD_PrintSpaces(1);

  SEQ_LCD_PrintTrgLayer(ui_selected_trg_layer);

  SEQ_LCD_PrintStepView(ui_selected_step_view);


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_CursorSet(0, 0);

  MIOS32_LCD_PrintFormattedString("Step");
  SEQ_LCD_PrintSelectedStep(ui_selected_step, 15);
  MIOS32_LCD_PrintChar(':');

  if( layer_event.midi_package.event == CC ) {
    MIOS32_LCD_PrintFormattedString("CC#%3d %3d",
				    layer_event.midi_package.cc_number,
				    layer_event.midi_package.value);
    SEQ_LCD_PrintVBar(layer_event.midi_package.value >> 4);
  } else {
    if( layer_event.midi_package.note && layer_event.midi_package.velocity && layer_event.len ) {
      SEQ_LCD_PrintNote(layer_event.midi_package.note);
      SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
    }
    else {
      MIOS32_LCD_PrintString("....");
    }
    MIOS32_LCD_PrintFormattedString(" Vel:%3d", layer_event.midi_package.velocity);
  }

  MIOS32_LCD_PrintString(" Len:");
  SEQ_LCD_PrintGatelength(layer_event.len);
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintFormattedString("G-a-r--");

  ///////////////////////////////////////////////////////////////////////////
  seq_layer_ctrl_type_t layer_ctrl_type = SEQ_LAYER_GetVControlType(visible_track, ui_selected_par_layer);

  // extra handling for gatelength (shows vertical bars)
  if( layer_ctrl_type == SEQ_LAYER_ControlType_Length ) {

    // we want to show horizontal bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

    // initial cursor position
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_CursorSet(0, 1);

    // determine length of previous step (depends on selected view and track length)
    int previous_step = 16*ui_selected_step_view - 1;
    if( previous_step < 0 )
      previous_step = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);

    seq_layer_evnt_t layer_event;
    SEQ_LAYER_GetEvntOfParLayer(visible_track, previous_step, ui_selected_par_layer, &layer_event);
    u16 previous_length = layer_event.len;
    u8 previous_step_was_long_multi = 0; // hard to determine here... TODO

    // show length of 16 steps
    for(step=0; step<16; ++step) {
      // 9th step reached: switch to second LCD
      if( step == 8 ) {
	MIOS32_LCD_DeviceSet(1);
	MIOS32_LCD_CursorSet(0, 1);
      }

      u8 visible_step = step + 16*ui_selected_step_view;
      SEQ_LAYER_GetEvntOfParLayer(visible_track, visible_step, ui_selected_par_layer, &layer_event);

      if( previous_step_was_long_multi || layer_event.len > 32 ) { // multi note trigger? - note: .len always +1 of parameter layer value
	if( !previous_step_was_long_multi ) {
	  if( previous_length < 24 && !layer_event.midi_package.velocity ) {
	    SEQ_LCD_PrintSpaces(5);
	    previous_length = 0;
	  } else {
	    SEQ_LCD_PrintGatelength(layer_event.len);
	    MIOS32_LCD_PrintChar(layer_event.midi_package.velocity ? ' ' : '?'); // notify about multi-triggered events 
	    // with gate=0 and previous length >= 24 - we don't know how the core will react here

	    // calculate total length of events -> previous_length
	    previous_length = ((layer_event.len-1)>>5) * (layer_event.len & 0x1f) + 1;
	    // long event if > 25
	    previous_step_was_long_multi = previous_length > 25;
	  }
	} else {
	  // continued long event
	  // previous step took 24 steps
	  previous_length -= 24;
	  // print warning (!!!) if current step not activated but should be played due to continued multi trigger
	  if( previous_step_was_long_multi && !layer_event.midi_package.velocity )
	    MIOS32_LCD_PrintString("!!!! ");
	  else
	    MIOS32_LCD_PrintString(">>>> ");
	  // still long event if > 25
	  previous_step_was_long_multi = previous_length > 25;
	}
      } else {
	previous_step_was_long_multi = 0;
	// muted step? if previous gatelength <= 25, print spaces
	if( !layer_event.midi_package.velocity && previous_length <= 25 ) {
	  SEQ_LCD_PrintSpaces(5);
	} else {
	  if( layer_event.len > 24 )
	    SEQ_LCD_PrintHBar(15); // glide
	  else
	    SEQ_LCD_PrintHBar((layer_event.len-1) >> 1);
	}
	previous_length = (layer_event.midi_package.velocity || (previous_length > 25 && layer_event.len > 25)) ? layer_event.len : 0;
      }
    }

  } else {

    // we want to show vertical bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

    // initial cursor position
    MIOS32_LCD_DeviceSet(0);
    MIOS32_LCD_CursorSet(0, 1);

    for(step=0; step<16; ++step) {
      // 9th step reached: switch to second LCD
      if( step == 8 ) {
	MIOS32_LCD_DeviceSet(1);
	MIOS32_LCD_CursorSet(0, 1);
      }

      u8 visible_step = step + 16*ui_selected_step_view;
      seq_layer_evnt_t layer_event;
      SEQ_LAYER_GetEvntOfParLayer(visible_track, visible_step, ui_selected_par_layer, &layer_event);

      switch( layer_ctrl_type ) {
        case SEQ_LAYER_ControlType_Note:
        case SEQ_LAYER_ControlType_Velocity:
	  if( layer_event.midi_package.note && layer_event.midi_package.velocity ) {
	    SEQ_LCD_PrintNote(layer_event.midi_package.note);
	    SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
	  } else {
	    MIOS32_LCD_PrintString("----");
	  }
	  break;

        case SEQ_LAYER_ControlType_Chord1:
	  if( layer_event.midi_package.velocity ) {
	    MIOS32_LCD_PrintFormattedString("S%2d", (SEQ_PAR_Get(visible_track, 0, step) % SEQ_CORE_NUM_STEPS) + 1);
	    SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
	  } else {
	    MIOS32_LCD_PrintString("----");
	  }
	  break;
	  
        case SEQ_LAYER_ControlType_Chord2:
	  if( layer_event.midi_package.note && layer_event.midi_package.velocity ) {
	    u8 par_value = SEQ_PAR_Get(visible_track, step, 0);
	    u8 chord_ix = par_value & 0x0f;
	    u8 chord_oct = par_value >> 4;
	    MIOS32_LCD_PrintFormattedString("%d/%d", chord_ix, chord_oct);
	    SEQ_LCD_PrintVBar(layer_event.midi_package.velocity >> 4);
	  } else {
	    MIOS32_LCD_PrintString("----");
	  }
	  break;
	  
	//case SEQ_LAYER_ControlType_Lenght:
	//break;
	// extra handling -- see code above

        case SEQ_LAYER_ControlType_CC:
	  MIOS32_LCD_PrintFormattedString("%3d", layer_event.midi_package.value);
	  SEQ_LCD_PrintVBar(layer_event.midi_package.value >> 4);
	  break;

        default:
	  MIOS32_LCD_PrintString("????");
	  break;
      }

      MIOS32_LCD_PrintChar(
        (visible_step == ui_selected_step) ? '<' 
	: ((visible_step == ui_selected_step-1) ? '>' : ' '));
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_EDIT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
