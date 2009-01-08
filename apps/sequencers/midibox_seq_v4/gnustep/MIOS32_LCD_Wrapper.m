//
//  MIOS32_LCD_Wrapper.m
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_LCD_Wrapper.h"

#include <AppKit/AppKit.h>
#include <mios32.h>
#include "app_lcd.h"

@implementation MIOS32_LCD_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;

#define NUM_LCD 2
CLCDView *LCD[NUM_LCD];



//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;

	// make object specific pointers visible for native C
	LCD[0]=lcdView1;
	LCD[1]=lcdView2;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	APP_LCD_Clear();
	APP_LCD_CursorSet(0, 0);

	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	[LCD[device] LCDPrintChar:data];

	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 if display not available or timed out
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	[LCD[device] LCDClear];
	
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	// exit with error if line is not in allowed range
	if( line >= MIOS32_LCD_MAX_MAP_LINES )
		return -1;

	// -> set cursor address
// (mapping not relevant)
//	return APP_LCD_Cmd(0x80 | (mios32_lcd_cursor_map[line] + column));
	[LCD[device] setLCDCursorX:column];
	[LCD[device] setLCDCursorY:line];

	return 0; // no error
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
// Prints a single character
// IN: character in <c>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(char c)
{
	return APP_LCD_Data(c);
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
	u8 device = MIOS32_LCD_DeviceGet();

	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	[LCD[device] LCDSpecialCharInit:num:table];
	
	return 0; // no error
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

@end
