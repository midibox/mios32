// $Id$
/*
 * Track record page
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
#include "seq_record.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       6
#define ITEM_GXTY          0
#define ITEM_STEP_MODE     1
#define ITEM_POLY_MODE     2
#define ITEM_AUTO_START    3
#define ITEM_RECORD_STEP   4
#define ITEM_TOGGLE_GATE   5


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  // branch to edit page if new event has been recorded (ui_hold_msg_ctr controlled from SEQ_RECORD_*)
  if( ui_hold_msg_ctr )
    return SEQ_UI_EDIT_LED_Handler(gp_leds);

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_STEP_MODE: *gp_leds = 0x0002; break;
    case ITEM_POLY_MODE: *gp_leds = 0x0004; break;
    case ITEM_AUTO_START: *gp_leds = 0x0018; break;
    case ITEM_RECORD_STEP: *gp_leds = 0x0020; break;
    case ITEM_TOGGLE_GATE: *gp_leds = 0x00c0; break;
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

  // ensure that original record screen will be print immediately
  ui_hold_msg_ctr = 0;

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_STEP_MODE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_POLY_MODE;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_AUTO_START;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_RECORD_STEP;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_TOGGLE_GATE;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return 0; // not mapped yet
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:
      return SEQ_UI_GxTyInc(incrementer);

    case ITEM_STEP_MODE:
      if( incrementer )
	seq_record_options.STEP_RECORD = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.STEP_RECORD ^= 1;
      return 1;

    case ITEM_POLY_MODE:
      if( incrementer )
	seq_record_options.POLY_RECORD = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.POLY_RECORD ^= 1;
      return 1;

    case ITEM_AUTO_START:
      if( incrementer )
	seq_record_options.AUTO_START = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.AUTO_START ^= 1;
      return 1;

    case ITEM_RECORD_STEP:
      return SEQ_UI_Var8_Inc(&seq_record_step, 0, SEQ_TRG_NumStepsGet(visible_track)-1, incrementer);

    case ITEM_TOGGLE_GATE: {
      u8 gate;
      if( incrementer > 0 )
	gate = 1;
      else if( incrementer < 0 )
	gate = 0;
      else
	gate = SEQ_TRG_GateGet(visible_track, seq_record_step, ui_selected_instrument) ? 0 : 1;

      int i;
      for(i=0; i<SEQ_TRG_NumInstrumentsGet(); ++i)
	SEQ_TRG_GateSet(visible_track, seq_record_step, i, gate);
      
      SEQ_RECORD_PrintEditScreen();
      return 1;
    }
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
    // re-use encoder handler - only select UI item, don't increment, flags will be toggled
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // branch to edit page if new event has been recorded (ui_hold_msg_ctr controlled from SEQ_RECORD_*)
  if( ui_hold_msg_ctr )
    return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_RECORD);

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Record Mode  AStart  Step  TglGate 
  // G1T1 Live  Poly    on      16           

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Record Mode  AStart  Step  TglGate ");
  SEQ_LCD_PrintSpaces(40);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_STEP_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(seq_record_options.STEP_RECORD ? "Step" : "Live");
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_POLY_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(seq_record_options.POLY_RECORD ? "Poly" : "Mono");
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_AUTO_START && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_options.AUTO_START ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RECORD_STEP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", seq_record_step+1);
  }
  SEQ_LCD_PrintSpaces(11);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(40);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKREC_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
