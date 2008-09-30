// $Id$
//
//  UI.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "UI.h"

#include <mios32.h>

@implementation UI

// local variables to bridge objects to C functions
#define NUM_LCD 2
CLCDView *LCD[NUM_LCD];

#define NUM_LEDS 17
NSColorWell *LED[NUM_LEDS];
u8 ledState[NUM_LEDS]; // for dual-colour option

NSButton *_buttonTrack1;
NSButton *_buttonTrack2;
NSButton *_buttonTrack3;
NSButton *_buttonTrack4;

NSButton *_buttonGroup1;
NSButton *_buttonGroup2;
NSButton *_buttonGroup3;
NSButton *_buttonGroup4;

NSButton *_buttonPLayerA;
NSButton *_buttonPLayerB;
NSButton *_buttonPLayerC;

NSButton *_buttonTLayerA;
NSButton *_buttonTLayerB;
NSButton *_buttonTLayerC;

NSButton *_buttonEdit;
NSButton *_buttonMute;
NSButton *_buttonPattern;
NSButton *_buttonSong;

NSButton *_buttonSolo;
NSButton *_buttonFast;
NSButton *_buttonAll;

NSButton *_buttonStepView;

NSButton *_buttonPlay;
NSButton *_buttonStop;
NSButton *_buttonPause;

// LCD selection
u8 selectedLCD;


//////////////////////////////////////////////////////////////////////////////
// LCD access functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Init(u32 mode)
{
	// select first device
	MIOS32_LCD_DeviceSet(0);

	// clear screen
	MIOS32_LCD_Clear();

	// set cursor to initial position
	MIOS32_LCD_CursorSet(0, 0);
	
	return 0; // no error
}

s32 MIOS32_LCD_DeviceSet(u8 device)
{
	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	selectedLCD = device;
	
	return 0; // no error
}

u8 MIOS32_LCD_DeviceGet(void)
{
	return selectedLCD;
}

s32 MIOS32_LCD_CursorSet(u16 line, u16 column)
{
	[LCD[selectedLCD] setLCDCursorY:line];
	[LCD[selectedLCD] setLCDCursorX:column];
	
	return 0; // no error
}

s32 MIOS32_LCD_Clear(void)
{
	[LCD[selectedLCD] LCDClear];
	
	return 0; // no error
}

s32 MIOS32_LCD_PrintChar(char c)
{
	[LCD[selectedLCD] LCDPrintChar:c];

	return 0; // no error
}

s32 MIOS32_LCD_PrintString(char *str)
{
	[LCD[selectedLCD] LCDPrintString:str];

	return 0; // no error
}

s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8])
{
	[LCD[selectedLCD] LCDSpecialCharInit:num:table];
	
	return 0; // no error
}

s32 MIOS32_LCD_SpecialCharsInit(u8 table[64])
{
	[LCD[selectedLCD] LCDSpecialCharsInit:table];
	
	return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// LED access functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinSet(u32 pin, u32 value)
{
	int gp_led = -1;
	
	// GP LEDs
	if( pin >= 16 && pin < 32 ) {
		gp_led = (pin^7) - 16;
		
		if( value )
			ledState[gp_led] |= (1 << 0); // set first color
		else
			ledState[gp_led] &= ~(1 << 0); // clear first color
		// color mapping at the end of this function

	} else if( pin >= 112 && pin < 128 ) {
		gp_led = (pin^7) - 112;
		
		if( value )
			ledState[gp_led] |= (1 << 1); // set second color
		else
			ledState[gp_led] &= ~(1 << 1); // clear second color
		// color mapping at the end of this function

	} else if( pin == 0 ) { // Beat LED
		if( value )
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]];
		else
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
	} else {
		// remaining LED functions via button highlighting
		// TODO: find solution to update this after button has been released!
		switch( pin ) {
			case  7: [_buttonTrack1 highlight:(value ? YES : NO)]; break;
			case  6: [_buttonTrack2 highlight:(value ? YES : NO)]; break;
			case  5: [_buttonTrack3 highlight:(value ? YES : NO)]; break;
			case  4: [_buttonTrack4 highlight:(value ? YES : NO)]; break;

			case  3: [_buttonPLayerA highlight:(value ? YES : NO)]; break;
			case  2: [_buttonPLayerB highlight:(value ? YES : NO)]; break;
			case  1: [_buttonPLayerC highlight:(value ? YES : NO)]; break;

			case 87: [_buttonGroup1 highlight:(value ? YES : NO)]; break;
			case 85: [_buttonGroup2 highlight:(value ? YES : NO)]; break;
			case 83: [_buttonGroup3 highlight:(value ? YES : NO)]; break;
			case 81: [_buttonGroup4 highlight:(value ? YES : NO)]; break;

			case 95: [_buttonTLayerA highlight:(value ? YES : NO)]; break;
			case 94: [_buttonTLayerB highlight:(value ? YES : NO)]; break;
			case 93: [_buttonTLayerC highlight:(value ? YES : NO)]; break;

			case 15: [_buttonEdit highlight:(value ? YES : NO)]; break;
			case 14: [_buttonMute highlight:(value ? YES : NO)]; break;
			case 13: [_buttonPattern highlight:(value ? YES : NO)]; break;
			case 12: [_buttonSong highlight:(value ? YES : NO)]; break;
			
			case 11: [_buttonSolo highlight:(value ? YES : NO)]; break;
			case 10: [_buttonFast highlight:(value ? YES : NO)]; break;
			case  9: [_buttonAll highlight:(value ? YES : NO)]; break;
			
			case 88: [_buttonStepView highlight:(value ? YES : NO)]; break;

			case 92: [_buttonPlay highlight:(value ? YES : NO)]; break;
			case 91: [_buttonStop highlight:(value ? YES : NO)]; break;
			case 90: [_buttonPause highlight:(value ? YES : NO)]; break;
		}
	}

	// handle dual colour option of GP LEDs
	if( gp_led >= 0 ) {
		switch( ledState[gp_led] ) {
			case 1:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]]; break;
			case 2:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:0.3 blue:0.1 alpha:1.0]]; break;
			case 3:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:0.1 alpha:1.0]]; break;
			default: [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
		}
	}

	return 0;
}

s32 MIOS32_DOUT_SRSet(u32 sr, u8 value)
{
	int i;

	for(i=0; i<8; ++i)
		MIOS32_DOUT_PinSet(sr*8+i, (value & (1 << i)) ? 1 : 0);

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;
	
	// make object specific pointers visible for native C
	LCD[0]=lcdView1;
	LCD[1]=lcdView2;
	selectedLCD = 0;

	LED[0]=LED1;
	LED[1]=LED2;
	LED[2]=LED3;
	LED[3]=LED4;
	LED[4]=LED5;
	LED[5]=LED6;
	LED[6]=LED7;
	LED[7]=LED8;
	LED[8]=LED9;
	LED[9]=LED10;
	LED[10]=LED11;
	LED[11]=LED12;
	LED[12]=LED13;
	LED[13]=LED14;
	LED[14]=LED15;
	LED[15]=LED16;
	LED[16]=LEDBeat;

	// (only buttons with "LED" function)
	_buttonTrack1 = buttonTrack1;
	_buttonTrack2 = buttonTrack2;
	_buttonTrack3 = buttonTrack3;
	_buttonTrack4 = buttonTrack4;

	_buttonGroup1 = buttonGroup1;
	_buttonGroup2 = buttonGroup2;
	_buttonGroup3 = buttonGroup3;
	_buttonGroup4 = buttonGroup4;

	_buttonPLayerA = buttonPLayerA;
	_buttonPLayerB = buttonPLayerB;
	_buttonPLayerC = buttonPLayerC;

	_buttonTLayerA = buttonTLayerA;
	_buttonTLayerB = buttonTLayerB;
	_buttonTLayerC = buttonTLayerC;

	_buttonEdit = buttonEdit;
	_buttonMute = buttonMute;
	_buttonPattern = buttonPattern;
	_buttonSong = buttonSong;

	_buttonSolo = buttonSolo;
	_buttonFast = buttonFast;
	_buttonAll = buttonAll;
	_buttonStepView = buttonStepView;

	_buttonPlay = buttonPlay;
	_buttonStop = buttonStop;
	_buttonPause = buttonPause;

	// clear LED states
	for(i=0; i<NUM_LEDS; ++i)
		ledState[i] = 0;
	
	// call init function of application
	Init(0);
}

@end
