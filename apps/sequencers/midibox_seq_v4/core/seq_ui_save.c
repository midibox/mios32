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
#include <string.h>
#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_pattern.h"
#include "seq_label.h"
#include "seq_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4

#define ITEM_GROUP         0
#define ITEM_BANK          1
#define ITEM_PATTERN       2
#define ITEM_SAVE          3


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static seq_pattern_t save_pattern[SEQ_CORE_NUM_GROUPS];

static u8 edit_label_mode;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  if( edit_label_mode ) {
    *gp_leds = 0;
  } else {
    switch( ui_selected_item ) {
      case ITEM_GROUP:         *gp_leds = 0x000f; break;
      case ITEM_BANK:          *gp_leds = 0x0020; break;
#if 0
      case ITEM_PATTERN:
        // too confusing!
        *gp_leds = (1 << save_pattern[ui_selected_group].group) | (1 << (save_pattern[ui_selected_group].num+8));
        break;
#else
      case ITEM_PATTERN:       *gp_leds = 0x0040; break;
#endif
      case ITEM_SAVE:          *gp_leds = 0x0080; break;
    }
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

  if( edit_label_mode ) {
    switch( encoder ) {
      case SEQ_UI_ENCODER_GP15: { // select preset
	int pos;

	if( ui_edit_name_cursor < 5 ) { // select category preset
	  if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_category, 0, SEQ_LABEL_NumPresetsCategory()-1, incrementer) ) {
	    SEQ_LABEL_CopyPresetCategory(ui_edit_preset_num_category, (char *)&seq_pattern_name[ui_selected_group][0]);
	    for(pos=4, ui_edit_name_cursor=pos; pos>=0; --pos)
	      if( seq_pattern_name[ui_selected_group][pos] == ' ' )
		ui_edit_name_cursor = pos;
	      else
		break;
	    return 1;
	  }
	  return 0;
	} else { // select label preset
	  if( SEQ_UI_Var8_Inc(&ui_edit_preset_num_label, 0, SEQ_LABEL_NumPresets()-1, incrementer) ) {
	    SEQ_LABEL_CopyPreset(ui_edit_preset_num_label, (char *)&seq_pattern_name[ui_selected_group][5]);
	    for(pos=19, ui_edit_name_cursor=pos; pos>=5; --pos)
	      if( seq_pattern_name[ui_selected_group][pos] == ' ' )
		ui_edit_name_cursor = pos;
	      else
		break;
	    return 1;
	  }
	  return 0;
	}
      } break;

      case SEQ_UI_ENCODER_GP16: { // store pattern and exit keypad editor
	// SAVE only via button
	if( incrementer != 0 )
	  return 0;

	s32 status;
	if( (status=SEQ_PATTERN_Save(ui_selected_group, save_pattern[ui_selected_group])) < 0 ) {
	  SEQ_UI_SDCardErrMsg(2000, status);
	} else {
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Pattern saved", "on SD Card!");
	  // change pattern number directly
	  seq_pattern[ui_selected_group] = save_pattern[ui_selected_group];
	}

	// exit keypad editor
	edit_label_mode = 0;
	return 1;
      } break;
    }

    return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&seq_pattern_name[ui_selected_group], 20);
  }


  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_GROUP;
      break;

    case SEQ_UI_ENCODER_GP5:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_BANK;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_PATTERN;
      break;

    case SEQ_UI_ENCODER_GP8:
      if( incrementer != 0 ) {
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Please press", "GP Button!");
	return -1; // too dangerous during live sessions - only available as button function
      }

      ui_selected_item = ITEM_SAVE;
      edit_label_mode = 1;
      SEQ_UI_KeyPad_Init();
      return 1;

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
	char buffer[20];
	sprintf(buffer, "Bank #%d", save_pattern[ui_selected_group].bank);
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 3000, buffer, "not available!");
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

  if( edit_label_mode && button <= SEQ_UI_BUTTON_GP16 )
    return Encoder_Handler((seq_ui_encoder_t)button, 0);

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
      ui_selected_item = ITEM_GROUP;
      break;

    case SEQ_UI_BUTTON_GP5:
      return -1; // not mapped

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_BANK;
      break;

    case SEQ_UI_BUTTON_GP7:
      ui_selected_item = ITEM_PATTERN;
      break;

    case SEQ_UI_BUTTON_GP8:
      // (enter edit label mode)
      ui_selected_item = ITEM_SAVE;
      return Encoder_Handler((seq_ui_encoder_t)button, 0);
      

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      return -1; // not mapped

    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( depressed ) return -1;
      if( edit_label_mode ) {
	// do nothing
      } else {
	if( ++ui_selected_item >= NUM_OF_ITEMS )
	  ui_selected_item = 0;
      }
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      if( edit_label_mode ) {
	// do nothing
      } else {
	if( ui_selected_item == 0 )
	  ui_selected_item = NUM_OF_ITEMS-1;
      }
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
  // Grp. Save Pattern       to Target       Category: xxxxx   Label: xxxxxxxxxxxxxxx
  // G1 (Track 11-11)/1:A1 ----> 1:A1   SAVE                                         

  // Grp. Save Pattern       to Target       Category: xxxxx   Label: xxxxxxxxxxxxxxx
  // G1 (Track 11-11)/1:A1 ----> 1:A1   SAVE   1:A1 on disk: xxxxx xxxxxxxxxxxxxxx   
  //                                         

  // Please enter Pattern Category for 1:A1  <xxxxx-xxxxxxxxxxxxxxx>                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset SAVE

  // Please enter Pattern Label for 1:A1     <xxxxx-xxxxxxxxxxxxxxx>                 
  // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins Preset SAVE


  ///////////////////////////////////////////////////////////////////////////
  if( edit_label_mode ) {
    int i;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString((ui_edit_name_cursor < 5) ? "Please enter Pattern Category for " : "Please enter Pattern Label for ");
    SEQ_LCD_PrintFormattedString("%d", save_pattern[ui_selected_group].bank+1);
    SEQ_LCD_PrintChar(':');
    SEQ_LCD_PrintPattern(save_pattern[ui_selected_group]);
    SEQ_LCD_PrintSpaces((ui_edit_name_cursor < 5) ? 2 : 5);

    SEQ_LCD_PrintChar('<');
    for(i=0; i<5; ++i)
      SEQ_LCD_PrintChar(seq_pattern_name[ui_selected_group][i]);
    SEQ_LCD_PrintChar('-');
    for(i=5; i<20; ++i)
      SEQ_LCD_PrintChar(seq_pattern_name[ui_selected_group][i]);
    SEQ_LCD_PrintChar('>');
    SEQ_LCD_PrintSpaces(17);


    // insert flashing cursor
    if( ui_cursor_flash ) {
      SEQ_LCD_CursorSet(40 + ((ui_edit_name_cursor < 5) ? 1 : 2) + ui_edit_name_cursor, 0);
      SEQ_LCD_PrintChar('*');
    }

    SEQ_UI_KeyPad_LCD_Msg();
    SEQ_LCD_PrintString("Preset SAVE");

    return 0;
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Grp. Save Pattern       to Target       ");


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

  SEQ_LCD_PrintString(" ----> ");

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

  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_SAVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintString("SAVE ");
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(40, 0);
  SEQ_LCD_PrintString("Category: ");
  SEQ_LCD_PrintPatternCategory(seq_pattern[ui_selected_group], seq_pattern_name[ui_selected_group]);
  SEQ_LCD_PrintString("   Label: ");
  SEQ_LCD_PrintPatternLabel(seq_pattern[ui_selected_group], seq_pattern_name[ui_selected_group]);


  ///////////////////////////////////////////////////////////////////////////
  // Save message
  SEQ_LCD_CursorSet(40, 1);
  if( ui_selected_item == ITEM_BANK || ui_selected_item == ITEM_PATTERN ) {
    char pattern_name[21];
    SEQ_PATTERN_PeekName(save_pattern[ui_selected_group], pattern_name);

    SEQ_LCD_PrintFormattedString("%3d", save_pattern[ui_selected_group].bank+1);
    SEQ_LCD_PrintChar(':');
    SEQ_LCD_PrintPattern(save_pattern[ui_selected_group]);
    SEQ_LCD_PrintString(" on Disk: ");
    SEQ_LCD_PrintPatternCategory(seq_pattern[ui_selected_group], pattern_name);
    SEQ_LCD_PrintChar(' ');
    SEQ_LCD_PrintPatternLabel(seq_pattern[ui_selected_group], pattern_name);
    SEQ_LCD_PrintSpaces(3);
  } else
    SEQ_LCD_PrintSpaces(40);

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

  // init edit mode
  edit_label_mode = 0;

  // initialize label editor
  SEQ_UI_KeyPad_Init();
  ui_edit_preset_num_category = 0;
  ui_edit_preset_num_label = 0;

  return 0; // no error
}
