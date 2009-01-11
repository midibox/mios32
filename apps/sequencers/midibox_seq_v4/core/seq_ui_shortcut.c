// $Id$
/*
 * Shortcut page (visible when MENU button is pressed)
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

// following pages are directly accessible with the GP buttons when MENU button is pressed
static const seq_ui_page_t shortcut_menu_pages[16] = {
  SEQ_UI_PAGE_MIXER,       // GP1
  SEQ_UI_PAGE_TRKEVNT,     // GP2
  SEQ_UI_PAGE_TRKMODE,     // GP3
  SEQ_UI_PAGE_TRKDIR,      // GP4
  SEQ_UI_PAGE_TRKDIV,      // GP5
  SEQ_UI_PAGE_TRKLEN,      // GP6
  SEQ_UI_PAGE_TRKTRAN,     // GP7
  SEQ_UI_PAGE_NONE,        // GP8
  SEQ_UI_PAGE_TRGASG,      // GP9
  SEQ_UI_PAGE_FX,          // GP10
  SEQ_UI_PAGE_NONE,        // GP11
  SEQ_UI_PAGE_NONE,        // GP12
  SEQ_UI_PAGE_BPM,         // GP13
  SEQ_UI_PAGE_SAVE,        // GP14
  SEQ_UI_PAGE_NONE,        // GP15
  SEQ_UI_PAGE_NONE         // GP16
};


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  int i;
  for(i=0; i<16; ++i)
    if(ui_shortcut_prev_page == shortcut_menu_pages[i])
      *gp_leds |= (1 << i);

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
  SEQ_LCD_PrintString("Menu Shortcuts:");
  SEQ_LCD_PrintSpaces(25 + 40);
  SEQ_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintString("Mix  Evnt Mode Dir. Div. Len. Trn. Grv. Trg.  Fx  Man. Mrp. BPM  Save MIDI SysEx");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_SHORTCUT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
