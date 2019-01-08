// $Id$
/*
 * Hawkeye: application specific CLCD driver for holtek 16514 compatible displays
 * Support for dual display mode using special cabling with different
 * rs/rw/e lines, which were necessary due to interference (the displays could
 * not share these lines, at least in my meager tests).
 *
 * Cabling diagram
 * J15A     DISPLAY 1   WHAT
 * 1     -> 1           GND
 * 2     -> 2           VCC 5V
 * 4     -> 4           RS
 * 5     -> 5           RW
 * 6     -> 6           E
 * 11    -> 11          DB4
 * 12    -> 12          DB5
 * 13    -> 13          DB6
 * 14    -> 14          DB7
 * -------------------------
 * J15B    DISPLAY 2    WHAT
 * 1     -> 1           GND
 * 2     -> 2           VCC 5V
 * 8     -> 4           RS 
 * 9     -> 5           RW
 * 10    -> 6           E
 * 11    -> 11          DB4
 * 12    -> 12          DB5
 * 13    -> 13          DB6
 * 14    -> 14          DB7
 *
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  extended to dual-display holtek 16514 specialties
 *  by Hawkeye (midibox.org/forums) 2010
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

static u8 lastdata = 0;
static u8 current_lcd_device = 0;
static u32 last_reinit_seconds = 0;
static u32 charcount = 0;


/////////////////////////////////////////////////////////////////////////////
// Display-port specific setting of RS
/////////////////////////////////////////////////////////////////////////////
static void RS_Set(u8 level)
{
   if (current_lcd_device == 0)
      MIOS32_BOARD_J15_RS_Set(level);
   else
   {
      if (level)
         lastdata |= 0b00000010;
      else
         lastdata &= 0b11111101;

      MIOS32_BOARD_J15_DataSet(lastdata);
   }
}


/////////////////////////////////////////////////////////////////////////////
// Display-port specific setting of RW
/////////////////////////////////////////////////////////////////////////////
static void RW_Set(u8 level)
{
   if (current_lcd_device == 0)
      MIOS32_BOARD_J15_RW_Set(level);
   else
   { 
      if (level)
         lastdata |= 0b00000100;
      else
         lastdata &= 0b11111011;

      MIOS32_BOARD_J15_DataSet(lastdata);
   }
}


/////////////////////////////////////////////////////////////////////////////
// Display-port specific setting of E
/////////////////////////////////////////////////////////////////////////////
static void E_Set(u8 level)
{
   if (current_lcd_device == 0)
      MIOS32_BOARD_J15_E_Set(current_lcd_device, level);
   else
   {
      if (level)
         lastdata |= 0b00001000;
      else
         lastdata &= 0b11110111;

      MIOS32_BOARD_J15_DataSet(lastdata);
   }
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to VFD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
   current_lcd_device = mios32_lcd_device;
   
   RS_Set(0);
   RW_Set(0);
   E_Set(1);
   lastdata = (lastdata & 0x0F) + (cmd & 0xF0);
   MIOS32_BOARD_J15_DataSet(lastdata);
   E_Set(0);

   RS_Set(0);
   RW_Set(0);
   E_Set(1);
   lastdata = (lastdata & 0x0F) + (cmd << 4);
   MIOS32_BOARD_J15_DataSet(lastdata);
   E_Set(0);
   
   return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Hack: reinitializes the screens after some time, to clean up 
//       potentially garbled screens
//
/////////////////////////////////////////////////////////////////////////////
void reinit()
{
   mios32_sys_time_t t = MIOS32_SYS_TimeGet();
   charcount++;
   if (t.seconds > last_reinit_seconds || charcount > 500)
   {
      charcount = 0;
      last_reinit_seconds = t.seconds;
      
      MIOS32_BOARD_J15_RS_Set(0);
      MIOS32_BOARD_J15_RW_Set(0);
      APP_LCD_Cmd(0b00110011);
      APP_LCD_Cmd(0b00110010);

      APP_LCD_Cmd(0b00101000); // 4bit, 2 lines
      APP_LCD_Cmd(0b00001100); // display on, cursor off, char blinking off
      APP_LCD_Cmd(0b00000110); // entry mode
   }
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to VFD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
   current_lcd_device = mios32_lcd_device;
   
   RS_Set(1);
   RW_Set(0);
   E_Set(1);
   lastdata = (lastdata & 0x0F) + (data & 0xF0);
   MIOS32_BOARD_J15_DataSet(lastdata);
   E_Set(0);

   RS_Set(1);
   RW_Set(0);
   E_Set(1);
   lastdata = (lastdata & 0x0F) + (data << 4);
   MIOS32_BOARD_J15_DataSet(lastdata);
   E_Set(0);

   reinit();

   return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific VFD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
   MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE);

   MIOS32_BOARD_J15_RS_Set(0);
   MIOS32_BOARD_J15_RW_Set(0);
   APP_LCD_Cmd(0b00110011);
   APP_LCD_Cmd(0b00110010);

   APP_LCD_Cmd(0b00101000); // 4bit, 2 lines
   APP_LCD_Cmd(0b00001100); // display on, cursor off, char blinking off
   APP_LCD_Cmd(0b00000110); // entry mode
   APP_LCD_Cmd(0b00000001); // clear

   return 0;
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
