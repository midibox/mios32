// $Id$
/*
 * TODO page (empty stumb)
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


/////////////////////////////////////////////////////////////////////////////
// Local GP LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_LED_Handler(u16 *gp_leds, u16 *gp_leds_flashing)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local GP button callback function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_Button_Handler(u32 pin, s32 depressed)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local GP encoder callback function
/////////////////////////////////////////////////////////////////////////////
static s32 GP_Encoder_Handler(u32 encoder, s32 incrementer)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( !high_prio )
    return 0; // there are no high-priority updates

  // print TODO message
  int lcd;
  for(lcd=0; lcd<2; ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("TODO page");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TODO_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallGPButtonCallback(GP_Button_Handler);
  SEQ_UI_InstallGPEncoderCallback(GP_Encoder_Handler);
  SEQ_UI_InstallGPLEDCallback(GP_LED_Handler);
  SEQ_UI_InstallGPLCDCallback(LCD_Handler);

  return 0; // no error
}
