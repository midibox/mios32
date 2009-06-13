// $Id$
/*
 * Scale Fx page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file_c.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_scale.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       3
#define ITEM_SCALE_CTRL    0
#define ITEM_SCALE_ROOT    1
#define ITEM_SCALE         2


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 store_file_required;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_SCALE_CTRL:    *gp_leds = 0x0003; break;
    case ITEM_SCALE_ROOT:    *gp_leds = 0x0004; break;
    case ITEM_SCALE:         *gp_leds = 0x00f8; break;
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
      ui_selected_item = ITEM_SCALE_CTRL;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_SCALE_ROOT;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_SCALE;
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
    case ITEM_SCALE_CTRL:
      if( SEQ_UI_Var8_Inc(&seq_core_global_scale_ctrl, 0, 4, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1;
      }
      return 0;

    case ITEM_SCALE_ROOT:
      if( seq_core_global_scale_ctrl == 0 ) {
	if( SEQ_UI_Var8_Inc(&seq_core_global_scale_root_selection, 0, 12, incrementer) >= 0 ) { // Keyb, C..H
	  store_file_required = 1;
	  return 1;
	}
	return 0;
      } else {
	u8 group = seq_core_global_scale_ctrl-1;
	if( SEQ_UI_Var8_Inc(&seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+3].shared.scale_root, 0, 12, incrementer) >= 0 ) { // Keyb, C..H
	  store_file_required = 1;
	  return 1;
	}
	return 0;
      }

    case ITEM_SCALE: {
      u8 scale_max = SEQ_SCALE_NumGet()-1;
      if( seq_core_global_scale_ctrl == 0 ) {
	if( SEQ_UI_Var8_Inc(&seq_core_global_scale, 0, scale_max, incrementer) >= 0 ) {
	  store_file_required = 1;
	  return 1;
	}
	return 0;
      } else {
	u8 group = seq_core_global_scale_ctrl-1;
	if( SEQ_UI_Var8_Inc(&seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+2].shared.scale, 0, scale_max, incrementer) >= 0 ) { // Keyb, C..H
	  store_file_required = 1;
	  return 1;
	}
	return 0;
      }
    } break;
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
      if( depressed ) return -1;
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      if( depressed ) return -1;
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      if( depressed ) return -1;
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
  //  Control  Root      Selected Scale      
  //  Global   Keyb   50:Hungarian Gypsy    


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString(" Control  Root      Selected Scale      ");
  SEQ_LCD_PrintSpaces(40);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintSpaces(1);

  if( ui_selected_item == ITEM_SCALE_CTRL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(8);
  } else {
    if( seq_core_global_scale_ctrl )
      SEQ_LCD_PrintFormattedString("Group G%d", seq_core_global_scale_ctrl);
    else
      SEQ_LCD_PrintString("Global  ");
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////

  // determine the selected scale and root note selection depending on
  // global/group specific settings
  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);

  if( ui_selected_item == ITEM_SCALE_ROOT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    const char root_str[13][5] = {
      "Keyb", " C  ", " C# ", " D  ", " D# ", " E  ", " F  ", " F# ", " G  ", " G# ", " A  ", " A# ", " B  "
    };
    SEQ_LCD_PrintString((char *)root_str[root_selection]);
  }
  SEQ_LCD_PrintSpaces(2);


  if( ui_selected_item == ITEM_SCALE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(24);
  } else {
    SEQ_LCD_PrintFormattedString("%3d:", scale);
    SEQ_LCD_PrintString(SEQ_SCALE_NameGet(scale));
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(40);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    store_file_required = 0;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_SCALE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  store_file_required = 0;

  return 0; // no error
}
