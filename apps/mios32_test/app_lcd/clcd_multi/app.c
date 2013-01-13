// $Id$
/*
 * Demo application for Multiple CLCDs
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
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


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static const u8 charset_vert_bars[8*8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
};


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
  int num_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y;

  // print configured LCD parameters
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Multi-CLCD Demo started.");
  MIOS32_MIDI_SendDebugMessage("Configured LCD Parameters in MIOS32 Bootloader Info Range:\n");
  MIOS32_MIDI_SendDebugMessage("lcd_type: 0x%02x (%s)\n", mios32_lcd_parameters.lcd_type, MIOS32_LCD_LcdTypeName(mios32_lcd_parameters.lcd_type));
  MIOS32_MIDI_SendDebugMessage("num_x:    %4d\n", mios32_lcd_parameters.num_x);
  MIOS32_MIDI_SendDebugMessage("num_y:    %4d\n", mios32_lcd_parameters.num_y);
  MIOS32_MIDI_SendDebugMessage("width:    %4d\n", mios32_lcd_parameters.width);
  MIOS32_MIDI_SendDebugMessage("height:   %4d\n", mios32_lcd_parameters.height);

  if( mios32_lcd_parameters.lcd_type != MIOS32_LCD_TYPE_CLCD &&
      mios32_lcd_parameters.lcd_type != MIOS32_LCD_TYPE_CLCD_DOG ) {
    // print warning if correct LCD hasn't been selected
    MIOS32_MIDI_SendDebugMessage("WARNING: your core module hasn't been configured for CLCD or CLCD_DOG!\n");
    MIOS32_MIDI_SendDebugMessage("Please do this with the bootloader update application!\n");
  }

  // initialize remaining CLCDs (programming_models/traditional/main.c will only initialize the first two)
  int lcd;
  for(lcd=0; lcd<num_lcds; ++lcd) {
    MIOS32_MIDI_SendDebugMessage("Initialize LCD #%d\n", lcd);
    MIOS32_LCD_DeviceSet(lcd);
    if( MIOS32_LCD_Init(0) < 0 ) {
      MIOS32_MIDI_SendDebugMessage("Failed - no response from CLCD #%d.%d\n",
				   (lcd % mios32_lcd_parameters.num_x) + 1,
				   (lcd / mios32_lcd_parameters.num_x) + 1);
    }
  }

  // init special characters for all LCDs
  for(lcd=0; lcd<num_lcds; ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_SpecialCharsInit((u8 *)charset_vert_bars);
    MIOS32_LCD_Clear();
  }

  // print text on all LCDs
  for(lcd=0; lcd<num_lcds; ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("CLCD #%d.%d",
				    (lcd % mios32_lcd_parameters.num_x) + 1,
				    (lcd / mios32_lcd_parameters.num_x) + 1);
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("READY.");
  }


  // print animated vertical bar
  while( 1 ) {
    int i, j;
    for(i=0; i<8; ++i) {
      // print vertical bars depending on i
      for(lcd=0; lcd<num_lcds; ++lcd) {
	MIOS32_LCD_DeviceSet(lcd);
	MIOS32_LCD_CursorSet(12, 0);

	for(j=0; j<8; ++j) {
	  u8 c = (i + j) % 8;
	  MIOS32_LCD_PrintChar(c);
	}
      }

      // wait for 100 mS
      for(j=0; j<100; ++j)
	MIOS32_DELAY_Wait_uS(1000);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
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
