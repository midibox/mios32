// $Id$
/*
 * Fx Loop page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_core.h"

#include "seq_file.h"
#include "seq_file_c.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           4
#define ITEM_LOOP_MODE         0
#define ITEM_LOOP_OFFSET       1
#define ITEM_LOOP_STEPS        2
#define ITEM_LOOP_ACTIVE       3


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_LOOP_MODE: *gp_leds = 0x001f; break;
    case ITEM_LOOP_OFFSET: *gp_leds = 0x0020; break;
    case ITEM_LOOP_STEPS: *gp_leds = 0x0040; break;
    case ITEM_LOOP_ACTIVE: *gp_leds = 0x0100; break;
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
    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_LOOP_MODE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_LOOP_OFFSET;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_LOOP_STEPS;
      break;

    case SEQ_UI_ENCODER_GP8:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_LOOP_ACTIVE;
      break;

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
    case ITEM_LOOP_MODE: {
      u8 value = seq_core_glb_loop_mode;
      if( SEQ_UI_Var8_Inc(&value, 0, SEQ_CORE_NUM_LOOP_MODES-1, incrementer) > 0 ) {
	seq_core_glb_loop_mode = value;
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
    } break;

    case ITEM_LOOP_OFFSET:
      if( SEQ_UI_Var8_Inc(&seq_core_glb_loop_offset, 0, 255, incrementer) > 0 ) {
	ui_store_file_required = 1;
	return 1;
      }
      return 0;

    case ITEM_LOOP_STEPS:
      if( SEQ_UI_Var8_Inc(&seq_core_glb_loop_steps, 0, 255, incrementer) ) {
	ui_store_file_required = 1;
	return 1;
      }
      return 0;

    case ITEM_LOOP_ACTIVE:
      // should be atomic
      MIOS32_IRQ_Disable();
      if( incrementer == 0 ) // button toggle function
	seq_core_state.LOOP ^= 1;
      else
	seq_core_state.LOOP = (incrementer >= 0) ? 1 : 0;
      MIOS32_IRQ_Enable();
      return 1;
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
  // Global Loop Mode       Offset Steps     Loop                                    
  // All Tracks/Step View       1    16      off                                     

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Global Loop Mode       Offset Steps     Loop");
  SEQ_LCD_PrintSpaces(34);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_LOOP_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(24);
  } else {
    const char loop_mode_str[SEQ_CORE_NUM_LOOP_MODES][25] = {
      "All Tracks/Step View    ",
      "All Tracks/Static Steps ",
      "Selected Track/Step View",
      "Selected Track/Static   "
    };

    SEQ_LCD_PrintString((char *)loop_mode_str[seq_core_glb_loop_mode >= SEQ_CORE_NUM_LOOP_MODES ? 0 : seq_core_glb_loop_mode]);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LOOP_OFFSET && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", (int)seq_core_glb_loop_offset+1);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LOOP_STEPS && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", (int)seq_core_glb_loop_steps+1);
  }
  SEQ_LCD_PrintSpaces(6);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LOOP_ACTIVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_core_state.LOOP ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(37);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( ui_store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    ui_store_file_required = 0;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_LOOP_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  return 0; // no error
}
