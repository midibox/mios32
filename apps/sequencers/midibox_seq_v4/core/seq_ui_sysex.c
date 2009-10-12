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


// for debugging list display
#define TEST_LIST 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 dev_num_items;
static u8 dev_view_offset = 0; // only changed once after startup


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
  if( SEQ_UI_SelectListItem(incrementer, dev_num_items, 8, &ui_selected_item, &dev_view_offset) )
    SEQ_UI_SYSEX_UpdateDirList();

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
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {
#endif

    if( button != SEQ_UI_BUTTON_Select )
      ui_selected_item = button / 2;

    char buffer[20];
    int i;
    for(i=0; i<9; ++i)
      buffer[i] = ui_global_dir_list[8*ui_selected_item + i];
    buffer[i] = 0;

    SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Selected:", buffer);

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
  SEQ_LCD_PrintString("Select MIDI Device with GP Button:      ");
  SEQ_LCD_PrintFormattedString("%d Devices found under /sysex            ", dev_num_items);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintList((char *)ui_global_dir_list, 9, 8);

  if( dev_view_offset == 0 && ui_selected_item == 0 )
    SEQ_LCD_PrintChar(0x01); // right arrow
  else if( (dev_view_offset+ui_selected_item+1) >= dev_num_items )
    SEQ_LCD_PrintChar(0x00); // left arrow
  else
    SEQ_LCD_PrintChar(0x02); // left/right arrow


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
  for(item=0; item<8 && item<dev_num_items; ++item) {
    char *list_item = (char *)&ui_global_dir_list[9*item];
    sprintf(list_item, "test%d", item + dev_view_offset);
  }
#else
#endif

  while( item < 8 ) {
    ui_global_dir_list[9*item] = 0;
    ++item;
  }

  return 0; // no error
}
