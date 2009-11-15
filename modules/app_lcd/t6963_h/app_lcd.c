// $Id$
/*
 * Application specific GLCD driver for T6963 with horizontal screen (-> 240x64)
 * Referenced from MIOS32_LCD routines
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

#include <glcd_font.h>

#include "app_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;

// we have to buffer the whole screen to avoid read operations when fonts/icons are drawn
static u8 screen_buffer[64][32]; // y*x, same memory size as T6963 internal RAM


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

  // set LCD type
  mios32_lcd_type = MIOS32_LCD_TYPE_GLCD;

  // RD_N pin connected to J15:E
  // WR_N pin connected to J15:RW
  // set both pins to non-active state
  MIOS32_BOARD_J15_E_Set(mios32_lcd_device, 1); // note: mios32_lcd_device should always be 0
  MIOS32_BOARD_J15_RW_Set(1);
  
  // initialize LCD
#ifdef MIOS32_DONT_USE_DELAY
  u32 delay;
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(1); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif

  // set graphic home address to 0x0000
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x42);

  // set it again, Sam
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x42);

  // set graphic area to 0x20 bytes per line
  APP_LCD_Data(0x20);
  APP_LCD_Data(0x00);
  APP_LCD_Cmd(0x43);

  // set mode (AND mode, CG ROM)
  APP_LCD_Cmd(0x82);

  // set display mode (graphic, no text)
  APP_LCD_Cmd(0x98);

  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // write data
  MIOS32_BOARD_J15_DataSet(data);
  MIOS32_BOARD_J15_RS_Set(0); // CD connected to RS

  // strobe WR_N
  MIOS32_BOARD_J15_RW_Set(0);
  MIOS32_BOARD_J15_RW_Set(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // check if if display already has been disabled
  if( !(display_available & (1 << mios32_lcd_device)) )
    return -1;

  // write command
  MIOS32_BOARD_J15_DataSet(cmd);
  MIOS32_BOARD_J15_RS_Set(1); // CD connected to RS

  // strobe WR_N
  u32 delay_ctr;
  MIOS32_BOARD_J15_RW_Set(0);
  for(delay_ctr=0; delay_ctr<40; ++delay_ctr) MIOS32_BOARD_J15_RW_Set(1);

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

  // set Y=0, X=0
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Cmd(0x24);

  // clear 32*64 bytes
  for(y=0; y<64; ++y) {
    for(x=0; x<32; ++x) {
      error |= APP_LCD_Data(0x00);
      error |= APP_LCD_Cmd(0xc0); // write and increment
      screen_buffer[y][x] = 0x00; // clear screen buffer as well
    }
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
  // nothing to do for GLCD driver

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  // nothing to do for GLCD driver

  return 0; // no error
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
  int line, x, y;
  int y_lines = (bitmap.height >> 3);

  // transfer character to screen buffer
  for(line=0; line<y_lines; ++line) {

    // calculate pointer to bitmap line
    u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;

    // TODO: we could provide a special mode w/o screenbuffer which would work fine
    // so long no MIOS fonts are print
    for(x=0; x<bitmap.width; ++x) {
      u8 x_pos = mios32_lcd_x + x;
      u8 y_pos = mios32_lcd_y + 8*line;
      u8 b = *memory_ptr++;

      for(y=0; y<8; ++y) {
	u8 mask = 1 << ((7-x_pos)&7);
	if( b & (1 << y)  )
	  screen_buffer[y_pos+y][x_pos>>3] |= mask;
	else
	  screen_buffer[y_pos+y][x_pos>>3] &= ~mask;
      }
    }

    // now transfer updated screen buffer contents to GLCD
    for(y=0; y<bitmap.height; ++y) {
      // set address pointer
      u16 addr = (mios32_lcd_x >> 3) + 32*(mios32_lcd_y + y);
      APP_LCD_Data(addr & 0xff);
      APP_LCD_Data(addr >> 8);
      APP_LCD_Cmd(0x24);

      for(x=0; x<(bitmap.width+7); x+=8) { // +7: depending on character position we mostly have to write one byte more
	// write and increment
	APP_LCD_Data(screen_buffer[mios32_lcd_y+y][(mios32_lcd_x+x)>>3]);
	APP_LCD_Cmd(0xc0);
      }
    }
  }

  mios32_lcd_x += bitmap.width;

  return 0; // no error
}
