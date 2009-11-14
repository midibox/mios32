// $Id$
/*
 * Application specific CLCD driver for HD44780 (or compatible) character LCDs
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

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#define APP_LCD_OUTPUT_MODE  1


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

  // enable display by default
  display_available |= (1 << mios32_lcd_device);

  // initialize LCD
  MIOS32_BOARD_J15_DataSet(0x38);
  MIOS32_BOARD_J15_RS_Set(0);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);
  MIOS32_BOARD_J15_RW_Set(0);
#ifdef MIOS32_DONT_USE_DELAY
  u32 delay;
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);
#ifdef MIOS32_DONT_USE_DELAY
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);
#ifdef MIOS32_DONT_USE_DELAY
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  APP_LCD_Cmd(0x08); // Display Off
  APP_LCD_Cmd(0x0c); // Display On
  APP_LCD_Cmd(0x06); // Entry Mode
  APP_LCD_Cmd(0x01); // Clear Display
#ifdef MIOS32_DONT_USE_DELAY
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  // for DOG displays: perform additional display initialisation
  // this has to be explicitely enabled
  // see also $MIOS32_PATH/modules/app_lcd/dog/app_lcd.mk for "auto selection"
  // this makefile is included if environment variable "MIOS32_LCD" is set to "dog"
#ifdef APP_LCD_INIT_DOG
  APP_LCD_Cmd(0x39); // 8bit interface, switch to instruction table 1
  APP_LCD_Cmd(0x1d); // BS: 1/4, 3 line LCD
  APP_LCD_Cmd(0x50); // Booster off, set contrast C5/C4
  APP_LCD_Cmd(0x6c); // set Voltage follower and amplifier
  APP_LCD_Cmd(0x7c); // set contrast C3/C2/C1
  //  APP_LCD_Cmd(0x38); // back to instruction table 0
  // (will be done below)

  // modify cursor mapping, so that it complies with 3-line dog displays
  u8 cursor_map[] = {0x00, 0x10, 0x20, 0x30}; // offset line 0/1/2/3
  MIOS32_LCD_CursorMapSet(cursor_map);
#endif

  APP_LCD_Cmd(0x38); // experience from PIC based MIOS: without these lines
  APP_LCD_Cmd(0x0c); // the LCD won't work correctly after a second APP_LCD_Init

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

  // wait until LCD unbusy, exit on error (timeout)
  if( MIOS32_BOARD_J15_PollUnbusy(mios32_lcd_device, 2500) < 0 ) {
    // disable display
    display_available &= ~(1 << mios32_lcd_device);
    return -2; // timeout
  }

  // write data
  MIOS32_BOARD_J15_DataSet(data);
  MIOS32_BOARD_J15_RS_Set(1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);

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

  // wait until LCD unbusy, exit on error (timeout)
  if( MIOS32_BOARD_J15_PollUnbusy(mios32_lcd_device, 2500) < 0 ) {
    // disable display
    display_available &= ~(1 << mios32_lcd_device);
    return -2; // timeout
  }

  // write command
  MIOS32_BOARD_J15_DataSet(cmd);
  MIOS32_BOARD_J15_RS_Set(0);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1);
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 0);

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
