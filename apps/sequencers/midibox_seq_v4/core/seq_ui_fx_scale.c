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

#include "seq_file.h"
#include "seq_file_c.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_scale.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       2
#define ITEM_SCALE_ROOT    0
#define ITEM_SCALE         1


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
      return -1; // obsolete
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
      // just to make it clear... ;-)
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "To display the scale only", "No editing possible (yet ;-)");
      return 0;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_SCALE_ROOT:
      if( SEQ_UI_Var8_Inc(&seq_core_global_scale_root_selection, 0, 12, incrementer) >= 0 ) { // Keyb, C..H
	ui_store_file_required = 1;
	return 1;
      }
      return 0;

    case ITEM_SCALE: {
      u8 scale_max = SEQ_SCALE_NumGet()-1;
      if( SEQ_UI_Var8_Inc(&seq_core_global_scale, 0, scale_max, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1;
      }
      return 0;
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
      else
	--ui_selected_item;
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
  const char keys_str[12][5] = {
    " C  ", " C# ", " D  ", " D# ", " E  ", " F  ", " F# ", " G  ", " G# ", " A  ", " A# ", " B  "
  };

  if( high_prio )
    return 0; // there are no high-priority updates

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //           Root      Selected Scale      
  //           Keyb   50:Hungarian Gypsy    


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("          Root      Selected Scale      ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintSpaces(10);

  ///////////////////////////////////////////////////////////////////////////

  // determine the selected scale and root note selection depending on
  // global/group specific settings
  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(visible_track, ui_selected_step, ui_selected_instrument, NULL, &scale, &root_selection, &root);

  if( ui_selected_item == ITEM_SCALE_ROOT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( !root_selection ) {
      SEQ_LCD_PrintString("Keyb ");
    } else {
      SEQ_LCD_PrintRootValue(root_selection);
    }
  }
  SEQ_LCD_PrintSpaces(2);


  if( ui_selected_item == ITEM_SCALE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(24);
  } else {
    SEQ_LCD_PrintFormattedString("%3d:", scale);
    SEQ_LCD_PrintString(SEQ_SCALE_NameGet(scale));
  }

  ///////////////////////////////////////////////////////////////////////////
  int y;
  int i;
  for(y=0; y<2; ++y) {
    SEQ_LCD_CursorSet(40, y);
    SEQ_LCD_PrintSpaces(2);

    for(i=0; i<12; ++i) {
      u8 key_ix = (root + i) % 12;
      char *key_str = (char *)&keys_str[key_ix][0];
      u8 halftone = key_str[2] == '#';

      u8 scaled_note = SEQ_SCALE_NoteValueGet(key_ix, scale, root);
      char *scaled_key_str = (char *)&keys_str[scaled_note % 12][0];

      if( (halftone && y == 0) || (!halftone && y == 1) ) {
	int j;
	for(j=0; j<3; ++j)
	  SEQ_LCD_PrintChar(scaled_key_str[j]);
      } else {
	SEQ_LCD_PrintSpaces(3);
      }
    }
    SEQ_LCD_PrintSpaces(2);
  }

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
s32 SEQ_UI_FX_SCALE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  return 0; // no error
}
