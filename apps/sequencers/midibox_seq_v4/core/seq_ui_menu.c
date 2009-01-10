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

#include <seq_midi_out.h>

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_file.h"



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
  if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    u8 tmp = ui_selected_page; // required conversion from ui_page_t to u8
    if( SEQ_UI_Var8_Inc(&tmp, SEQ_UI_FIRST_MENU_SELECTION_PAGE, SEQ_UI_PAGES-1, incrementer) ) {
      ui_selected_page = tmp;
      return 1; // value changed
    }
    return 0; // value not changed
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
    case SEQ_UI_BUTTON_GP12: // clears max counter
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      seq_midi_out_max_allocated = 0;
      return 1;

    case SEQ_UI_BUTTON_GP15: // clears drop counter
    case SEQ_UI_BUTTON_GP16:
      seq_midi_out_dropouts = 0;
      return 1;

    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);

    case SEQ_UI_BUTTON_Exit:
      SEQ_UI_PageSet(ui_selected_page);
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //                     Select Menu Page:   MIDI Scheduler: Alloc xxx/xxx Drops: xxx
  //                     xxxxxxxxxxxxxxxxxx<>SD Card: not available                  


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(0);

  MIOS32_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintSpaces(20);
  MIOS32_LCD_PrintString("Select Menu Page:   ");
  MIOS32_LCD_CursorSet(0, 1);
  SEQ_LCD_PrintSpaces(20);
  MIOS32_LCD_PrintString(SEQ_UI_PageNameGet(ui_selected_page));
  MIOS32_LCD_PrintChar((ui_selected_page == SEQ_UI_FIRST_MENU_SELECTION_PAGE) ? ' ' : '<');
  MIOS32_LCD_PrintChar((ui_selected_page < (SEQ_UI_PAGES-1)) ? '>' : ' ');


  ///////////////////////////////////////////////////////////////////////////
  MIOS32_LCD_DeviceSet(1);

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintFormattedString("MIDI Scheduler: Alloc %3d/%3d Drops: %3d",
    seq_midi_out_allocated, seq_midi_out_max_allocated, seq_midi_out_dropouts);

  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintFormattedString("SD Card: ");
  char status_str[128];
  if( !SEQ_FILE_VolumeAvailable() ) {
    sprintf(status_str, "not connected");
  } else {
#if 0
    // disabled: determing the number of free bytes takes too long!
    sprintf(status_str, "%d/%d MB", 
	    SEQ_FILE_VolumeBytesFree()/1000000,
	    SEQ_FILE_VolumeBytesTotal()/1000000);
#else
    // disabled: determing the number of free bytes takes too long!
    sprintf(status_str, "connected (%d MB)", SEQ_FILE_VolumeBytesTotal()/1000000);
#endif
  }
  SEQ_LCD_PrintStringPadded(status_str, 31);

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

  return 0; // no error
}
