// $Id$
/*
 * Options page
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
#include "seq_scale.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       7
#define ITEM_SYNC_CHANGE   0
#define ITEM_STEPS_MEASURE 1
#define ITEM_FOLLOW_SONG   2
#define ITEM_PASTE_CLR_ALL 3
#define ITEM_SCALE_CTRL    4
#define ITEM_SCALE_ROOT    5
#define ITEM_SCALE         6


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_SYNC_CHANGE:   *gp_leds = 0x0001; break;
    case ITEM_STEPS_MEASURE: *gp_leds = 0x0006; break;
    case ITEM_FOLLOW_SONG:   *gp_leds = 0x0018; break;
    case ITEM_PASTE_CLR_ALL: *gp_leds = 0x00e0; break;
    case ITEM_SCALE_CTRL:    *gp_leds = 0x0300; break;
    case ITEM_SCALE_ROOT:    *gp_leds = 0x0400; break;
    case ITEM_SCALE:         *gp_leds = 0xf800; break;
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
      ui_selected_item = ITEM_SYNC_CHANGE;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_STEPS_MEASURE;
      break;
    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_STEPS_MEASURE;
      // special feature: this encoder increments *10
      incrementer *= 10;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_FOLLOW_SONG;
      break;

    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_PASTE_CLR_ALL;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_SCALE_CTRL;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_SCALE_ROOT;
      break;

    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_SCALE;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_SYNC_CHANGE:     seq_core_options.SYNCHED_PATTERN_CHANGE = incrementer >= 0 ? 1 : 0; return 1;
    case ITEM_STEPS_MEASURE:   return SEQ_UI_Var8_Inc(&seq_core_steps_per_measure, 0, 255, incrementer);
    case ITEM_FOLLOW_SONG:     seq_core_options.FOLLOW_SONG = incrementer >= 0 ? 1 : 0; return 1;
    case ITEM_PASTE_CLR_ALL:   seq_core_options.PASTE_CLR_ALL = incrementer >= 0 ? 1 : 0; return 1;return 1;
    case ITEM_SCALE_CTRL:      return SEQ_UI_Var8_Inc(&seq_core_global_scale_ctrl, 0, 4, incrementer);
    case ITEM_SCALE_ROOT:
      if( seq_core_global_scale_ctrl == 0 ) {
	return SEQ_UI_Var8_Inc(&seq_core_global_scale_root_selection, 0, 12, incrementer); // Keyb, C..H
      } else {
	u8 group = seq_core_global_scale_ctrl-1;
	return SEQ_UI_Var8_Inc(&seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+3].shared.scale_root, 0, 12, incrementer); // Keyb, C..H
      }

    case ITEM_SCALE: {
      u8 scale_max = SEQ_SCALE_NumGet()-1;
      if( seq_core_global_scale_ctrl == 0 ) {
	return SEQ_UI_Var8_Inc(&seq_core_global_scale, 0, scale_max, incrementer);
      } else {
	u8 group = seq_core_global_scale_ctrl-1;
	return SEQ_UI_Var8_Inc(&seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+2].shared.scale, 0, scale_max, incrementer); // Keyb, C..H
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_SYNC_CHANGE;
      seq_core_options.SYNCHED_PATTERN_CHANGE ^= 1;
      break;

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
      ui_selected_item = ITEM_STEPS_MEASURE;
      break;

    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_FOLLOW_SONG;
      seq_core_options.FOLLOW_SONG ^= 1;
      break;

    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_PASTE_CLR_ALL;
      seq_core_options.PASTE_CLR_ALL ^= 1;
      break;

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
      ui_selected_item = ITEM_SCALE_CTRL;
      break;

    case SEQ_UI_BUTTON_GP11:
      ui_selected_item = ITEM_SCALE_ROOT;
      break;

    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      ui_selected_item = ITEM_SCALE;
      break;

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
  // SyncPatChange  FollowSong  Paste/Clr Beh Control  Root      Selected Scale      
  //   off    16        off       Steps only  Global   Keyb   50:Hungarian Gypsy    


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("SyncPatChange  FollowSong  Paste/Clr Beh Control  Root      Selected Scale      ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintSpaces(2);

  if( ui_selected_item == ITEM_SYNC_CHANGE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_core_options.SYNCHED_PATTERN_CHANGE ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(3);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_STEPS_MEASURE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", (int)seq_core_steps_per_measure + 1);
  }
  SEQ_LCD_PrintSpaces(8);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_FOLLOW_SONG && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_core_options.FOLLOW_SONG ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(7);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PASTE_CLR_ALL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(11);
  } else {
    SEQ_LCD_PrintString(seq_core_options.PASTE_CLR_ALL ? "Whole Track" : "Steps only ");
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 1);
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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_OPT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
