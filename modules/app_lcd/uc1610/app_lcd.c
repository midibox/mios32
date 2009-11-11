// $Id: app_lcd.c 95 2008-10-19 14:13:14Z tk $
/*
 * for up to 8 * DOG GLCD (every display provides a resolution of 128x64) 
 *
 * This driver allows to drive up to 8 of them, every display is connected
 * to a dedicated chip select line. They can be addressed with
 * following (graphical) cursor positions:
 * 
 * 
 * +--------------+  +--------------+  
 * |              |  |              |  
 * | X =   0..127 |  | X = 128..255 |    
 * | Y =   0..  7 |  | Y =   0..  7 | 
 * |              |  |              |  
 * +--------------+  +--------------+  
 *
 * +--------------+  +--------------+ 
 * |              |  |              | 
 * | X =   0..127 |  | X = 128..255 | 
 * | Y =   8.. 15 |  | Y =   8.. 15 | 
 * |              |  |              | 
 * +--------------+  +--------------+ 
 *
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =  16.. 23 |  | Y =  16.. 23 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * +--------------+  +--------------+
 * |              |  |              |
 * | X =   0..127 |  | X = 128..255 |
 * | Y =  24.. 31 |  | Y =  24.. 31 |
 * |              |  |              |
 * +--------------+  +--------------+
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Copyright (C) 2009 Phil Taylor (phil@taylor.org.uk)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
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

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#define APP_LCD_OUTPUT_MODE  0
// MEMO: DOGM128 works at 3.3V, level shifting (and open drain mode) not required


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

  // set LCD type
  mios32_lcd_type = MIOS32_LCD_TYPE_GLCD;

  // initialize LCD
#ifdef MIOS32_DONT_USE_DELAY
  u32 delay;
  for(delay=0; delay<50000; ++delay) MIOS32_BOARD_J15_RW_Set(0); // ca. 50 mS Delay
#else
  MIOS32_DELAY_Wait_uS(50000); // exact 50 mS delay
#endif
  APP_LCD_Cmd(0xf1); 
  APP_LCD_Cmd(0x67);  
  APP_LCD_Cmd(0xc0); 
  APP_LCD_Cmd(0x40);   
  APP_LCD_Cmd(0x50); 
  APP_LCD_Cmd(0x2b); 
  APP_LCD_Cmd(0xeb); 
  APP_LCD_Cmd(0x81); 
  APP_LCD_Cmd(0x5f);
  APP_LCD_Cmd(0x89);
  APP_LCD_Cmd(0xd0); //Greyscale control
  APP_LCD_Cmd(0xaf); 

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
  u8 line = 0;
  if( mios32_lcd_y >= 3*26*4 )
    line = 3;
  else if( mios32_lcd_y >= 2*26*4 )
    line = 2;
  else if( mios32_lcd_y >= 1*26*4 )
    line = 1;

  u8 row = 0;
  if( mios32_lcd_x >= 1*160 )
    row = 1;

  u8 cs = 2*line + row;

  if( cs >= 8 )
    return -1; // invalid CS line

  // chip select and DC
  MIOS32_BOARD_J15_DataSet(~(1 << cs));
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

  // send data
  MIOS32_BOARD_J15_SerDataShift(data);
  //MIOS32_DELAY_Wait_uS(40); // exact 10 uS delay

  // if end of display segment reached: set X position of all segments to 0
 // if( (mios32_lcd_x % 160) == 0 ) {
  //  APP_LCD_Cmd(0x00); // Set lower nibble to 0
  //  return APP_LCD_Cmd(0x10); // Set upper nibble to 0
  //}

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
  MIOS32_BOARD_J15_DataSet(0x00);
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

  // send command
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
  // select all LCDs
  error |= MIOS32_LCD_CursorSet(0, 0);

  // send data
  for(y=0; y<104; y=y+4) {	
    error |= APP_LCD_GCursorSet(0, y);
    MIOS32_BOARD_J15_DataSet(0x00);
    MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC
    for(x=0; x<160; ++x){
      MIOS32_BOARD_J15_SerDataShift(0x00);
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
  error |= APP_LCD_Cmd(0x00 | ((x % 160) & 0x0f)); // send LSB nibble
  error |= APP_LCD_Cmd(0x10 | (((x % 160) >> 4) & 0x0f));   // send MSB nibble

  // set Y position
  error |= APP_LCD_Cmd(0x60 | ((y>>2) % 26));

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Prints a single character
// IN: character in <c>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(char c)
{
  int x, y, line;


  // font not initialized yet!
  if( mios32_lcd_font == NULL )
    return -1;

  u8 y_lines = (mios32_lcd_font_height>>3);

  for(line=0; line<y_lines; ++line) {

    // calculate pointer to character line
    u8 *font_ptr = mios32_lcd_font + line * mios32_lcd_font_offset + y_lines * mios32_lcd_font_offset * (size_t)c + (size_t)mios32_lcd_font_x0;

    // set graphical cursor after second line has reached
    if( line > 0 ) {
      mios32_lcd_x -= mios32_lcd_font_width;
      mios32_lcd_y += 8;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }

	u8 dbit0,dbit1,dbit2,dbit3;

    // transfer character
    for(x=0; x<mios32_lcd_font_width; ++x) {
	    dbit0 = 0; dbit1 = 0; dbit2 = 0; dbit3 = 0;
		if ((*font_ptr & 0x01)==0x01) dbit0 = 0x03;
		if ((*font_ptr & 0x02)==0x02) dbit1 = 0x0C;
		if ((*font_ptr & 0x04)==0x04) dbit2 = 0x30;
		if ((*font_ptr & 0x08)==0x08) dbit3 = 0xC0;
		APP_LCD_Data((dbit0+dbit1+dbit2+dbit3));
		*font_ptr++;
	}
	
    // calculate pointer to character line
    font_ptr = mios32_lcd_font + line * mios32_lcd_font_offset + y_lines * mios32_lcd_font_offset * (size_t)c + (size_t)mios32_lcd_font_x0;
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y+4);
	
    for(x=0; x<mios32_lcd_font_width; ++x) {
	    dbit0 = 0; dbit1 = 0; dbit2 = 0; dbit3 = 0;
		if ((*font_ptr & 0x10)==0x10) dbit0 = 0x03;
		if ((*font_ptr & 0x20)==0x20) dbit1 = 0x0C;
		if ((*font_ptr & 0x40)==0x40) dbit2 = 0x30;
		if ((*font_ptr & 0x80)==0x80) dbit3 = 0xC0;
		APP_LCD_Data((dbit0+dbit1+dbit2+dbit3));
		// increment graphical cursor
		++mios32_lcd_x;
		*font_ptr++;
	}


  }

  // fix graphical cursor if more than one line has been printed
  if( y_lines >= 1 ) {
    mios32_lcd_y = mios32_lcd_y - (mios32_lcd_font_height-8);
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
  }

  // increment cursor
  mios32_lcd_column += 1;

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
// IN: r/g/b values
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u8 r, u8 g, u8 b)
{
  return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b values
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u8 r, u8 g, u8 b)
{
  return -1; // n.a.
}
