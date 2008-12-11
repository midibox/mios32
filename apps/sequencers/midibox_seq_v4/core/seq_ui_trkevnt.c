// $Id$
/*
 * Track clock divider page
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
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       8
#define ITEM_GXTY          0
#define ITEM_EVNT_MODE     1
#define ITEM_EVNT_CONST1   2
#define ITEM_EVNT_CONST2   3
#define ITEM_EVNT_CONST3   4
#define ITEM_MIDI_CHANNEL  5
#define ITEM_MIDI_PORT     6
#define ITEM_PRESET        7


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_EVNT_MODE: *gp_leds = 0x001e; break;
    case ITEM_EVNT_CONST1: *gp_leds = 0x0020; break;
    case ITEM_EVNT_CONST2: *gp_leds = 0x0040; break;
    case ITEM_EVNT_CONST3: *gp_leds = 0x0080; break;
    case ITEM_MIDI_CHANNEL: *gp_leds = 0x0100; break;
    case ITEM_MIDI_PORT: *gp_leds = 0x0200; break;
    case ITEM_PRESET: *gp_leds = 0xc000; break;
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
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_EVNT_MODE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_EVNT_CONST1;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_EVNT_CONST2;
      break;

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_EVNT_CONST3;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_MIDI_PORT;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_PRESET;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_EVNT_MODE:     return SEQ_UI_CCInc(SEQ_CC_MIDI_EVNT_MODE, 0, SEQ_LAYER_EVNTMODE_NUM-1, incrementer);
    case ITEM_EVNT_CONST1:   return SEQ_UI_CCInc(SEQ_CC_MIDI_EVNT_CONST1, 0, 127, incrementer);
    case ITEM_EVNT_CONST2:   return SEQ_UI_CCInc(SEQ_CC_MIDI_EVNT_CONST2, 0, 127, incrementer);
    case ITEM_EVNT_CONST3:   return SEQ_UI_CCInc(SEQ_CC_MIDI_EVNT_CONST3, 0, 127, incrementer);
    case ITEM_MIDI_CHANNEL: return SEQ_UI_CCInc(SEQ_CC_MIDI_CHANNEL, 0, 15, incrementer);
    case ITEM_MIDI_PORT: return SEQ_UI_CCInc(SEQ_CC_MIDI_PORT, 0, 3, incrementer); // TODO: use global define for number of MIDI ports!
    case ITEM_PRESET: return -1; // TODO
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_EVNT_MODE;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_EVNT_CONST1;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP7:
      ui_selected_item = ITEM_EVNT_CONST2;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_EVNT_CONST3;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP9:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP10:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      return -1;

    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      return -1; // TODO: copy preset!

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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Layer Assignments:xx Val1 Val2 Val3Chn. Port                     >> COPY <<
  // G1T1 A:Chrd B:Vel. C:Len.  127  127  127 16  IIC2 (not available)     > PRESET <

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 evnt_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_MODE);

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Trk. Layer Assignments:");
  ///
  if( ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    MIOS32_LCD_PrintFormattedString("%2d", evnt_mode + 1);
  }
  MIOS32_LCD_PrintFormattedString(" %s %s %s", 
				  SEQ_LAYER_CTypeStr(evnt_mode, 0),
				  SEQ_LAYER_CTypeStr(evnt_mode, 1),
				  SEQ_LAYER_CTypeStr(evnt_mode, 2));
  ///
  MIOS32_LCD_PrintString(" Val1 Val2 Val3");
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Chn. Port                     >> COPY <<");


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "A:");
  MIOS32_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 0));
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "B:");
  MIOS32_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 1));
  SEQ_LCD_PrintSpaces(1);

  MIOS32_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "C:");
  MIOS32_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 2));


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EVNT_CONST1 && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    MIOS32_LCD_PrintFormattedString("  %3d", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_CONST1));
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EVNT_CONST2 && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    MIOS32_LCD_PrintFormattedString("  %3d", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_CONST2));
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EVNT_CONST3 && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    MIOS32_LCD_PrintFormattedString("  %3d", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_CONST3));
  }

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_CursorSet(0, 1);
  if( ui_selected_item == ITEM_MIDI_CHANNEL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    MIOS32_LCD_PrintFormattedString("%3d  ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MIDI_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    MIOS32_LCD_PrintFormattedString("0x%02x ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT)); // TODO: get MIDI port name from SEQ_MIDI
  }
#if 0
  MIOS32_LCD_PrintString(1 ? "(not available)" : "(available)    "); // TODO: get MIDI port availability from SEQ_MIDI
#else
  SEQ_LCD_PrintSpaces(15);
#endif

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_PrintString("     > PRESET <");


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKEVNT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
