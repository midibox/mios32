// $Id$
/*
 * Mixer page
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

#include "seq_mixer.h"
#include "seq_file.h"
#include "seq_file_m.h"
#include "seq_file_gc.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS 16

// used "In-Menu" messages
#define MSG_DEFAULT 0x00
#define MSG_COPY    0x81
#define MSG_PASTE   0x82
#define MSG_CLEAR   0x83
#define MSG_LOAD    0x84
#define MSG_SAVE    0x85
#define MSG_DUMP    0x86
#define MSG_NAME    0x87


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 in_menu_msg;

static const char in_menu_msg_str[6][9] = {
  ">COPIED<",	// #1
  ">PASTED<",	// #2
  "CLEARED!",	// #3
  ">>LOAD<<",	// #4
  ">>SAVE<<",	// #5
  ">>DUMP<<"	// #6
};


#define MIXER_UTIL_PAGE_OFF      0
#define MIXER_UTIL_PAGE_OPTIONS  1
#define MIXER_UTIL_PAGE_NAME     2

static u8 show_mixer_util_page;

static u8 mixer_par;

static u8 copypaste_buffer_filled = 0;
static u8 copypaste_buffer[SEQ_MIXER_NUM_CHANNELS][SEQ_MIXER_NUM_PARAMETERS];

static u8 undo_buffer_filled = 0;
static u8 undo_buffer[SEQ_MIXER_NUM_CHANNELS][SEQ_MIXER_NUM_PARAMETERS];
static u8 undo_map;

static char edit_mixer_map_name[20];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed);
static s32 CheckStoreFile(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no LED action as long as files not available

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( show_mixer_util_page ) {
  case MIXER_UTIL_PAGE_OFF:
    *gp_leds = seq_ui_button_state.CHANGE_ALL_STEPS ? 0xffff : (1 << ui_selected_item);
    break;
  case MIXER_UTIL_PAGE_OPTIONS:
    *gp_leds = 0x0001;
    break;
  case MIXER_UTIL_PAGE_NAME:
    *gp_leds = 0x0000;
    break;
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
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no encoder action as long as files not available

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    if( show_mixer_util_page ) {
      switch( show_mixer_util_page ) {
      case MIXER_UTIL_PAGE_OPTIONS: {
	switch( encoder ) {
	case SEQ_UI_ENCODER_GP1: {
	  if( incrementer && SEQ_FILE_M_NumMaps() ) {
	    u8 mixer_map = SEQ_MIXER_NumGet();
	    if( SEQ_UI_Var8_Inc(&mixer_map, 0, SEQ_FILE_M_NumMaps()-1, incrementer) >= 0 ) {
	      SEQ_MIXER_NumSet(mixer_map);

	      // send to external
	      SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP, mixer_map, 0);

	      // load page
	      SEQ_MIXER_Load(SEQ_MIXER_NumGet());

	      // print message
	      in_menu_msg = MSG_LOAD & 0x7f;
	      ui_hold_msg_ctr = 1000;

	      return 1; // value changed
	    }
	  }
	  return 0; // no change
	} break;

        case SEQ_UI_ENCODER_GP2: // Copy
	  // copy map
	  SEQ_UI_MIXER_Copy();
	  // print message
	  in_menu_msg = MSG_COPY & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP3: // Paste
	  // update undo buffer
	  SEQ_UI_MIXER_UndoUpdate();
	  // paste map
	  SEQ_UI_MIXER_Paste();
	  // print message
	  in_menu_msg = MSG_PASTE & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP4: // Clear
	  // update undo buffer
	  SEQ_UI_MIXER_UndoUpdate();
	  // clear map
	  SEQ_UI_MIXER_Clear();
	  // print message
	  in_menu_msg = MSG_CLEAR & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP5:
	  return -1; // not used

        case SEQ_UI_ENCODER_GP6: // Load
	  // load page
	  SEQ_MIXER_Load(SEQ_MIXER_NumGet());
	  // print message
	  in_menu_msg = MSG_LOAD & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP7: // Save
	  // store current page
	  SEQ_MIXER_Save(SEQ_MIXER_NumGet());
	  // print message
	  in_menu_msg = MSG_SAVE & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP8: // Dump
	  // send to external
	  SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP, SEQ_MIXER_NumGet(), 0);
	  // dump all values
	  {
	    MUTEX_MIDIOUT_TAKE;
	    SEQ_MIXER_SendAll();
	    MUTEX_MIDIOUT_GIVE;
	  }
	  // print message
	  in_menu_msg = MSG_DUMP & 0x7f;
	  ui_hold_msg_ctr = 1000;
	  return 1;

        case SEQ_UI_ENCODER_GP9: // select CC1 Number page
        case SEQ_UI_ENCODER_GP10: // select CC2 Number page
        case SEQ_UI_ENCODER_GP11: // select CC3 Number page
        case SEQ_UI_ENCODER_GP12: // select CC4 Number page
	  show_mixer_util_page = MIXER_UTIL_PAGE_OFF;
	  mixer_par = SEQ_MIXER_PAR_CC1_NUM + (encoder-8);
	  return 1; // always changed

        case SEQ_UI_ENCODER_GP13: // change MIXER_LIVE_SEND
        case SEQ_UI_ENCODER_GP14: {
	  u8 value = seq_core_options.MIXER_LIVE_SEND;
	  if( !incrementer )
	    incrementer = seq_core_options.MIXER_LIVE_SEND ? -1 : 1;
	  if( SEQ_UI_Var8_Inc(&value, 0, 1, incrementer) ) {
	    seq_core_options.MIXER_LIVE_SEND = value;
	    ui_store_file_required = 1;
	    return 1;
	  }
	  return 0;
	} break;

        case SEQ_UI_ENCODER_GP16: // Mixermap name
	  // Unnamed -> empty string
	  if( strncmp(seq_mixer_map_name, "Unnamed             ", 20) == 0 ) {
	    int i;
	    for(i=0; i<20; ++i) {
	      edit_mixer_map_name[i] = ' ';
	    }
	  } else {
	    int i;
	    for(i=0; i<20 && seq_mixer_map_name[i] != 0; ++i) {
	      edit_mixer_map_name[i] = seq_mixer_map_name[i];
	    }
	    for(; i<20; ++i) {
	      edit_mixer_map_name[i] = ' ';
	    }
	  }

	  // init keypad editor
	  SEQ_UI_KeyPad_Init();

	  // set cursor to last char
	  ui_edit_name_cursor = 20;
	  while( ui_edit_name_cursor && edit_mixer_map_name[ui_edit_name_cursor-1] == ' ' ) {
	    --ui_edit_name_cursor;
	  }

	  show_mixer_util_page = MIXER_UTIL_PAGE_NAME;
	  
	  return 1; // always changed	  
	}
      } break;

      case MIXER_UTIL_PAGE_NAME: {
	switch( encoder ) {
	case SEQ_UI_ENCODER_GP15:
	  return -1; // no function
	case SEQ_UI_ENCODER_GP16: // back to options page
	  // take over new name
	  strncpy(seq_mixer_map_name, edit_mixer_map_name, 20);

	  // back to options name
	  show_mixer_util_page = MIXER_UTIL_PAGE_OPTIONS;
	  return 1;
	}

	return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&edit_mixer_map_name, 20);
      } break;
      }

      return -1;
    }

    ui_selected_item = encoder;

    u8 min, max;
    switch( mixer_par ) {
      case SEQ_MIXER_PAR_PORT:
	min=0x00; max=SEQ_MIDI_PORT_OutNumGet()-1;
	break;
      case SEQ_MIXER_PAR_CHANNEL:
	min=0x00; max=0x0f;
	break;
      case SEQ_MIXER_PAR_PRG:
      case SEQ_MIXER_PAR_VOLUME:
      case SEQ_MIXER_PAR_PANORAMA:
      case SEQ_MIXER_PAR_REVERB:
      case SEQ_MIXER_PAR_CHORUS:
      case SEQ_MIXER_PAR_MODWHEEL:
      case SEQ_MIXER_PAR_CC1:
      case SEQ_MIXER_PAR_CC2:
      case SEQ_MIXER_PAR_CC3:
      case SEQ_MIXER_PAR_CC4:
	min=0x00; max=0x80;
	break;
      case SEQ_MIXER_PAR_CC1_NUM:
      case SEQ_MIXER_PAR_CC2_NUM:
      case SEQ_MIXER_PAR_CC3_NUM:
      case SEQ_MIXER_PAR_CC4_NUM:
	min=0x00; max=0x7f;
	break;
      default:
	return -1; // unsupported parameter
    }

    s32 value_changed = 0;
    s32 forced_value = -1;

    // first change the selected value
    if( seq_ui_button_state.CHANGE_ALL_STEPS && seq_ui_button_state.CHANGE_ALL_STEPS_SAME_VALUE ) {
      u8 value = SEQ_MIXER_Get(ui_selected_item, mixer_par);
      if( mixer_par == SEQ_MIXER_PAR_PORT )
	value = SEQ_MIDI_PORT_OutIxGet(value);
      if( SEQ_UI_Var8_Inc(&value, min, max, incrementer) ) {
	forced_value = (s32)value;
	if( mixer_par == SEQ_MIXER_PAR_PORT )
	  forced_value = SEQ_MIDI_PORT_OutPortGet(forced_value);
	value_changed |= 1;
      }
      else
	return 0; // no change
    }

    // change value of all selected channels
    u8 chn;
    for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
      if( chn == ui_selected_item || seq_ui_button_state.CHANGE_ALL_STEPS ) {
	if( forced_value >= 0 ) {
	  SEQ_MIXER_Set(chn, mixer_par, forced_value);
	  if( seq_core_options.MIXER_LIVE_SEND ) {
	    MUTEX_MIDIOUT_TAKE;
	    SEQ_MIXER_Send(chn, mixer_par);
	    MUTEX_MIDIOUT_GIVE;
	  }
	} else {
	  u8 value = SEQ_MIXER_Get(chn, mixer_par);
	  if( mixer_par == SEQ_MIXER_PAR_PORT )
	    value = SEQ_MIDI_PORT_OutIxGet(value);
	  if( SEQ_UI_Var8_Inc(&value, min, max, incrementer) ) {
	    if( mixer_par == SEQ_MIXER_PAR_PORT )
	      value = SEQ_MIDI_PORT_OutPortGet(value);
	    SEQ_MIXER_Set(chn, mixer_par, value);
	    if( seq_core_options.MIXER_LIVE_SEND ) {
	      MUTEX_MIDIOUT_TAKE;
	      SEQ_MIXER_Send(chn, mixer_par);
	      MUTEX_MIDIOUT_GIVE;
	    }
	    value_changed |= 1;
	  }
	}
      }
    }

    return value_changed;
  }

  if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    if( show_mixer_util_page ) {
      return Encoder_Handler(SEQ_UI_ENCODER_GP1, incrementer); // only one item in util page
    } else {
      if( mixer_par >= SEQ_MIXER_PAR_CC1_NUM ) // only accessible when select button pressed
	return SEQ_UI_Var8_Inc(&mixer_par, SEQ_MIXER_PAR_PORT, SEQ_MIXER_PAR_CC4_NUM, incrementer);
      else
	return SEQ_UI_Var8_Inc(&mixer_par, SEQ_MIXER_PAR_PORT, SEQ_MIXER_PAR_CC4, incrementer);
    }
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
  if( SEQ_FILE_FormattingRequired() )
    return 0; // no button action as long as files not available

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    if( show_mixer_util_page ) {
      if( depressed ) return 0; // ignore when button depressed

      // -> continue with encoder handler
      return Encoder_Handler((seq_ui_encoder_t)button, 0);
    } else {
      if( depressed ) return 0; // ignore when button depressed
      ui_selected_item = button;

      // dump channel
      {
	MUTEX_MIDIOUT_TAKE;
	SEQ_MIXER_SendAllByChannel(ui_selected_item);
	MUTEX_MIDIOUT_GIVE;
      }

      return 1; // value always changed
    }
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
      if( depressed && show_mixer_util_page == MIXER_UTIL_PAGE_NAME ) {
	// ignore
      } else {
	show_mixer_util_page = depressed ? MIXER_UTIL_PAGE_OFF : MIXER_UTIL_PAGE_OPTIONS;
      }
      return 1; // value always changed

    case SEQ_UI_BUTTON_Right:
      if( depressed ) return 0; // ignore when button depressed
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return 0; // ignore when button depressed
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      if( depressed ) return 0; // ignore when button depressed
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      if( depressed ) return 0; // ignore when button depressed
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

  if( !seq_ui_button_state.SELECT_PRESSED )
    CheckStoreFile(); // for MIXER_LIVE_SEND: stored on SD Card when SELECT has been depressed


  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //   No Mixer Maps available as long as theSession hasn't been created!            
  //            Please press EXIT and create a new Session!                          

  if( SEQ_FILE_FormattingRequired() ) {
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("  No Mixer Maps available as long as theSession hasn't been created!            ");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("           Please press EXIT and create a new Session!                          ");
    return 0;
  }

  switch( show_mixer_util_page ) {
  case MIXER_UTIL_PAGE_OFF: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // Mixer Map #  1      xxxxxxxxxxxxxxxxxxxxPage [ 4] Volume             IIC1 Chn#12
    //  127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_ 127_

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintFormattedString("Mixer Map #%3d", SEQ_MIXER_NumGet()+1);
    SEQ_LCD_PrintSpaces(6);
    SEQ_LCD_PrintString(SEQ_MIXER_MapNameGet()); // 20 characters

    SEQ_LCD_PrintFormattedString("Page%2d  ", mixer_par+1);

    if( mixer_par < SEQ_MIXER_PAR_CC1_NUM ) {
      if( mixer_par < SEQ_MIXER_PAR_CC1 ) {
	const char page_name[8][13] = {
	  "MIDI Port   ",
	  "MIDI Channel",
	  "Prog.Change ",
	  "Volume      ",
	  "Panorama    ",
	  "Reverb      ",
	  "Chorus      ",
	  "ModWheel    ",
	};
	SEQ_LCD_PrintString((char *)page_name[mixer_par]);
      } else {
	SEQ_LCD_PrintFormattedString("CC%d #%3d    ", 
				     mixer_par-SEQ_MIXER_PAR_CC1+1, 
				     SEQ_MIXER_Get(ui_selected_item, mixer_par-SEQ_MIXER_PAR_CC1+SEQ_MIXER_PAR_CC1_NUM));
      }
      SEQ_LCD_PrintSpaces(7);
    } else {
      SEQ_LCD_PrintFormattedString("Assignment for CC%d ", mixer_par-SEQ_MIXER_PAR_CC1_NUM+1);
    }

    SEQ_LCD_PrintSpaces(2);
    SEQ_LCD_PrintMIDIOutPort(SEQ_MIXER_Get(ui_selected_item, SEQ_MIXER_PAR_PORT));
    SEQ_LCD_PrintFormattedString(" Chn#%2d", SEQ_MIXER_Get(ui_selected_item, SEQ_MIXER_PAR_CHANNEL)+1);


    /////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);
    u8 chn;
    for(chn=0; chn<16; ++chn) {
      if( ui_selected_item == chn && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	u8 value = SEQ_MIXER_Get(chn, mixer_par);

	if( mixer_par == SEQ_MIXER_PAR_PORT ) {
	  SEQ_LCD_PrintChar(' ');
	  SEQ_LCD_PrintMIDIOutPort(value);
	} else if( mixer_par == SEQ_MIXER_PAR_CHANNEL ) {
	  SEQ_LCD_PrintFormattedString(" #%2d ", value+1);
	} else if( mixer_par >= SEQ_MIXER_PAR_PRG && mixer_par <= SEQ_MIXER_PAR_CC4 ) {
	  SEQ_LCD_PrintFormattedString(value ? " %3d " : " --- ", value-1);
	} else {
	  SEQ_LCD_PrintFormattedString(" %3d ", value);
	}
      }
    }
  } break;

  case MIXER_UTIL_PAGE_OPTIONS: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // Map#   Mixer Utility Functions          CC Assignments      LiveSend       Edit 
    // 128  Copy Paste Clr      Load Save Dump  CC1  CC2  CC3  CC4    on          Name 
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintFormattedString("Map#   Mixer Utility Functions  ");
    if( (in_menu_msg & 0x80) || ((in_menu_msg & 0x7f) && ui_hold_msg_ctr) ) {
      SEQ_LCD_PrintString((char *)in_menu_msg_str[(in_menu_msg & 0x7f)-1]);
    } else {
      SEQ_LCD_PrintSpaces(8);
    }

    SEQ_LCD_PrintString("CC Assignments      LiveSend       Edit ");

    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintFormattedString("%3d  Copy Paste Clr      Load Save Dump ", SEQ_MIXER_NumGet()+1);
    SEQ_LCD_PrintString(" CC1  CC2  CC3  CC4   ");
    SEQ_LCD_PrintString(seq_core_options.MIXER_LIVE_SEND ? " on" : "off");
    SEQ_LCD_PrintString("          Name ");
  } break;

  case MIXER_UTIL_PAGE_NAME: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // Please enter name for Mixer Map #  1    <xxxxxxxxxxxxxxxxxxxx>                  
    // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins        Done

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintFormattedString("Please enter name for Mixer Map #%3d    ", SEQ_MIXER_NumGet()+1);

    SEQ_LCD_PrintChar('<');
    int i;
    for(i=0; i<20; ++i)
      SEQ_LCD_PrintChar(edit_mixer_map_name[i]);
    SEQ_LCD_PrintChar('>');
    SEQ_LCD_PrintSpaces(18);

    // insert flashing cursor
    if( ui_cursor_flash ) {
      SEQ_LCD_CursorSet(41 + ui_edit_name_cursor, 0);
      SEQ_LCD_PrintChar('*');
    }

    SEQ_UI_KeyPad_LCD_Msg();
    SEQ_LCD_PrintString("       Done");
  } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Stores config file if required
/////////////////////////////////////////////////////////////////////////////
static s32 CheckStoreFile(void)
{
  if( ui_store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    s32 status;
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    ui_store_file_required = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Exit
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  CheckStoreFile();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  in_menu_msg = MSG_DEFAULT;
  ui_hold_msg_ctr = 0;
  show_mixer_util_page = MIXER_UTIL_PAGE_OFF;

  // we want to show vertical VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

  // initialize label editor
  SEQ_UI_KeyPad_Init();

  // disabled: don't change previous settings (values will be initialized with 0 by gcc)
  // SEQ_MIXER_NumSet(0);
  // mixer_par = 0;

  // copypaste_buffer_filled = 0;
  // undo_buffer_filled = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copy Mixer Map
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_Copy(void)
{
  u8 chn;
  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
    u8 par;
    for(par=0; par<SEQ_MIXER_NUM_PARAMETERS; ++par)
      copypaste_buffer[chn][par] = SEQ_MIXER_Get(chn, par);
  }

  // notify that copy&paste buffer is filled
  copypaste_buffer_filled = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Paste Mixer Map
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_Paste(void)
{
  // branch to clear function if copy&paste buffer not filled
  if( !copypaste_buffer_filled )
    return SEQ_UI_MIXER_Clear();

  u8 chn;
  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
    u8 par;
    for(par=0; par<SEQ_MIXER_NUM_PARAMETERS; ++par)
      SEQ_MIXER_Set(chn, par, copypaste_buffer[chn][par]);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Clear Mixer Map
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_Clear(void)
{
  // already available in SEQ_MIXER layer
  return SEQ_MIXER_Clear();
}

/////////////////////////////////////////////////////////////////////////////
// UnDo function
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_Undo(void)
{
  // exit if undo buffer not filled
  if( !undo_buffer_filled )
    return -1;

  // exit if we are not in the same map
  if( undo_map != SEQ_MIXER_NumGet() )
    return -1;

  u8 chn;
  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
    u8 par;
    for(par=0; par<SEQ_MIXER_NUM_PARAMETERS; ++par)
      SEQ_MIXER_Set(chn, par, undo_buffer[chn][par]);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Updates the UnDo buffer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MIXER_UndoUpdate(void)
{
  u8 chn;
  for(chn=0; chn<SEQ_MIXER_NUM_CHANNELS; ++chn) {
    u8 par;
    for(par=0; par<SEQ_MIXER_NUM_PARAMETERS; ++par)
      undo_buffer[chn][par] = SEQ_MIXER_Get(chn, par);
  }

  // remember mixer map
  undo_map = SEQ_MIXER_NumGet();

  // notify that undo buffer is filled
  undo_buffer_filled = 1;

  return 0; // no error
}
