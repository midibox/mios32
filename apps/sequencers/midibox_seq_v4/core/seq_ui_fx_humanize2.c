// $Id$
/*
 * Fx Humanizer page
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
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_humanize.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           5
#define ITEM_GXTY              0
#define ITEM_HUMANIZE_OCTAVE   1
#define ITEM_HUMANIZE_NOTE     2
#define ITEM_HUMANIZE_VELOCITY 3
#define ITEM_HUMANIZE_LENGTH   4


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_HUMANIZE_OCTAVE: *gp_leds = 0x000e; break;
    case ITEM_HUMANIZE_NOTE: *gp_leds = 0x0010; break;
    case ITEM_HUMANIZE_VELOCITY: *gp_leds = 0x0020; break;
    case ITEM_HUMANIZE_LENGTH: *gp_leds = 0x00c0; break;
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
  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      return -1; // not mapped
    case SEQ_UI_ENCODER_GP3:
      return -1; // not mapped
    
    case SEQ_UI_ENCODER_GP4:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_HUMANIZE_OCTAVE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_HUMANIZE_NOTE;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_HUMANIZE_VELOCITY;
      break;
      
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_HUMANIZE_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:              return SEQ_UI_GxTyInc(incrementer);
    case ITEM_HUMANIZE_OCTAVE:   return SEQ_UI_CC_Inc(SEQ_CC_HUMANIZE2_OCT, 0, 127, incrementer);
    case ITEM_HUMANIZE_NOTE:     return SEQ_UI_CC_Inc(SEQ_CC_HUMANIZE2_NOTE, 0, 127, incrementer);
    case ITEM_HUMANIZE_VELOCITY: return SEQ_UI_CC_Inc(SEQ_CC_HUMANIZE2_VEL, 0, 127, incrementer);
    case ITEM_HUMANIZE_LENGTH:   return SEQ_UI_CC_Inc(SEQ_CC_HUMANIZE2_LEN, 0, 127, incrementer);
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
    // re-use encoder handler - only select UI item, don't increment
    return Encoder_Handler((int)button, 0);
  }
  
  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;

      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk.  Intensity     Note Vel/CC Length  
  // G1T1      0          off   off   off    


  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Trk. Prob       Skip Octv Note VelCC Len");
  SEQ_LCD_PrintSpaces(40);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(18);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_HUMANIZE_OCTAVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_HUMANIZE2_OCT));
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_HUMANIZE_NOTE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_HUMANIZE2_NOTE));
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_HUMANIZE_VELOCITY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_HUMANIZE2_VEL));
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_HUMANIZE_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_HUMANIZE2_LEN));
  }
  SEQ_LCD_PrintSpaces(0);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(40);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_HUMANIZE2_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
