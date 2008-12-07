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
#include <app.h>

@implementation UI

// local variables to bridge objects to C functions
static NSObject *_self;

#define NUM_LEDS 17
NSColorWell *LED[NUM_LEDS];
u8 ledState[NUM_LEDS]; // for dual-colour option

static NSButton *_buttonTrack1;
static NSButton *_buttonTrack2;
static NSButton *_buttonTrack3;
static NSButton *_buttonTrack4;

static NSButton *_buttonGroup1;
static NSButton *_buttonGroup2;
static NSButton *_buttonGroup3;
static NSButton *_buttonGroup4;

static NSButton *_buttonPLayerA;
static NSButton *_buttonPLayerB;
static NSButton *_buttonPLayerC;

static NSButton *_buttonTLayerA;
static NSButton *_buttonTLayerB;
static NSButton *_buttonTLayerC;

static NSButton *_buttonEdit;
static NSButton *_buttonMute;
static NSButton *_buttonPattern;
static NSButton *_buttonSong;

static NSButton *_buttonSolo;
static NSButton *_buttonFast;
static NSButton *_buttonAll;

static NSButton *_buttonStepView;

static NSButton *_buttonPlay;
static NSButton *_buttonStop;
static NSButton *_buttonPause;

static NSButton *_buttonMenu;
static NSButton *_buttonScrub;
static NSButton *_buttonMetronome;


//////////////////////////////////////////////////////////////////////////////
// Emulation specific LED access functions
//////////////////////////////////////////////////////////////////////////////
s32 EMU_DOUT_PinSet(u32 pin, u32 value)
{
	int gp_led = -1;
	
	pin ^= 7;
	
	// GP LEDs
	if( pin >= 16 && pin < 32 ) {
		gp_led = pin - 16;
		
		if( value )
			ledState[gp_led] |= (1 << 0); // set first color
		else
			ledState[gp_led] &= ~(1 << 0); // clear first color
		// color mapping at the end of this function

	} else if( pin >= 112 && pin < 128 ) {
		gp_led = pin - 112;
		
		if( value )
			ledState[gp_led] |= (1 << 1); // set second color
		else
			ledState[gp_led] &= ~(1 << 1); // clear second color
		// color mapping at the end of this function

	} else if( pin == 7 ) { // Beat LED
		if( value )
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]];
		else
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
	} else {
		// remaining LED functions via button highlighting
		// TODO: find solution to update this after button has been released!
		switch( pin ) {
			case  0: [_buttonTrack1 highlight:(value ? YES : NO)]; break;
			case  1: [_buttonTrack2 highlight:(value ? YES : NO)]; break;
			case  2: [_buttonTrack3 highlight:(value ? YES : NO)]; break;
			case  3: [_buttonTrack4 highlight:(value ? YES : NO)]; break;

			case  4: [_buttonPLayerA highlight:(value ? YES : NO)]; break;
			case  5: [_buttonPLayerB highlight:(value ? YES : NO)]; break;
			case  6: [_buttonPLayerC highlight:(value ? YES : NO)]; break;

			case 80: [_buttonGroup1 highlight:(value ? YES : NO)]; break;
			case 82: [_buttonGroup2 highlight:(value ? YES : NO)]; break;
			case 84: [_buttonGroup3 highlight:(value ? YES : NO)]; break;
			case 86: [_buttonGroup4 highlight:(value ? YES : NO)]; break;

			case 88: [_buttonTLayerA highlight:(value ? YES : NO)]; break;
			case 89: [_buttonTLayerB highlight:(value ? YES : NO)]; break;
			case 90: [_buttonTLayerC highlight:(value ? YES : NO)]; break;

			case  8: [_buttonEdit highlight:(value ? YES : NO)]; break;
			case  9: [_buttonMute highlight:(value ? YES : NO)]; break;
			case 10: [_buttonPattern highlight:(value ? YES : NO)]; break;
			case 11: [_buttonSong highlight:(value ? YES : NO)]; break;
			
			case 12: [_buttonSolo highlight:(value ? YES : NO)]; break;
			case 13: [_buttonFast highlight:(value ? YES : NO)]; break;
			case 14: [_buttonAll highlight:(value ? YES : NO)]; break;
			
			case 91: [_buttonPlay highlight:(value ? YES : NO)]; break;
			case 92: [_buttonStop highlight:(value ? YES : NO)]; break;
			case 93: [_buttonPause highlight:(value ? YES : NO)]; break;
			
			case 95: [_buttonStepView highlight:(value ? YES : NO)]; break;
			
			case 96: [_buttonMenu highlight:(value ? YES : NO)]; break;
			case 97: [_buttonScrub highlight:(value ? YES : NO)]; break;
			case 98: [_buttonMetronome highlight:(value ? YES : NO)]; break;
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

s32 EMU_DOUT_SRSet(u32 sr, u8 value)
{
	int i;

	for(i=0; i<8; ++i)
		EMU_DOUT_PinSet(sr*8+i, (value & (1 << i)) ? 1 : 0);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Emulation specific button event forwarding function
//////////////////////////////////////////////////////////////////////////////
s32 EMU_DIN_NotifyToggle(u32 pin, u32 value)
{
//	APP_DIN_NotifyToggle(pin, value);

	// true emulation: forward to MIOS32_SRIO_Wrapper
	if( pin > 8*MIOS32_SRIO_NUM_SR )
		return -1;

#if 0
	NSLog(@"Pin: %d: %d\n", pin, value);
#endif

	if( value )
		mios32_srio_din_buffer[pin>>3] |= (1 << (pin&7));
	else
		mios32_srio_din_buffer[pin>>3] &= ~(1 << (pin&7));

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
// Tasks
//////////////////////////////////////////////////////////////////////////////
- (void)periodic1mSTask:(NSTimer *)aTimer
{
	// -> forward to application
	SEQ_TASK_Period1mS();
}


void SRIO_ServiceFinish(void)
{
#ifndef MIOS32_DONT_USE_SRIO

# ifndef MIOS32_DONT_USE_ENC
  // update encoder states
//  MIOS32_ENC_UpdateStates();
# endif

  // notify application about finished SRIO scan
  APP_SRIO_ServiceFinish();
#endif
}

- (void)periodicSRIOTask:(NSTimer *)aTimer
{
	// notify application about SRIO scan start
	APP_SRIO_ServicePrepare();

	// start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
	MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);

	// emulation: transfer new DOUT SR values to GUI LED elements
	int sr;
	for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
		EMU_DOUT_SRSet(sr, mios32_srio_dout[MIOS32_SRIO_NUM_SR-sr-1]);

#ifndef MIOS32_DONT_USE_DIN
	// check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
	MIOS32_DIN_Handler(APP_DIN_NotifyToggle);
#endif
}

- (void)periodicMIDITask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	while (YES) {
		// TOOD: find better more FreeRTOS/MIOS32 compliant solution
		// check for incoming MIDI messages and call hooks
		MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);

		// -> forward to application
		SEQ_TASK_MIDI();
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}



//////////////////////////////////////////////////////////////////////////////
// initialisation hook for OS specific tasks
// (called from APP_Init() after everything has been prepared)
//////////////////////////////////////////////////////////////////////////////
s32 TASKS_Init(u32 mode)
{
	// install 1mS task
	NSTimer *timer2 = [NSTimer timerWithTimeInterval:0.001 target:_self selector:@selector(periodic1mSTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer2 forMode: NSRunLoopCommonModes];	

	// install SRIO task
	NSTimer *timer3 = [NSTimer timerWithTimeInterval:0.001 target:_self selector:@selector(periodicSRIOTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer3 forMode: NSRunLoopCommonModes];	

	// Detach the new threads
	[NSThread detachNewThreadSelector:@selector(periodicMIDITask:) toTarget:_self withObject:nil];
	
	return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// init application after ca. 1 mS (this ensures that all objects have been initialized)
//////////////////////////////////////////////////////////////////////////////
- (void)delayedAPP_Init:(id)anObject
{
	// init emulated MIOS32 modules
#ifndef MIOS32_DONT_USE_SRIO
	MIOS32_SRIO_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SRIO
	MIOS32_SRIO_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DIN_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DOUT) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DOUT_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SRIO
	MIOS32_SRIO_Init(0);
#endif
#ifndef MIOS32_DONT_USE_MIDI
	MIOS32_MIDI_Init(0);
#endif
#ifndef MIOS32_DONT_USE_COM
//  MIOS32_COM_Init(0);
#endif
#ifndef MIOS32_DONT_USE_LCD
	MIOS32_LCD_Init(0);
#endif

	// call init function of application
	APP_Init();
}


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;

	_self = self;

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

	_buttonMenu = buttonMenu;
	_buttonScrub = buttonScrub;
	_buttonMetronome = buttonMetronome;

	// clear LED states
	for(i=0; i<NUM_LEDS; ++i)
		ledState[i] = 0;

	// init application after ca. 1 mS (this ensures that all objects have been initialized)
	NSTimer *init_timer = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(delayedAPP_Init:) userInfo:nil repeats:NO];
	[[NSRunLoop currentRunLoop] addTimer: init_timer forMode: NSRunLoopCommonModes];
}

@end
