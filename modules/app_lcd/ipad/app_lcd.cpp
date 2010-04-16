/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Application specific CLCD driver for MacOS
 *
 * Note: this module has been created for running on a Mac
 * Therefore we are allowed to use malloc based map here w/o dangers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Phil Taylor (phil@taylor.org.uk) / Thorsten Klose (tk@midibox.org)
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

#include "CLcd.h"
#include <map>


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

volatile u32 display_available = 0;

std::map<unsigned char, CLcd *> cLcd;


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_Init(u32 mode)
{
	// currently only mode 0 supported
    if( mode != 0 )
        return -1; // unsupported mode

    // only up to 32 displays supported
    if( mios32_lcd_device >= 32 )
        return -2; // unsupported display number

	// enable display
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
extern "C" s32 APP_LCD_Data(u8 data)
{
	// check if if display already has been disabled
    if( !(display_available & (1 << mios32_lcd_device)) )
        return -1;

	if( cLcd[mios32_lcd_device] == 0 ) {
		// display hasn't been created yet - disable it
		display_available &= ~(1 << mios32_lcd_device);
		return -1; // display not available (anymore)    
	} else {    // forward to LCD emulation
		cLcd[mios32_lcd_device]->lcdData(data);
	}	
    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_Cmd(u8 cmd)
{
    // check if if display already has been disabled
    if( !(display_available & (1 << mios32_lcd_device)) )
        return -1;

	if( cLcd[mios32_lcd_device] == 0 ) {
		// display hasn't been created yet - disable it
		display_available &= ~(1 << mios32_lcd_device);
		return -1; // display not available (anymore)    
	} else {    // forward to LCD emulation
	    cLcd[mios32_lcd_device]->lcdCmd(cmd);
	}
    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_Clear(void)
{
    // -> send clear command
    return APP_LCD_Cmd(0x01);
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_CursorSet(u16 column, u16 line)
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
extern "C" s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
    // n.a.
    return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
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
extern "C" s32 APP_LCD_BColourSet(u32 rgb)
{
    return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_FColourSet(u32 rgb)
{
    return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
    return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap within given boundaries to the LCD
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
    return -1; // n.a.
}



/////////////////////////////////////////////////////////////////////////////
// Returns a pointer to the CLcd component
// Creates a new CLcd if it doesn't exist yet (up to 256 displays can be created ;)
/////////////////////////////////////////////////////////////////////////////
CLcd *APP_LCD_GetComponentPtr(u8 device)
{
	if( cLcd[device] == 0 ) // create display
        cLcd[device] = new CLcd();

	if( !(display_available & (1 << device)) ) {
        // initialize display
        u8 mios32_lcd_device_saved = mios32_lcd_device;
        mios32_lcd_device = device;

		s32 status = APP_LCD_Init(0);

        mios32_lcd_device = mios32_lcd_device_saved;

        if( status < 0 ) {
            // display cannot be created
            delete cLcd[device];
            cLcd[device] = NULL;
        }
	} 

    return cLcd[device]; // returns NULL if not available
}
