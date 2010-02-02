// $Id: app_lcd.c 775 2009-11-14 18:42:41Z tk $
/*
 * Application specific CLCD driver for Juce CLCD Emulation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Phil Taylor (phil@taylor.org.uk)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////



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

  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // initialize LCD
  APP_LCD_Cmd(0x08); // Display Off
  APP_LCD_Cmd(0x0c); // Display On
  APP_LCD_Cmd(0x06); // Entry Mode
  APP_LCD_Cmd(0x01); // Clear Display

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // Call Juce specific LCD_Data function.
  lcdData(data);
	
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // Call Juce specific LCD_Cmd function.
  lcdCmd(cmd);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  // -> send clear command
  return APP_LCD_Cmd(0x01);
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // exit with error if line is not in allowed range
  if( line >= MIOS32_LCD_MAX_MAP_LINES )
    return -1;

  // -> set cursor address
  return APP_LCD_Cmd(0x80 | (mios32_lcd_cursor_map[line] + column));
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  // n.a.
  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  s32 i;

  // send character number
  APP_LCD_Cmd(((num&7)<<3) | 0x40);

  // send 8 data bytes
  for(i=0; i<8; ++i)
    if( APP_LCD_Data(table[i]) < 0 )
      return -1; // error during sending character

  // set cursor to original position
  return APP_LCD_CursorSet(mios32_lcd_column, mios32_lcd_line);
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
  return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  return -1; // n.a.
}
