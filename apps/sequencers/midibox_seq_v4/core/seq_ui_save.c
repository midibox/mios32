// $Id$
/*
 * Save page
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

#include "seq_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       3
#define ITEM_GROUP         0
#define ITEM_BANK          1
#define ITEM_PATTERN       2


// used "In-Menu" messages
#define MSG_DEFAULT 0x00
#define MSG_SAVE    0x81
#define MSG_NO_BANK 0x82


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static seq_pattern_t save_pattern[SEQ_CORE_NUM_GROUPS];

static u8 in_menu_msg;
static u32 in_menu_msg_error_status;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GROUP:         *gp_leds = 0x000f; break;
    case ITEM_BANK:          *gp_leds = 0x0010; break;
#if 0
    case ITEM_PATTERN:
      *gp_leds = (1 << save_pattern[ui_selected_group].group) | (1 << (save_pattern[ui_selected_group].num+8));
      break;
#else
    case ITEM_PATTERN:       *gp_leds = 0x0020; break;
#endif
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
      ui_selected_item = ITEM_GROUP;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_BANK;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_PATTERN;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return 0; // not used
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GROUP:
      return SEQ_UI_Var8_Inc(&ui_selected_group, 0, SEQ_CORE_NUM_GROUPS-1, incrementer);
    case ITEM_BANK: {
      u8 tmp = save_pattern[ui_selected_group].bank;
      if( SEQ_UI_Var8_Inc(&tmp, 0, SEQ_FILE_B_NUM_BANKS-1, incrementer) ) {
	save_pattern[ui_selected_group].bank = tmp;
	return 1;
      }
      return 0;
    } break;
    case ITEM_PATTERN: {
      u8 tmp = save_pattern[ui_selected_group].pattern;
      u8 max_patterns = SEQ_FILE_B_NumPatterns(save_pattern[ui_selected_group].bank);

      if( !max_patterns ) {
	// print error message if bank not valid (max_patterns = 0)
	in_menu_msg = MSG_NO_BANK;
	in_menu_msg_error_status = save_pattern[ui_selected_group].bank;
	ui_hold_msg_ctr = 2000; // print for 2 seconds
	return 1;
      }

      if( SEQ_UI_Var8_Inc(&tmp, 0, max_patterns-1, incrementer) ) {
	save_pattern[ui_selected_group].pattern = tmp;
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
      ui_selected_item = ITEM_GROUP;
      break;

    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_BANK;
      break;

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_PATTERN;
      break;

    case SEQ_UI_BUTTON_GP7:
      return 0;

    case SEQ_UI_BUTTON_GP8: {
      s32 status;
      if( (status=SEQ_FILE_B_PatternWrite(save_pattern[ui_selected_group].bank, save_pattern[ui_selected_group].pattern, ui_selected_group)) < 0 ) {
	// TODO: print error message
      } else {
	// TODO: print success message
	// change pattern number directly
	seq_pattern[ui_selected_group] = save_pattern[ui_selected_group];
      }
      in_menu_msg_error_status = -status; // we expect a value within byte range...
      in_menu_msg = MSG_SAVE;
      ui_hold_msg_ctr = 2000; // print for 2 seconds
      return 1;
    } break;

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      return 0;

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
  // Grp. Save Pattern    to Target     SAVE Name: xxxxxxxxxxxxxxxxxxxx              
  // G1 (Track 11-11)/1:A1 -> 1:A1       IT  ToDo: Pattern name entry                                                        


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Grp. Save Pattern    to Target     SAVE ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GROUP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("G%d ", ui_selected_group+1);
  }

  SEQ_LCD_PrintFormattedString("(Track %2d-%2d)/",
				  (ui_selected_group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+1,
				  ((ui_selected_group+1)*SEQ_CORE_NUM_TRACKS_PER_GROUP));

  if( ui_selected_item == ITEM_GROUP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%d:", seq_pattern[ui_selected_group].bank+1);
    SEQ_LCD_PrintPattern(seq_pattern[ui_selected_group]);
  }

  SEQ_LCD_PrintString(" -> ");

  if( ui_selected_item == ITEM_BANK && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(1);
  } else {
    SEQ_LCD_PrintFormattedString("%d", save_pattern[ui_selected_group].bank+1);
  }

  SEQ_LCD_PrintChar(':');

  if( ui_selected_item == ITEM_PATTERN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintPattern(save_pattern[ui_selected_group]);
  }

  SEQ_LCD_PrintChar(SEQ_FILE_B_NumPatterns(save_pattern[ui_selected_group].bank) ? ' ' : '!');

  SEQ_LCD_PrintSpaces(6);
  SEQ_LCD_PrintString("IT  ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);
  SEQ_LCD_PrintString("Name: ");
  SEQ_LCD_PrintPatternName(seq_pattern[ui_selected_group], seq_pattern_name[ui_selected_group]);
  SEQ_LCD_PrintSpaces(14);

  ///////////////////////////////////////////////////////////////////////////
  // Save message
  SEQ_LCD_CursorSet(40, 1);
  if( ui_hold_msg_ctr && in_menu_msg != MSG_DEFAULT ) {
    switch( in_menu_msg ) {
      case MSG_SAVE:
	if( in_menu_msg_error_status ) // TODO: here we could decode the error status
	  SEQ_LCD_PrintFormattedString("Store Operation failed with error #%-3d  ", in_menu_msg_error_status);
	else
	  SEQ_LCD_PrintString("Pattern stored successfully             ");
        break;

      case MSG_NO_BANK:
	SEQ_LCD_PrintFormattedString("Bank #%d not available!", in_menu_msg_error_status+1);
	SEQ_LCD_PrintSpaces(18);
        break;
    }
  } else {
    SEQ_LCD_PrintString("ToDo: Pattern name entry                ");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SAVE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // copy current patterns into save patterns
  int i;
  for(i=0; i<SEQ_CORE_NUM_GROUPS; ++i)
    save_pattern[i] = seq_pattern[i];

  // for printing the error status after save operation
  in_menu_msg = MSG_DEFAULT;
  ui_hold_msg_ctr = 0;
  in_menu_msg_error_status = 0;

  return 0; // no error
}
