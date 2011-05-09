// $Id$
/*
 * Application specific OLED driver for up to 8 * SSD1306 (every display 
 * provides a resolution of 128x64)
 * Referenced from MIOS32_LCD routines
 *
 * This display can be ordered at:
 *
 * This driver allows to drive up to 8 of them, every display is connected
 * to a dedicated chip select line at LCD port J15. They can be addressed with
 * following (graphical) cursor positions:
 * 
 * 
 * CS at J15.D0      CS at J15.D1
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =   0..  7 |  | Y =   0..  7 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * CS at J15.D2      CS at J15.D3
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =   8.. 15 |  | Y =   8.. 15 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * CS at J15.D4      CS at J15.D5
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =  16.. 23 |  | Y =  16.. 23 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * CS at J15.D6      CS at J15.D7
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =  24.. 31 |  | Y =  24.. 31 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <glcd_font.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <glcd_font.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef APP_LCD_OUTPUT_MODE
#define APP_LCD_OUTPUT_MODE  0
#endif

// for LPC17 only: should J10 be used for CS lines
// This option is nice if no J15 shiftregister is connected to a LPCXPRESSO.
// This shiftregister is available on the MBHP_CORE_LPC17 module
#ifndef APP_LCD_USE_J10_FOR_CS
#define APP_LCD_USE_J10_FOR_CS 1
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE) < 0 )
    return -2; // failed to initialize J15

#if APP_LCD_USE_J10_FOR_CS
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, APP_LCD_OUTPUT_MODE ? MIOS32_BOARD_PIN_MODE_OUTPUT_OD : MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#endif

  // set LCD type
  mios32_lcd_type = MIOS32_LCD_TYPE_GLCD;

  // initialize LCD
  APP_LCD_Cmd(0xa8); // Set MUX Ratio
  APP_LCD_Cmd(0x3f);

  APP_LCD_Cmd(0xd3); // Set Display Offset
  APP_LCD_Cmd(0x00);

  APP_LCD_Cmd(0x40); // Set Display Start Line

  APP_LCD_Cmd(0xa0); // Set Segment re-map

  APP_LCD_Cmd(0xc0); // Set COM Output Scan Direction

  APP_LCD_Cmd(0xda); // Set COM Pins hardware configuration
  APP_LCD_Cmd(0x12);

  APP_LCD_Cmd(0x81); // Set Contrast Control
  APP_LCD_Cmd(0x7f); // middle

  APP_LCD_Cmd(0xa4); // Disable Entiere Display On

  APP_LCD_Cmd(0xa6); // Set Normal Display

  APP_LCD_Cmd(0xd5); // Set OSC Frequency
  APP_LCD_Cmd(0x80);

  APP_LCD_Cmd(0x8d); // Enable charge pump regulator
  APP_LCD_Cmd(0x14);

  APP_LCD_Cmd(0xaf); // Display On

  APP_LCD_Cmd(0x20); // Enable Page mode
  APP_LCD_Cmd(0x02);

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // select LCD depending on current cursor position
  // THIS PART COULD BE CHANGED TO ARRANGE THE 8 DISPLAYS ON ANOTHER WAY
  u8 line = mios32_lcd_y / APP_LCD_HEIGHT;
  u8 row = (mios32_lcd_x % (2*APP_LCD_WIDTH)) / APP_LCD_WIDTH;

  u8 cs = 2*line + row;

  if( cs >= 8 )
    return -1; // invalid CS line

  // chip select and DC
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(~(1 << cs));
#else
  MIOS32_BOARD_J15_DataSet(~(1 << cs));
#endif
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

  // send data
  MIOS32_BOARD_J15_SerDataShift(data);

  // increment graphical cursor
  ++mios32_lcd_x;

  // if end of display segment reached: set X position of all segments to 0
  if( (mios32_lcd_x % APP_LCD_WIDTH) == 0 ) {
    APP_LCD_Cmd(0x00); // set X=0
    APP_LCD_Cmd(0x10);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // select all LCDs
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(0x00);
#else
  MIOS32_BOARD_J15_DataSet(0x00);
#endif
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

  MIOS32_BOARD_J15_SerDataShift(cmd);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  s32 error = 0;
  u8 x, y;

  // use default font
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);

  // send data
  for(y=0; y<8; ++y) {
    error |= MIOS32_LCD_CursorSet(0, y);

    // select all LCDs
#if APP_LCD_USE_J10_FOR_CS
    MIOS32_BOARD_J10_Set(0x00);
#else
    MIOS32_BOARD_J15_DataSet(0x00);
#endif
    MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

    for(x=0; x<128; ++x)
      MIOS32_BOARD_J15_SerDataShift(0x00);
  }

  // set X=0, Y=0
  error |= MIOS32_LCD_CursorSet(0, 0);

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  s32 error = 0;

#if 0
  // exit with error if line is not in allowed range
  if( line >= MIOS32_LCD_MAX_MAP_LINES )
    return -1;

  // exit if mapped line >= 8 (not supported by KS0108)
  u8 mapped_line = mios32_lcd_cursor_map[line];
  if( mapped_line >= 8 )
    return -2;

  // ...

  return error;
#else
  // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
  return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  s32 error = 0;

  // set X position
  error |= APP_LCD_Cmd(0x00 | (x & 0xf));
  error |= APP_LCD_Cmd(0x10 | ((x>>4) & 0xf));

  // set Y position
  error |= APP_LCD_Cmd(0xb0 | ((y>>3) & 7));

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  // TODO

  return -1; // not implemented yet
}


/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb)
{
  return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  if( x >= bitmap.width || y >= bitmap.height )
    return -1; // pixel is outside bitmap

  u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
  u8 mask = 1 << (y % 8);

  *pixel &= ~mask;
  if( colour )
    *pixel |= mask;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  int line;
  int y_lines = (bitmap.height >> 3);

  for(line=0; line<y_lines; ++line) {

    // calculate pointer to bitmap line
    u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

    // set graphical cursor after second line has reached
    if( line > 0 ) {
      mios32_lcd_x -= bitmap.width;
      mios32_lcd_y += 8;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

    // transfer bitmap
    int x;
    for(x=0; x<bitmap.width; ++x)
      APP_LCD_Data(*memory_ptr++);
  }

  // fix graphical cursor if more than one line has been print
  if( y_lines >= 1 ) {
    mios32_lcd_y = mios32_lcd_y - (bitmap.height-8);
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
  }

  return 0; // no error
}
