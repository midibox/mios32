// $Id$
/*
 * SysEx page
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
#include "tasks.h"

#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           8
#define ITEM_DEV1              0
#define ITEM_DEV2              1
#define ITEM_DEV3              2
#define ITEM_DEV4              3
#define ITEM_DEV5              4
#define ITEM_DEV6              5
#define ITEM_DEV7              6
#define ITEM_DEV8              7

#define LIST_ENTRY_WIDTH 9

// for debugging list display
#define TEST_LIST 0



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 dev_num_items; // contains SEQ_FILE error status if < 0
static u8 dev_view_offset = 0; // only changed once after startup
static char dev_name[12]; // directory name of device (first char is 0 if no device selected)

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_UI_SYSEX_ReadDir(void);
static s32 SEQ_UI_SYSEX_UpdateDirList(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = (3 << (2*ui_selected_item));

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
  // any encoder changes the dir view
  if( !dev_name[0] ) {
    if( SEQ_UI_SelectListItem(incrementer, dev_num_items, NUM_OF_ITEMS, &ui_selected_item, &dev_view_offset) )
      SEQ_UI_SYSEX_UpdateDirList();
  } else {
    if( SEQ_UI_SelectListItem(incrementer, dev_num_items, NUM_OF_ITEMS-1, &ui_selected_item, &dev_view_offset) )
      SEQ_UI_SYSEX_UpdateDirList();
  }

  return 1;
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
  if( depressed ) return -1; // ignore when button depressed, -1 ensures that message still print

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {
#endif

    if( button != SEQ_UI_BUTTON_Select )
      ui_selected_item = button / 2;

#if TEST_LIST
    char buffer[20];
    int i;
    for(i=0; i<LIST_ENTRY_WIDTH; ++i)
      buffer[i] = ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item + i];
    buffer[i] = 0;

    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Selected:", buffer);
#else
    if( dev_name[0] == 0 && dev_num_items >= 0 ) {
      // Select MIDI Device
      memcpy((char *)dev_name, (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item], 8);
      dev_name[8] = 0;
      ui_selected_item = 0;
      dev_view_offset = 0;
      SEQ_UI_SYSEX_UpdateDirList();
    } else {
      if( ui_selected_item == (NUM_OF_ITEMS-1) ) {
	// Exit...
	dev_name[0] = 0;
	ui_selected_item = 0;
	dev_view_offset = 0;
	SEQ_UI_SYSEX_UpdateDirList();
      } else if( dev_num_items >= 1 && (ui_selected_item+dev_view_offset) < dev_num_items ) {
	// Send SysEx Dump
	char syx_file[20];
	memcpy((char *)syx_file, (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*ui_selected_item], 8);
	memcpy((char *)syx_file+8, ".syx", 5);
	SEQ_UI_Msg((ui_selected_item < 4) ? SEQ_UI_MSG_USER : SEQ_UI_MSG_USER_R, 10000, "Sending:", syx_file);
	MUTEX_SDCARD_TAKE;
	MUTEX_MIDIOUT_TAKE;
	s32 status = SEQ_FILE_SendSyxDump(DEFAULT, dev_name, syx_file); // TODO: MIDI port selection (via config file or GP encoder?)
	MUTEX_MIDIOUT_GIVE;
	MUTEX_SDCARD_GIVE;
	if( status < 0 )
	  SEQ_UI_SDCardErrMsg(2000, status);
	else {
	  char buffer[20];
	  sprintf(buffer, "Sent %d bytes", status);
	  SEQ_UI_Msg((ui_selected_item < 4) ? SEQ_UI_MSG_USER : SEQ_UI_MSG_USER_R, 10000, buffer, syx_file);
	}
      }
    }
#endif

    return 1;
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
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
  // Select MIDI Device with GP Button:      2 Devices found under /sysex            
  //   MBSID     MBFM  ....


  // Select MIDI Device with GP Button:      10 Devices found under /sysex           
  //  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx>



  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintSpaces(80);

  SEQ_LCD_CursorSet(0, 0);
  if( dev_num_items < 0 ) {
    if( dev_num_items == SEQ_FILE_ERR_NO_SYSEX_DIR )
      SEQ_LCD_PrintString("sysex/ directory not found on SD Card!");
    else if( dev_name[0] != 0 && dev_num_items == SEQ_FILE_ERR_NO_SYSEX_DEV_DIR )
      SEQ_LCD_PrintFormattedString("sysex/%s directory not found on SD Card!", dev_name);
    else
      SEQ_LCD_PrintFormattedString("SD Card Access Error: %d", dev_num_items);
  } else if( dev_num_items == 0 ) {
    if( dev_name[0] != 0 )
      SEQ_LCD_PrintFormattedString("No .syx files found under sysex/%s!", dev_name);
    else
      SEQ_LCD_PrintString("No subdirectories found in sysex/ directory on SD Card!");
  } else {
    if( dev_name[0] != 0 ) {
      SEQ_LCD_PrintString("Select .syx file with GP Button:");
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintFormattedString("%d files found under /sysex/%s", dev_num_items, dev_name);
    } else {
      SEQ_LCD_PrintString("Select MIDI Device with GP Button:");
      SEQ_LCD_CursorSet(40, 0);
      SEQ_LCD_PrintFormattedString("%d Devices found under /sysex", dev_num_items);
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( !dev_name[0] ) {
    SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dev_num_items, NUM_OF_ITEMS, ui_selected_item, dev_view_offset);
  } else {
    SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, dev_num_items, NUM_OF_ITEMS-1, ui_selected_item, dev_view_offset);
    SEQ_LCD_PrintString("      EXIT");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SYSEX_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  dev_name[0] = 0;

  SEQ_UI_SYSEX_ReadDir();
  SEQ_UI_SYSEX_UpdateDirList();

  return 0; // no error
}


static s32 SEQ_UI_SYSEX_ReadDir(void)
{
#if TEST_LIST
  dev_num_items = 32;
#else
#endif

  return 0; // no error
}

static s32 SEQ_UI_SYSEX_UpdateDirList(void)
{
  int item;

#if TEST_LIST
  for(item=0; item<NUM_OF_ITEMS && item<dev_num_items; ++item) {
    char *list_item = (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*item];
    sprintf(list_item, "test%d", item + dev_view_offset);
  }
#else
  MUTEX_SDCARD_TAKE;
  if( !dev_name[0] )
    dev_num_items = SEQ_FILE_GetSysExDirs((char *)&ui_global_dir_list[0], NUM_OF_ITEMS, dev_view_offset);
  else
    dev_num_items = SEQ_FILE_GetSysExDumps((char *)&ui_global_dir_list[0], NUM_OF_ITEMS-1, dev_view_offset, dev_name);
  MUTEX_SDCARD_GIVE;

  if( dev_num_items < 0 )
    item = 0;
  else if( dev_num_items < NUM_OF_ITEMS )
    item = dev_num_items;
  else
    item = NUM_OF_ITEMS;
#endif

  while( item < NUM_OF_ITEMS ) {
    ui_global_dir_list[LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}
