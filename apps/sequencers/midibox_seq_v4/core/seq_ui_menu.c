// $Id$
/*
 * Menu page (visible when EXIT button is pressed)
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
#include <seq_midi_out.h>
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           2
#define ITEM_LIST1             0
#define ITEM_LIST2             1
#define ITEM_OPEN              2
#define ITEM_SAVE              3
#define ITEM_SAVE_AS           4
#define ITEM_NEW               5
#define ITEM_INFO              6



// Session
#define MENU_DIALOG_NONE          0
#define MENU_DIALOG_OPEN          1
#define MENU_DIALOG_SAVE_AS       2
#define MENU_DIALOG_SAVE_DEXISTS  3
#define MENU_DIALOG_NEW           4
#define MENU_DIALOG_NEW_DEXISTS   5

#define MENU_NUM_LIST_DISPLAYED_ITEMS 2
#define MENU_LIST_ENTRY_WIDTH 19

#define SESSION_NUM_LIST_DISPLAYED_ITEMS 4
#define SESSION_LIST_ENTRY_WIDTH 9


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 menu_view_offset = 0; // only changed once after startup or if menu page outside view
static u8 menu_selected_item = 0; // dito

static u8 menu_dialog;

static s32 dir_num_items; // contains SEQ_FILE error status if < 0
static u8 dir_view_offset = 0; // only changed once after startup
static char dir_name[12]; // directory name of device (first char is 0 if no device selected)

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_UI_MENU_UpdatePageList(void);
static s32 SEQ_UI_MENU_UpdateSessionList(void);
static s32 DoSessionSave(u8 force_overwrite);
static s32 DoSessionNew(u8 force_overwrite);
static s32 OpenSession(void);

/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  *gp_leds = 0x0000;

  switch( menu_dialog ) {
  case MENU_DIALOG_NONE:
    if( ui_cursor_flash ) // if flashing flag active: no LED flag set
      return 0;
    if( menu_selected_item <= ITEM_LIST2 )
      *gp_leds = (15 << (4*menu_selected_item));
    break;

  case MENU_DIALOG_OPEN:
    *gp_leds = (3 << (2*ui_selected_item));
    break;

  case MENU_DIALOG_SAVE_AS:
  case MENU_DIALOG_SAVE_DEXISTS:
  case MENU_DIALOG_NEW:
  case MENU_DIALOG_NEW_DEXISTS:
    // no LED functions yet
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
  switch( menu_dialog ) {
  case MENU_DIALOG_NONE:
    // datawheel and GP1-8 change the menu view
    if( encoder <= SEQ_UI_ENCODER_GP8 || encoder == SEQ_UI_ENCODER_Datawheel ) {
      if( SEQ_UI_SelectListItem(incrementer, SEQ_UI_NUM_MENU_PAGES, MENU_NUM_LIST_DISPLAYED_ITEMS, &menu_selected_item, &menu_view_offset) )
	SEQ_UI_MENU_UpdatePageList();

      return 1;      
    }

    // (GP9..GP16 ignored)
    break;


  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_OPEN:
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP16:
      // EXIT only via button
      if( incrementer == 0 ) {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;

    default:
      if( SEQ_UI_SelectListItem(incrementer, dir_num_items, SESSION_NUM_LIST_DISPLAYED_ITEMS, &ui_selected_item, &dir_view_offset) )
	SEQ_UI_MENU_UpdateSessionList();
    }
    break;


  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_SAVE_AS: {
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP15: {
      // SAVE only via button
      if( incrementer != 0 )
	return 0;

      if( DoSessionSave(0) == 0 ) { // don't force overwrite
	// exit keypad editor
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }

      return 1;
    } break;

    case SEQ_UI_ENCODER_GP16: {
      // EXIT only via button
      if( incrementer == 0 ) {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;
    }

    return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&dir_name[0], 8);
  }


  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_NEW: {
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP15: {
      // NEW only via button
      if( incrementer != 0 )
	return 0;

      if( DoSessionNew(0) == 0 ) { // don't force overwrite
	// exit keypad editor
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }

      return 1;
    } break;

    case SEQ_UI_ENCODER_GP16: {
      // EXIT only via button
      if( incrementer == 0 ) {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;
    }

    return SEQ_UI_KeyPad_Handler(encoder, incrementer, (char *)&dir_name[0], 8);
  }


  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_SAVE_DEXISTS: {
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP11: {
      // YES only via button
      if( incrementer != 0 )
	return 0;

      // YES: overwrite session
      if( DoSessionSave(1) == 0 ) { // force overwrite
	// exit keypad editor
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP12: {
      // NO only via button
      if( incrementer != 0 )
	return 0;

      // NO: don't overwrite session - back to dirname entry

      ui_selected_item = 0;
      menu_dialog = MENU_DIALOG_SAVE_AS;
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP16: {
      // EXIT only via button
      if( incrementer == 0 ) {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;
    }

    return -1; // not mapped
  }


  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_NEW_DEXISTS: {
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP11: {
      // YES only via button
      if( incrementer != 0 )
	return 0;

      // YES: overwrite session
      if( DoSessionNew(1) == 0 ) { // force overwrite
	// exit keypad editor
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP12: {
      // NO only via button
      if( incrementer != 0 )
	return 0;

      // NO: don't overwrite session - back to dirname entry

      ui_selected_item = 0;
      menu_dialog = MENU_DIALOG_NEW;
      return 1;
    } break;

    case SEQ_UI_ENCODER_GP16: {
      // EXIT only via button
      if( incrementer == 0 ) {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
    } break;
    }

    return -1; // not mapped
  }

  }


  // all other encoders
  return -1;
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
  switch( menu_dialog ) {
  case MENU_DIALOG_NONE:
    if( depressed ) return 0; // ignore when button depressed
    if( button <= SEQ_UI_BUTTON_GP8 || button == SEQ_UI_BUTTON_Select ) {
      if( button != SEQ_UI_BUTTON_Select )
	menu_selected_item = button / 4;

      SEQ_UI_PageSet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item);
      return 1;
    }

    switch( button ) {
    case SEQ_UI_BUTTON_GP9:
      ui_selected_item = 0;
      menu_dialog = MENU_DIALOG_OPEN;
      SEQ_UI_MENU_UpdateSessionList();
      return 1;

    case SEQ_UI_BUTTON_GP10:
      if( SEQ_FILE_FormattingRequired() )
	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "No valid session!", "Create NEW one!");
      else {
	// "save all" done in app.c as background task
	seq_ui_saveall_req = 1;

	// print message immediately for better "look&feel"
	// otherwise we could think that the button isn't working
	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "All 4 patterns", "stored!");
      }
      return 1;

    case SEQ_UI_BUTTON_GP11: {
      if( SEQ_FILE_FormattingRequired() )
	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "No valid session!", "Create NEW one!");
      else {
	// "save all" done in app.c as background task
	seq_ui_saveall_req = 1;

	// enter keypad editor anyhow
	// initialize keypad editor
	SEQ_UI_KeyPad_Init();
	int i;
	for(i=0; i<8; ++i)
	  dir_name[i] = ' ';
	ui_selected_item = 0;
	menu_dialog = MENU_DIALOG_SAVE_AS;
      }
      return 1;
    }

    case SEQ_UI_BUTTON_GP12: {
      // initialize keypad editor
      SEQ_UI_KeyPad_Init();
      int i;
      for(i=0; i<8; ++i)
	dir_name[i] = ' ';
      ui_selected_item = 0;
      menu_dialog = MENU_DIALOG_NEW;
    } return 1;

    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      return 1;

    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      SEQ_UI_PageSet(SEQ_UI_PAGE_INFO);
      return 1;
    }
    break;

  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_OPEN:
    if( depressed ) return 0; // ignore when button depressed

    if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 )
      return Encoder_Handler(button, 0); // re-use encoder handler

    if( button <= SEQ_UI_BUTTON_GP8 || button == SEQ_UI_BUTTON_Select ) {
      if( button != SEQ_UI_BUTTON_Select )
	ui_selected_item = button / 2;

      if( dir_num_items >= 1 && (ui_selected_item+dir_view_offset) < dir_num_items ) {
	// get filename
	int i;
	char *p = (char *)&dir_name[0];
	for(i=0; i<8; ++i) {
	  char c = ui_global_dir_list[SESSION_LIST_ENTRY_WIDTH*ui_selected_item + i];
	  if( c != ' ' )
	    *p++ = c;
	}
	*p++ = 0;

	// try to open session
	OpenSession();

	// stay in menu...
#if 0
	// switch to main page
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
#endif
	return 1;
      }
    }
    break;

  ///////////////////////////////////////////////////////////////////////////
  case MENU_DIALOG_SAVE_AS:
  case MENU_DIALOG_SAVE_DEXISTS:
  case MENU_DIALOG_NEW:
  case MENU_DIALOG_NEW_DEXISTS:
    if( depressed ) return 0; // ignore when button depressed
    return Encoder_Handler((seq_ui_encoder_t)button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

    case SEQ_UI_BUTTON_Exit:
      if( menu_dialog == MENU_DIALOG_NONE )
	SEQ_UI_PageSet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item);
      else {
	SEQ_UI_MENU_UpdatePageList();
	menu_dialog = MENU_DIALOG_NONE;
      }
      return 1;
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  switch( menu_dialog ) {
  case MENU_DIALOG_NONE: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // MIDIbox SEQ V4.0Beta17         1.  1.  0/SESSION/20100309                       
    //        Edit             Mute Tracks     Open Save SaveAs New               Info 

    if( high_prio ) {
      ///////////////////////////////////////////////////////////////////////////
      // Print Sequencer Position
      SEQ_LCD_CursorSet(24, 0);

      u32 tick = SEQ_BPM_TickGet();
      u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
      u32 ticks_per_measure = ticks_per_step * (seq_core_steps_per_measure+1);
      u32 measure = (tick / ticks_per_measure) + 1;
      u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;
      u32 microstep = tick % ticks_per_step;
      SEQ_LCD_PrintFormattedString("%8u.%3d.%3d", measure, step, microstep);
      return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
    int fill_spaces = 25 - strlen(MIOS32_LCD_BOOT_MSG_LINE1);
    if( fill_spaces > 0 )
      SEQ_LCD_PrintSpaces(fill_spaces);

    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintFormattedString("%s/%s", SEQ_FILE_SESSION_PATH, seq_file_session_name);
    SEQ_LCD_PrintSpaces(40);


    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);

    SEQ_LCD_PrintList((char *)ui_global_dir_list, MENU_LIST_ENTRY_WIDTH, SEQ_UI_NUM_MENU_PAGES, MENU_NUM_LIST_DISPLAYED_ITEMS, menu_selected_item, menu_view_offset);

    SEQ_LCD_CursorSet(40, 1);
    SEQ_LCD_PrintString("Open Save SaveAs New               Info ");
  } break;

  case MENU_DIALOG_OPEN: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // Open Session (10 found)                 Current Session: /SESSION/xxxxxxxx      
    //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx                                     EXIT

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintSpaces(40);
    SEQ_LCD_CursorSet(0, 0);
    if( dir_num_items < 0 ) {
      if( dir_num_items == SEQ_FILE_ERR_NO_DIR )
	SEQ_LCD_PrintFormattedString("%s directory not found on SD Card!", SEQ_FILE_SESSION_PATH);
      else
	SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dir_num_items);
    } else if( dir_num_items == 0 ) {
      SEQ_LCD_PrintFormattedString("No directories found under %s!", SEQ_FILE_SESSION_PATH);
    } else {
      SEQ_LCD_PrintFormattedString("Open Session (%d found)", dir_num_items);
    }

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintSpaces(40);
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintFormattedString("Current Session: %s/%s", SEQ_FILE_SESSION_PATH, seq_file_session_name);

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);

    SEQ_LCD_PrintList((char *)ui_global_dir_list, SESSION_LIST_ENTRY_WIDTH, dir_num_items, SESSION_NUM_LIST_DISPLAYED_ITEMS, ui_selected_item, dir_view_offset);

    SEQ_LCD_PrintSpaces(36);
    SEQ_LCD_PrintString("EXIT");
  } break;

  case MENU_DIALOG_SAVE_AS:
  case MENU_DIALOG_NEW: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    // "SaveAs":
    // >SAVE AS< Please enter session name:    /SESSIONS/<xxxxxxxx>                    
    // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins   SAVE EXIT

    // "New":
    // >NEW< Please enter name of new session: /SESSIONS/<xxxxxxxx>                    
    // .,!1 ABC2 DEF3 GHI4 JKL5 MNO6 PQRS7 TUV8WXYZ9 -_ 0  Char <>  Del Ins   SAVE EXIT

    int i;

    SEQ_LCD_CursorSet(0, 0);
    if( menu_dialog == MENU_DIALOG_SAVE_AS )
      SEQ_LCD_PrintString(">SAVE AS< Please enter session name:    ");
    else
      SEQ_LCD_PrintString(">NEW< Please enter name of new session: ");

    SEQ_LCD_PrintFormattedString("%s/<", SEQ_FILE_SESSION_PATH);
    for(i=0; i<8; ++i)
      SEQ_LCD_PrintChar(dir_name[i]);
    SEQ_LCD_PrintString(">");
    SEQ_LCD_PrintSpaces(20);

    // insert flashing cursor
    if( ui_cursor_flash ) {
      SEQ_LCD_CursorSet(51 + ui_edit_name_cursor, 0);
      SEQ_LCD_PrintChar('*');
    }

    SEQ_UI_KeyPad_LCD_Msg();
    if( menu_dialog == MENU_DIALOG_SAVE_AS )
      SEQ_LCD_PrintString("  SAVE EXIT");
    else
      SEQ_LCD_PrintString("CREATE EXIT");
  } break;

  case MENU_DIALOG_SAVE_DEXISTS:
  case MENU_DIALOG_NEW_DEXISTS: {
    // layout:
    // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
    // <--------------------------------------><-------------------------------------->
    //                                         '/SESSIONS/xxxxxxxx' already exists     
    //                                         Overwrite? YES  NO                  EXIT
    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintSpaces(40);

    SEQ_LCD_PrintFormattedString("%s/%s already exists!", SEQ_FILE_SESSION_PATH, dir_name);
    SEQ_LCD_PrintSpaces(5+13);

    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintSpaces(40);

    SEQ_LCD_PrintString("Overwrite? YES  NO");
    SEQ_LCD_PrintSpaces(18);
    SEQ_LCD_PrintString("EXIT");
  } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MENU_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  menu_dialog = 0;
  dir_name[0] = 0;

  int selected_page = SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item;
  if( selected_page != ui_selected_page ) {
    int page = ui_selected_page;
    if( page < SEQ_UI_FIRST_MENU_SELECTION_PAGE )
      page = SEQ_UI_FIRST_MENU_SELECTION_PAGE;

    menu_view_offset = page - SEQ_UI_FIRST_MENU_SELECTION_PAGE;
    int max_offset = SEQ_UI_NUM_MENU_PAGES - MENU_NUM_LIST_DISPLAYED_ITEMS;
    if( menu_view_offset >= max_offset ) {
      menu_view_offset = max_offset;
      menu_selected_item = MENU_NUM_LIST_DISPLAYED_ITEMS-1;
    } else {
      menu_selected_item = 0;
    }
  }

  SEQ_UI_MENU_UpdatePageList();


  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Updates list based on selected menu page
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_MENU_UpdatePageList(void)
{
  int item;

  for(item=0; item<MENU_NUM_LIST_DISPLAYED_ITEMS && (item+menu_view_offset)<SEQ_UI_NUM_MENU_PAGES; ++item) {
    int i;

    char *list_item = (char *)&ui_global_dir_list[MENU_LIST_ENTRY_WIDTH*item];
    strcpy(list_item, SEQ_UI_PageNameGet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + item + menu_view_offset));
    for(i=strlen(list_item)-1; i>=0; --i)
      if( list_item[i] == ' ' )
	list_item[i] = 0;
      else
	break;
  }

  while( item < MENU_NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[MENU_LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Updates list based on available sessions
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_MENU_UpdateSessionList(void)
{
  int item;

  MUTEX_SDCARD_TAKE;
    dir_num_items = SEQ_FILE_GetDirs(SEQ_FILE_SESSION_PATH, (char *)&ui_global_dir_list[0], SESSION_NUM_LIST_DISPLAYED_ITEMS, dir_view_offset);
  MUTEX_SDCARD_GIVE;

  if( dir_num_items < 0 )
    item = 0;
  else if( dir_num_items < SESSION_NUM_LIST_DISPLAYED_ITEMS )
    item = dir_num_items;
  else
    item = SESSION_NUM_LIST_DISPLAYED_ITEMS;

  while( item < SESSION_NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[SESSION_LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Stores a session into new directory
/////////////////////////////////////////////////////////////////////////////
static s32 DoSessionSaveOrNew(u8 new_session, u8 force_overwrite)
{
  s32 status;
  int i;
  char path[30];

  // if an error is detected, we jump back to original page
  ui_selected_item = 0;
  menu_dialog = new_session ? MENU_DIALOG_NEW : MENU_DIALOG_SAVE_AS;

  u8 dirname_valid = 1;
  for(i=0; i<8; ++i)
    if( dir_name[i] == '.' || dir_name[i] == '?' || dir_name[i] == ',' || dir_name[i] == '!' )
      dirname_valid = 0;

  if( !dirname_valid ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Name not valid!", "(remove . ? , !)");
    return -1;
  }

  if( dir_name[0] == ' ') {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Please enter", "Session Name!");
    return -2;
  }

  strcpy(path, SEQ_FILE_SESSION_PATH);
  MUTEX_SDCARD_TAKE;
  SEQ_FILE_MakeDir(path); // create directory if it doesn't exist
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -3;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "/SESSIONS directory", "cannot be created!");
    return -4;
  }

  char session_dir[20];
  char *p = (char *)&session_dir[0];
  for(i=0; i<8; ++i) {
    char c = dir_name[i];
    if( c != ' ' )
      *p++ = c;
  }
  *p++ = 0;

  sprintf(path, "%s/%s", SEQ_FILE_SESSION_PATH, session_dir);

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -5;
  }

  if( !force_overwrite && status == 1) {
    // file exists - ask if it should be overwritten in a special dialog page
    menu_dialog = new_session ? MENU_DIALOG_NEW_DEXISTS : MENU_DIALOG_SAVE_DEXISTS;
    return 1;
  }

  SEQ_FILE_MakeDir(path); // create directory

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;
	    
  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -6;
  }

  // take over session name
  strcpy(seq_file_new_session_name, session_dir);
  seq_file_backup_percentage = 0;
  seq_file_copy_percentage = 0; // for percentage display

  if( new_session ) {
    // stop sequencer if it is still running
    if( SEQ_BPM_IsRunning() )
      SEQ_BPM_Stop();
    SEQ_SONG_Reset();
    SEQ_CORE_Reset();
    SEQ_MIDPLY_Reset();

    // clear all patterns/etc.. to have a clean start
    SEQ_CORE_Init(0);
    SEQ_GROOVE_Init();
    SEQ_SONG_Init();
    SEQ_MIXER_Init();

    // formatting handled by low-priority task in app.c
    // messages print in seq_ui.c so long request is active
    seq_ui_format_req = 1;
  } else {
    // Copy operation handled by low-priority task in app.c
    // messages print in seq_ui.c so long request is active
    seq_ui_backup_req = 1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Stores a session into new directory
/////////////////////////////////////////////////////////////////////////////
static s32 DoSessionSave(u8 force_overwrite)
{
  return DoSessionSaveOrNew(0, force_overwrite);
}

/////////////////////////////////////////////////////////////////////////////
// Creates a new session
/////////////////////////////////////////////////////////////////////////////
static s32 DoSessionNew(u8 force_overwrite)
{
  return DoSessionSaveOrNew(1, force_overwrite);
}


/////////////////////////////////////////////////////////////////////////////
// Open a session
/////////////////////////////////////////////////////////////////////////////
static s32 OpenSession(void)
{
  s32 status = 0;

  // get directory name
  char session_dir[20];
  int i;
  char *p = (char *)&session_dir[0];
  for(i=0; i<8; ++i) {
    char c = ui_global_dir_list[SESSION_LIST_ENTRY_WIDTH*ui_selected_item + i];
    if( c != ' ' )
      *p++ = c;
  }
  *p++ = 0;


  // check if directory still exists
  char path[30];
  sprintf(path, "%s/%s", SEQ_FILE_SESSION_PATH, session_dir);

  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_DirExists(path);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    SEQ_UI_SDCardErrMsg(2000, status);
    return -1;
  }

  if( status == 0 ) {
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Session doesn't", "exist anymore?!?");
    return -2;
  }

  // remember previous session and switch to new name
  char prev_session_name[13];
  strcpy(prev_session_name, seq_file_session_name);
  strcpy(seq_file_session_name, session_dir);

  // try to load files
  MUTEX_SDCARD_TAKE;
  status = SEQ_FILE_LoadAllFiles(0); // excluding HW config
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    // session invalid - switch back to previous one!
    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Session is invalid:", seq_file_session_name);
    strcpy(seq_file_session_name, prev_session_name);
    return -3;
  }

  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Changed to Session:", seq_file_session_name);

  // store session name
  MUTEX_SDCARD_TAKE;
  status |= SEQ_FILE_StoreSessionName();
  MUTEX_SDCARD_GIVE;

  return 0; // no error
}
