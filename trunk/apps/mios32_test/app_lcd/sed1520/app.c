// $Id$
/*
 * Demo application for 8xSSD1306 OLEDs
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
#define MAX_LCDS 2
  int num_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y;
  if( num_lcds > MAX_LCDS )
    num_lcds = MAX_LCDS;

  // clear LCDs
  {
    u8 n;
    for(n=0; n<num_lcds; ++n) {
      MIOS32_LCD_DeviceSet(n);
      MIOS32_LCD_Clear();
    }
  }

  u8 vmeter_icon_ctr[MAX_LCDS][2] = {{0,5},{3,14}}; // memo: 28 icons (14 used)
  u8 vmeter_icon_dir[MAX_LCDS][2] = {{1,1},{1,1}};
  u8 vmeter_icon_delay_ctr[MAX_LCDS][2] = {{1,4},{1,4}};
  const u8 vmeter_icon_x[2] = {0, 114}; // memo: icon width 8
  const u8 vmeter_icon_y[2] = {8, 8}; // memo: icon height 32

  u8 hmeter_icon_ctr[MAX_LCDS][2] = {{6,11},{2,27}}; // memo: 28 icons (14 used)
  u8 hmeter_icon_dir[MAX_LCDS][2] = {{1,0},{1,0}};
  u8 hmeter_icon_delay_ctr[MAX_LCDS][2] = {{4,2},{4,2}};
  const u8 hmeter_icon_x[2] = {20, 80}; // memo: icon width 28
  const u8 hmeter_icon_y[2] = {24, 24}; // memo: icon height 8

  // print configured LCD parameters
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("SED1520 Demo started.");
  MIOS32_MIDI_SendDebugMessage("Configured LCD Parameters in MIOS32 Bootloader Info Range:\n");
  MIOS32_MIDI_SendDebugMessage("lcd_type: 0x%02x (%s)\n", mios32_lcd_parameters.lcd_type, MIOS32_LCD_LcdTypeName(mios32_lcd_parameters.lcd_type));
  MIOS32_MIDI_SendDebugMessage("num_x:    %4d\n", mios32_lcd_parameters.num_x);
  MIOS32_MIDI_SendDebugMessage("num_y:    %4d\n", mios32_lcd_parameters.num_y);
  MIOS32_MIDI_SendDebugMessage("width:    %4d\n", mios32_lcd_parameters.width);
  MIOS32_MIDI_SendDebugMessage("height:   %4d\n", mios32_lcd_parameters.height);

  if( mios32_lcd_parameters.lcd_type != MIOS32_LCD_TYPE_GLCD_SED1520 ) {
    // print warning if correct LCD hasn't been selected
    MIOS32_MIDI_SendDebugMessage("WARNING: your core module hasn't been configured for the SSD1306 GLCD!\n");
    MIOS32_MIDI_SendDebugMessage("Please do this with the bootloader update application!\n");
  }



  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // wait some mS
    MIOS32_DELAY_Wait_uS(10000);

    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    u8 n;
    for(n=0; n<num_lcds; ++n) {
      int i;
      u8 x_offset = 0;
      u8 y_offset = 0;
      MIOS32_LCD_DeviceSet(n);

      // print text
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
      MIOS32_LCD_GCursorSet(x_offset + 0*6, y_offset + 0*8);
      MIOS32_LCD_PrintFormattedString("SED1520#%d powered by", n+1);

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
      MIOS32_LCD_GCursorSet(x_offset + 2*6+3, y_offset + 1*8);
      MIOS32_LCD_PrintString("MIOS32");

      // print vmeter icons
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_V); // memo: 28 icons, 14 used, icon size: 8x32
      for(i=0; i<2; ++i) {
	if( ++vmeter_icon_delay_ctr[n][i] ) {
	  vmeter_icon_delay_ctr[n][i] = 0;
	  if( vmeter_icon_dir[n][i] ) {
	    if( ++vmeter_icon_ctr[n][i] >= 13 )
	      vmeter_icon_dir[n][i] = 0;
	  } else {
	    if( --vmeter_icon_ctr[n][i] < 1 )
	      vmeter_icon_dir[n][i] = 1;
	  }
	}
	MIOS32_LCD_GCursorSet(vmeter_icon_x[i]+x_offset, vmeter_icon_y[i]+y_offset);
	MIOS32_LCD_PrintChar(vmeter_icon_ctr[n][i]);
      }

#if 0
      // print hmeter icons
      for(i=0; i<2; ++i) {
	MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_H); // memo: 28 icons, 14 used, icon size: 28x8
	if( ++hmeter_icon_delay_ctr[n][i] > 7 ) {
	  hmeter_icon_delay_ctr[n][i] = 0;
	  if( hmeter_icon_dir[n][i] ) {
	    if( ++hmeter_icon_ctr[n][i] >= 13 )
	      hmeter_icon_dir[n][i] = 0;
	  } else {
	    if( --hmeter_icon_ctr[n][i] < 1 )
	      hmeter_icon_dir[n][i] = 1;
	  }
	}
	MIOS32_LCD_GCursorSet(hmeter_icon_x[i]+x_offset, hmeter_icon_y[i]+y_offset);
	MIOS32_LCD_PrintChar(hmeter_icon_ctr[n][i]);
      }
#endif
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
