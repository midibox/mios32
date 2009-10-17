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

#define NUM_OF_ITEMS           4
#define ITEM_LIST1             0
#define ITEM_LIST2             1
#define ITEM_LIST3             2
#define ITEM_LIST4             3

#define NUM_LIST_DISPLAYED_ITEMS NUM_OF_ITEMS
#define LIST_ENTRY_WIDTH 19


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 menu_view_offset = 0; // only changed once after startup or if menu page outside view
static u8 menu_selected_item = 0; // dito


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_UI_MENU_UpdatePageList(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = (15 << (4*menu_selected_item));

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
  // any encoder changes the menu view
  if( SEQ_UI_SelectListItem(incrementer, SEQ_UI_NUM_MENU_PAGES, NUM_LIST_DISPLAYED_ITEMS, &menu_selected_item, &menu_view_offset) )
    SEQ_UI_MENU_UpdatePageList();

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
  if( depressed ) return 0; // ignore when button depressed

  if( button <= SEQ_UI_BUTTON_GP16 || button == SEQ_UI_BUTTON_Select ) {
    if( button != SEQ_UI_BUTTON_Select )
      menu_selected_item = button / 4;

    SEQ_UI_PageSet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item);
    return 1;
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

    case SEQ_UI_BUTTON_Exit:
      SEQ_UI_PageSet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item);
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
  SEQ_LCD_PrintString("Menu Page Browser:");
  SEQ_LCD_PrintSpaces(16);

  SEQ_LCD_CursorSet(40, 0);
  SEQ_LCD_PrintSpaces(8);
  SEQ_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
  SEQ_LCD_PrintSpaces(9);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintList((char *)ui_global_dir_list, LIST_ENTRY_WIDTH, SEQ_UI_NUM_MENU_PAGES, NUM_LIST_DISPLAYED_ITEMS, menu_selected_item, menu_view_offset);

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

  int selected_page = SEQ_UI_FIRST_MENU_SELECTION_PAGE + menu_view_offset + menu_selected_item;
  if( selected_page != ui_selected_page ) {
    int page = ui_selected_page;
    if( page < SEQ_UI_FIRST_MENU_SELECTION_PAGE )
      page = SEQ_UI_FIRST_MENU_SELECTION_PAGE;
    menu_view_offset = page - SEQ_UI_FIRST_MENU_SELECTION_PAGE;
    menu_selected_item = 0;
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

  for(item=0; item<NUM_LIST_DISPLAYED_ITEMS && (item+menu_view_offset)<SEQ_UI_NUM_MENU_PAGES; ++item) {
    int i;

    char *list_item = (char *)&ui_global_dir_list[LIST_ENTRY_WIDTH*item];
    strcpy(list_item, SEQ_UI_PageNameGet(SEQ_UI_FIRST_MENU_SELECTION_PAGE + item + menu_view_offset));
    for(i=strlen(list_item)-1; i>=0; --i)
      if( list_item[i] == ' ' )
	list_item[i] = 0;
      else
	break;
  }

  while( item < NUM_LIST_DISPLAYED_ITEMS ) {
    ui_global_dir_list[LIST_ENTRY_WIDTH*item] = 0;
    ++item;
  }

  return 0; // no error
}
