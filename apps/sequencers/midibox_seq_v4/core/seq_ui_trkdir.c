// $Id$
/*
 * Track direction page
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

#define NUM_OF_ITEMS       5
#define ITEM_GXTY          0
#define ITEM_DIRECTION     1
#define ITEM_STEPS_FORWARD 2
#define ITEM_STEPS_JMPBCK  3
#define ITEM_STEPS_REPLAY  4


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds, u16 *gp_leds_flashing)
{
  *gp_leds_flashing = 0xffff; // selected item will flash

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break; // GxTy
    case ITEM_DIRECTION: { // dir selection
      int selected_direction = SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_DIRECTION);
      *gp_leds = (1 << (selected_direction+1));
      }
      break;
    case ITEM_STEPS_FORWARD: *gp_leds = 0x0700; break; // Forward
    case ITEM_STEPS_JMPBCK:  *gp_leds = 0x1800; break; // Jump Back
    case ITEM_STEPS_REPLAY:  *gp_leds = 0xe000; break; // Replay
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
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_DIRECTION;
      SEQ_CC_Set(SEQ_UI_VisibleTrackGet(), SEQ_CC_DIRECTION, encoder-1);
      return 1;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_STEPS_FORWARD;
      break;

    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_STEPS_JMPBCK;
      break;

    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_STEPS_REPLAY;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_DIRECTION:     return SEQ_UI_CCInc(SEQ_CC_DIRECTION, 0, 6, incrementer);
    case ITEM_STEPS_FORWARD: return SEQ_UI_CCInc(SEQ_CC_STEPS_FORWARD, 0, 7, incrementer);
    case ITEM_STEPS_JMPBCK:  return SEQ_UI_CCInc(SEQ_CC_STEPS_JMPBCK, 0, 7, incrementer);
    case ITEM_STEPS_REPLAY:  return SEQ_UI_CCInc(SEQ_CC_STEPS_REPLAY, 0, 7, incrementer);
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 1; // value changed

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_DIRECTION;
      SEQ_CC_Set(SEQ_UI_VisibleTrackGet(), SEQ_CC_DIRECTION, button-1);
      return 1; // value changed

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
      ui_selected_item = ITEM_STEPS_FORWARD;
      return 1; // value changed

    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
      ui_selected_item = ITEM_STEPS_JMPBCK;
      return 1; // value changed

    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      ui_selected_item = ITEM_STEPS_REPLAY;
      return 1; // value changed

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
  // >Forward<  PingPong  Rand.Dir  Rand.D+S  Experimental "Progression" Parameters
  // G1T1   Backward  Pendulum  Rand.Step    Steps Fwd: 1  Jump Back: 0  Replay: x 1

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(2);
  }

  ///////////////////////////////////////////////////////////////////////////
  const char dir_names[7][13] = {
    ">Forward<   ",
    ">Backward<  ",
    ">PingPong<  ",
    ">Pendulum<  ",
    ">Rand.Dir<  ",
    ">Rand.Step< ",
    ">Rand.D+S<  "
  };
  int i;
  int selected_direction = SEQ_CC_Get(visible_track, SEQ_CC_DIRECTION);
  for(i=0; i<7; ++i) {
    u8 x = 5*i;
    MIOS32_LCD_CursorSet(x, i%2);

    // print unmodified name if item selected
    // replace '>' and '<' by space if item not selected
    // flash item (print only '>'/'<' and spaces) if cursor position == 1 and flash flag set by timer
    int j;
    for(j=0; j<12; ++j) {
      u8 c = dir_names[i][j];

      if( ++x > 40 ) // don't print more than 40 characters per line
	break;

      if( c == '>' || c == '<' ) {
	MIOS32_LCD_PrintChar((i == selected_direction) ? c : ' ');
      } else {
	if( ui_selected_item == ITEM_DIRECTION && i == selected_direction && ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF )
	  MIOS32_LCD_PrintChar(' ');
	else
	  MIOS32_LCD_PrintChar(c);
      }
    }
  }

  // 3 additional spaces to fill LCD (avoids artefacts on page switches)
  MIOS32_LCD_CursorSet(37, 1);
  SEQ_LCD_PrintSpaces(3);

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(1);
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString(" Experimental \"Progression\" Parameters  ");

  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Steps Fwd:");
  if( ui_selected_item == ITEM_STEPS_FORWARD && ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    MIOS32_LCD_PrintFormattedString("%2d  ", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_FORWARD)+1);
  }

  MIOS32_LCD_PrintString("Jump Back:");
  if( ui_selected_item == ITEM_STEPS_JMPBCK && ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    MIOS32_LCD_PrintFormattedString("%2d  ", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_JMPBCK));
  }

  MIOS32_LCD_PrintString("Replay: x");
  if( ui_selected_item == ITEM_STEPS_REPLAY && ui_cursor_flash_ctr >= SEQ_UI_CURSOR_FLASH_CTR_LED_OFF ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    MIOS32_LCD_PrintFormattedString("%2d ", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_REPLAY)+1);
  }  

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKDIR_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
