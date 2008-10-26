// $Id$
/*
 * Demo application for 8xPCD8544 GLCDs
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
#include "app.h"

#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // clear LCD
  MIOS32_LCD_Clear();

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // X/Y "position" of displays (see also comments in $MIOS32_PATH/modules/app_lcd/pcd8544/README.txt)
    const u8 lcd_x[8] = {0, 1, 2, 0, 1, 2, 0, 1}; // CS#0..7
    const u8 lcd_y[8] = {0, 0, 0, 1, 1, 1, 2, 2};

    u8 i;
    for(i=0; i<8; ++i) {
      u8 x_offset = 84*lcd_x[i];
      u8 y_offset = 6*8*lcd_y[i];

      // print text
      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 0*8);
      MIOS32_LCD_PrintFormattedString("  PCD8544 #%d", i+1);

      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 2*8);
      MIOS32_LCD_PrintString("  powered by  ");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
      MIOS32_LCD_GCursorSet(x_offset + 0, y_offset + 3*8);
      MIOS32_LCD_PrintString("MIOS");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
      MIOS32_LCD_GCursorSet(x_offset + 64, y_offset + 4*8);
      MIOS32_LCD_PrintString("32");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
