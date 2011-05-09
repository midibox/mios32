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
  // X/Y "position" of displays (see also comments in $MIOS32_PATH/modules/app_lcd/pcd8544/README.txt)
  const u8 lcd_x[8] = {0, 1, 0, 1, 0, 1, 0, 1}; // CS#0..7
  const u8 lcd_y[8] = {0, 0, 1, 1, 2, 2, 3, 3};

  u8 vmeter_icon_ctr[8][2] = {{0,5},{3,14},{7,1},{3,9},{13,6},{10,2},{1,4},{6,2}}; // memo: 28 icons (14 used)
  u8 vmeter_icon_dir[8][2] = {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}};
  u8 vmeter_icon_delay_ctr[8][2] = {{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4},{1,4}};
  const u8 vmeter_icon_x[2] = {0, 120}; // memo: icon width 8
  const u8 vmeter_icon_y[2] = {12, 12}; // memo: icon height 32

  u8 hmeter_icon_ctr[8][2] = {{6,11},{2,27},{23,1},{15,6},{18,9},{10,12},{3,25},{26,7}}; // memo: 28 icons (14 used)
  u8 hmeter_icon_dir[8][2] = {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}};
  u8 hmeter_icon_delay_ctr[8][2] = {{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2},{4,2}};
  const u8 hmeter_icon_x[2] = {20, 80}; // memo: icon width 28
  const u8 hmeter_icon_y[2] = {60, 60}; // memo: icon height 8

  // print static screen
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // clear LCD
  MIOS32_LCD_Clear();

  // endless loop - LED will flicker on each iteration
  while( 1 ) {
    // wait some mS
    MIOS32_DELAY_Wait_uS(10000);

    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    u8 n;
    for(n=0; n<8; ++n) {
      int i;
      // X/Y "position" of displays (see also comments in $MIOS32_PATH/modules/app_lcd/ssd1306/README.txt)
      u8 x_offset = 128*lcd_x[n];
      u8 y_offset = 64*lcd_y[n];

      // print text
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
      MIOS32_LCD_GCursorSet(x_offset + 6*6, y_offset + 1*8);
      MIOS32_LCD_PrintFormattedString("SSD1306 #%d", n+1);

      MIOS32_LCD_GCursorSet(x_offset + 6*6, y_offset + 2*8);
      MIOS32_LCD_PrintString("powered by    ");

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_BIG);
      MIOS32_LCD_GCursorSet(x_offset + 3*6, y_offset + 3*8);
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

	MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	if( i == 0 ) {
	  MIOS32_LCD_GCursorSet(0+x_offset, hmeter_icon_y[i]+y_offset);
	  MIOS32_LCD_PrintFormattedString("%d", hmeter_icon_ctr[n][i]*4);
	} else {
	  MIOS32_LCD_GCursorSet(128-3*6+x_offset, hmeter_icon_y[i]+y_offset);
	  MIOS32_LCD_PrintFormattedString("%3d", hmeter_icon_ctr[n][i]*4);
	}
      }
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
