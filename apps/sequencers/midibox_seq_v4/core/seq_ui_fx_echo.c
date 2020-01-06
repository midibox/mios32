// $Id$
/*
 * Echo Fx page
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
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       9
#define ITEM_GXTY          0
#define ITEM_ENABLE        1
#define ITEM_REPEATS       2
#define ITEM_DELAY         3
#define ITEM_VELOCITY      4
#define ITEM_FB_VELOCITY   5
#define ITEM_FB_NOTE       6
#define ITEM_FB_GATELENGTH 7
#define ITEM_FB_TICKS      8


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_ENABLE: *gp_leds = 0x0002; break;
    case ITEM_REPEATS: *gp_leds = 0x000c; break;
    case ITEM_DELAY: *gp_leds = 0x0030; break;
    case ITEM_VELOCITY: *gp_leds = 0x00c0; break;
    case ITEM_FB_VELOCITY: *gp_leds = 0x0300; break;
    case ITEM_FB_NOTE: *gp_leds = 0x0c00; break;
    case ITEM_FB_GATELENGTH: *gp_leds = 0x3000; break;
    case ITEM_FB_TICKS: *gp_leds = 0xc000; break;
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

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_ENABLE;
      break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_REPEATS;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_DELAY;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_VELOCITY;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_FB_VELOCITY;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_FB_NOTE;
      break;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_FB_GATELENGTH;
      break;

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_FB_TICKS;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
  case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);

  case ITEM_ENABLE: {
    u8 value = (SEQ_CC_Get(visible_track, SEQ_CC_ECHO_REPEATS) & 0x40) ? 0 : 1; // inverted

    if( incrementer )
      value = incrementer > 0 ? 1 : 0;
    else
      value ^= 1;
    
    SEQ_UI_CC_SetFlags(SEQ_CC_ECHO_REPEATS, 0x40, value ? 0x00 : 0x40);
    return 1; // value changed
  } break;

  case ITEM_REPEATS: {
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_ECHO_REPEATS) & 0x3f;
    if( SEQ_UI_Var8_Inc(&value, 0, 15, incrementer) >= 1 ) {
      SEQ_UI_CC_SetFlags(SEQ_CC_ECHO_REPEATS, 0x3f, value);
      return 1; // value changed
    }
    return 0; // no change
  } break;

  case ITEM_DELAY: {
    // for compatibility with older patches pre Beta30
    u8 value = SEQ_CC_Get(visible_track, SEQ_CC_ECHO_DELAY);
    value = SEQ_CORE_Echo_MapInternalToUser(value);
    if( SEQ_UI_Var8_Inc(&value, 0, 21, incrementer) ) {
      value = SEQ_CORE_Echo_MapUserToInternal(value);
      SEQ_UI_CC_Set(SEQ_CC_ECHO_DELAY, value);
      return 1;
    }
    return 0;
  } break;

  case ITEM_VELOCITY:      return SEQ_UI_CC_Inc(SEQ_CC_ECHO_VELOCITY, 0, 40, incrementer);
  case ITEM_FB_VELOCITY:   return SEQ_UI_CC_Inc(SEQ_CC_ECHO_FB_VELOCITY, 0, 40, incrementer);
  case ITEM_FB_NOTE:       return SEQ_UI_CC_Inc(SEQ_CC_ECHO_FB_NOTE, 0, 49, incrementer);
  case ITEM_FB_GATELENGTH: return SEQ_UI_CC_Inc(SEQ_CC_ECHO_FB_GATELENGTH, 0, 40, incrementer);
  case ITEM_FB_TICKS:      return SEQ_UI_CC_Inc(SEQ_CC_ECHO_FB_TICKS, 0, 40, incrementer);
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
  // Trk.  Echo  Repeats   Delay   Vel.Level  FB Velocity  Note   Gatelen.    Ticks  
  // GxTy   off     3       1/16      75%        120%       + 0     100%       100%

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.  Echo  Repeats   Delay   Vel.Level  FB Velocity  Note   Gatelen.    Ticks  ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(3);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ENABLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_ECHO_REPEATS) & 0x40) ? "off" : "on ");
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REPEATS && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintFormattedString("%2d", SEQ_CC_Get(visible_track, SEQ_CC_ECHO_REPEATS) & 0x3f);
  }
  SEQ_LCD_PrintSpaces(7);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_DELAY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString((char *)SEQ_CORE_Echo_GetDelayModeName(SEQ_CC_Get(visible_track, SEQ_CC_ECHO_DELAY)));
  }
  SEQ_LCD_PrintSpaces(6);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_VELOCITY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%%", 5*SEQ_CC_Get(visible_track, SEQ_CC_ECHO_VELOCITY));
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////

  SEQ_LCD_CursorSet(40, 1);
  SEQ_LCD_PrintSpaces(4);

  if( ui_selected_item == ITEM_FB_VELOCITY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%%", 5*SEQ_CC_Get(visible_track, SEQ_CC_ECHO_FB_VELOCITY));
  }
  SEQ_LCD_PrintSpaces(7);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_FB_NOTE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    u8 note_delta = SEQ_CC_Get(visible_track, SEQ_CC_ECHO_FB_NOTE);
    if( note_delta < 24 )
      SEQ_LCD_PrintFormattedString("-%2d", 24-note_delta);
    else if( note_delta < 49 )
      SEQ_LCD_PrintFormattedString("+%2d", note_delta-24);
    else
      SEQ_LCD_PrintString("Rnd");
  }
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_FB_GATELENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%%", 5*SEQ_CC_Get(visible_track, SEQ_CC_ECHO_FB_GATELENGTH));
  }
  SEQ_LCD_PrintSpaces(7);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_FB_TICKS && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%%", 5*SEQ_CC_Get(visible_track, SEQ_CC_ECHO_FB_TICKS));
  }
  SEQ_LCD_PrintSpaces(2);


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_ECHO_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
