// $Id$
/*
 * Fx menu page
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



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// following pages are directly accessible with the GP buttons inside the Fx menu
static const seq_ui_page_t shortcut_menu_pages[16] = {
  SEQ_UI_PAGE_FX_ECHO,     // GP1
  SEQ_UI_PAGE_NONE,        // GP2
  SEQ_UI_PAGE_NONE,        // GP3
  SEQ_UI_PAGE_NONE,        // GP4
  SEQ_UI_PAGE_NONE,        // GP5
  SEQ_UI_PAGE_NONE,        // GP6
  SEQ_UI_PAGE_NONE,        // GP7
  SEQ_UI_PAGE_NONE,        // GP8
  SEQ_UI_PAGE_NONE,        // GP9
  SEQ_UI_PAGE_NONE,        // GP10
  SEQ_UI_PAGE_NONE,        // GP11
  SEQ_UI_PAGE_NONE,        // GP12
  SEQ_UI_PAGE_NONE,        // GP13
  SEQ_UI_PAGE_NONE,        // GP14
  SEQ_UI_PAGE_NONE,        // GP15
  SEQ_UI_PAGE_NONE         // GP16
};


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    // change to new page
    if( shortcut_menu_pages[encoder] != SEQ_UI_PAGE_NONE )
      SEQ_UI_PageSet(shortcut_menu_pages[encoder]);

    return 1; // value changed
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
    // change to new page
    if( shortcut_menu_pages[button] != SEQ_UI_PAGE_NONE )
      SEQ_UI_PageSet(shortcut_menu_pages[button]);

    return 1; // value always changed
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

  // print available menu pages
  SEQ_LCD_CursorSet(0, 0);
  //                      <-------------------------------------->
  //                      0123456789012345678901234567890123456789
  SEQ_LCD_PrintString("Fx Pages:");
  SEQ_LCD_PrintSpaces(32 + 40);

  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintString("Echo");
  SEQ_LCD_PrintSpaces(36 + 40);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
