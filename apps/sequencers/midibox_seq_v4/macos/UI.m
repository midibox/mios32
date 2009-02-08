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

// shadow registers for DOUT pins
u8 dout_sr_shadow[MIOS32_SRIO_NUM_SR];

// LED reverences
#define NUM_LEDS 17
NSColorWell *LED[NUM_LEDS];
u8 ledState[NUM_LEDS]; // for dual-colour option

static NSButton *_buttonGP1;
static NSButton *_buttonGP2;
static NSButton *_buttonGP3;
static NSButton *_buttonGP4;
static NSButton *_buttonGP5;
static NSButton *_buttonGP6;
static NSButton *_buttonGP7;
static NSButton *_buttonGP8;
static NSButton *_buttonGP9;
static NSButton *_buttonGP10;
static NSButton *_buttonGP11;
static NSButton *_buttonGP12;
static NSButton *_buttonGP13;
static NSButton *_buttonGP14;
static NSButton *_buttonGP15;
static NSButton *_buttonGP16;

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
static NSButton *_buttonRew;
static NSButton *_buttonFwd;

static NSButton *_buttonMenu;
static NSButton *_buttonScrub;
static NSButton *_buttonMetronome;

static NSButton *_buttonUtility;
static NSButton *_buttonCopy;
static NSButton *_buttonPaste;
static NSButton *_buttonClear;

static NSButton *_buttonF1;
static NSButton *_buttonF2;
static NSButton *_buttonF3;
static NSButton *_buttonF4;

static NSButton *_buttonUp;
static NSButton *_buttonDown;


//////////////////////////////////////////////////////////////////////////////
// Emulation specific LED access functions
//////////////////////////////////////////////////////////////////////////////
s32 EMU_DOUT_PinSet(u32 pin, u32 value)
{
	int gp_led = -1;
	
	pin ^= 7;
	
	// GP LEDs
	if( pin >= 16 && pin < 32 ) {
#if 0
		// disabled - using highlight function of GP button instead
		gp_led = pin - 16;
		
		if( value )
			ledState[gp_led] |= (1 << 0); // set first color
		else
			ledState[gp_led] &= ~(1 << 0); // clear first color
		// color mapping at the end of this function
#endif
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
	}
	
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

		case 16: [_buttonGP1 highlight:(value ? YES : NO)]; break;
		case 17: [_buttonGP2 highlight:(value ? YES : NO)]; break;
		case 18: [_buttonGP3 highlight:(value ? YES : NO)]; break;
		case 19: [_buttonGP4 highlight:(value ? YES : NO)]; break;
		case 20: [_buttonGP5 highlight:(value ? YES : NO)]; break;
		case 21: [_buttonGP6 highlight:(value ? YES : NO)]; break;
		case 22: [_buttonGP7 highlight:(value ? YES : NO)]; break;
		case 23: [_buttonGP8 highlight:(value ? YES : NO)]; break;
		case 24: [_buttonGP9 highlight:(value ? YES : NO)]; break;
		case 25: [_buttonGP10 highlight:(value ? YES : NO)]; break;
		case 26: [_buttonGP11 highlight:(value ? YES : NO)]; break;
		case 27: [_buttonGP12 highlight:(value ? YES : NO)]; break;
		case 28: [_buttonGP13 highlight:(value ? YES : NO)]; break;
		case 29: [_buttonGP14 highlight:(value ? YES : NO)]; break;
		case 30: [_buttonGP15 highlight:(value ? YES : NO)]; break;
		case 31: [_buttonGP16 highlight:(value ? YES : NO)]; break;

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
		case 94: [_buttonRew highlight:(value ? YES : NO)]; break;
		case 95: [_buttonFwd highlight:(value ? YES : NO)]; break;
			
		case 96: [_buttonMenu highlight:(value ? YES : NO)]; break;
		case 97: [_buttonScrub highlight:(value ? YES : NO)]; break;
		case 98: [_buttonMetronome highlight:(value ? YES : NO)]; break;

		case 99: [_buttonUtility highlight:(value ? YES : NO)]; break;
		case 100: [_buttonCopy highlight:(value ? YES : NO)]; break;
		case 101: [_buttonPaste highlight:(value ? YES : NO)]; break;
		case 102: [_buttonClear highlight:(value ? YES : NO)]; break;

		case 104: [_buttonF1 highlight:(value ? YES : NO)]; break;
		case 105: [_buttonF2 highlight:(value ? YES : NO)]; break;
		case 106: [_buttonF3 highlight:(value ? YES : NO)]; break;
		case 107: [_buttonF4 highlight:(value ? YES : NO)]; break;

		case 109: [_buttonStepView highlight:(value ? YES : NO)]; break;
		
		case 110: [_buttonDown highlight:(value ? YES : NO)]; break;
		case 111: [_buttonUp highlight:(value ? YES : NO)]; break;
	}

	// handle dual colour option of GP LEDs
	if( gp_led >= 0 ) {
		switch( ledState[gp_led] ) {
#if 0
			case 1:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]]; break;
			case 2:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:0.3 blue:0.1 alpha:1.0]]; break;
			case 3:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:0.1 alpha:1.0]]; break;
			default: [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
#else
			// more contrast
			case 1:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.0 green:1.0 blue:0.0 alpha:1.0]]; break;
			case 2:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:0.0 blue:0.0 alpha:1.0]]; break;
			case 3:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:0.0 alpha:1.0]]; break;
			default: [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.9 green:0.9 blue:0.9 alpha:1.0]];
#endif
		}
	}

	return 0;
}

s32 EMU_DOUT_SRSet(u32 sr, u8 value)
{
	int i;

	// optimisation: only update DOUT pins in GUI if SR value has changed
	if( dout_sr_shadow[sr] == value )
		return 0;

	// same optimisation for individual pins
	u8 mask = 1;
	for(i=0; i<8; ++i) {
		if( (dout_sr_shadow[sr] ^ value) & mask )
			EMU_DOUT_PinSet(sr*8+i, (value & (1 << i)) ? 1 : 0);
		mask <<= 1;
	}

	// take over new value
	dout_sr_shadow[sr] = value;

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
// printf compatible function to output debug messages
// referenced in mios32_config.h (DEBUG_MSG macro)
//////////////////////////////////////////////////////////////////////////////
s32 UI_printf(char *format, ...)
{
  char buffer[1024];
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  NSLog(@"%s", buffer);
  
  return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// Tasks
//////////////////////////////////////////////////////////////////////////////

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
	NSLock *theLock = [[NSLock alloc] init];

	// notify application about SRIO scan start
	[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
	APP_SRIO_ServicePrepare();
	[theLock unlock];

	// start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
	MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);

	// emulation: transfer new DOUT SR values to GUI LED elements
	int sr;
	for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
		EMU_DOUT_SRSet(sr, mios32_srio_dout[MIOS32_SRIO_NUM_SR-sr-1]);

#ifndef MIOS32_DONT_USE_DIN
	// check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
	[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
	MIOS32_DIN_Handler(APP_DIN_NotifyToggle);
	[theLock unlock];
#endif
}

- (void)periodicMIDITask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSLock *theLock = [[NSLock alloc] init];

	while (YES) {
		// TODO: find better more FreeRTOS/MIOS32 compliant solution
		// check for incoming MIDI messages and call hooks
		[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
		MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);
		[theLock unlock];

		// -> forward to application
		[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
		SEQ_TASK_MIDI();
		[theLock unlock];
	
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}

- (void)periodic1mSTask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSLock *theLock = [[NSLock alloc] init];

	while (YES) {		
		[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
		SEQ_TASK_Period1mS();
		[theLock unlock];
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}

- (void)periodic1STask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSLock *theLock = [[NSLock alloc] init];

	while (YES) {		
		[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
		SEQ_TASK_Period1S();
		[theLock unlock];
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.000]];
    }
	
	[pool release];
	[NSThread exit];
}

/////////////////////////////////////////////////////////////////////////////
// This task is triggered from SEQ_PATTERN_Change to transport the new patch
// into RAM
/////////////////////////////////////////////////////////////////////////////
static void TASK_Pattern(void *pvParameters)
{
#if 0
  do {
    // suspend task - will be resumed from SEQ_PATTERN_Change()
    vTaskSuspend(NULL);

    SEQ_TASK_Pattern();
  } while( 1 );
#endif
}

// use this function to resume the task
void SEQ_TASK_PatternResume(void)
{
	NSLock *theLock = [[NSLock alloc] init];
//    vTaskResume(xPatternHandle);

	// MacOS: call task directly

	[theLock lock]; // TMP solution so long there is no better way to emulate MIOS32_IRQ_Disable()
    SEQ_TASK_Pattern();
	[theLock unlock];
}



//////////////////////////////////////////////////////////////////////////////
// initialisation hook for OS specific tasks
// (called from APP_Init() after everything has been prepared)
//////////////////////////////////////////////////////////////////////////////
s32 TASKS_Init(u32 mode)
{
	// install SRIO task
	NSTimer *timer1 = [NSTimer timerWithTimeInterval:0.001 target:_self selector:@selector(periodicSRIOTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer1 forMode: NSDefaultRunLoopMode];

	// Detach the new threads
	[NSThread detachNewThreadSelector:@selector(periodicMIDITask:) toTarget:_self withObject:nil];
	[NSThread detachNewThreadSelector:@selector(periodic1mSTask:) toTarget:_self withObject:nil];
	[NSThread detachNewThreadSelector:@selector(periodic1STask:) toTarget:_self withObject:nil];

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
	
	// print boot message
	MIOS32_LCD_PrintBootMessage();

	// doesn't work here - LCD won't be updated so long we are stalling the task :-/
#if 0
	// wait for 2 seconds
	int delay = 0;
	for(delay=0; delay<2000; ++delay)
		MIOS32_DELAY_Wait_uS(1000);
#endif

	// SD Card path selection
	int result;
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
 
    [oPanel setAllowsMultipleSelection:NO];
	[oPanel setCanChooseDirectories:YES];
	[oPanel setCanChooseFiles:NO];
    [oPanel setTitle:@"Choose SD Card Storage Directory"];
    [oPanel setMessage:@"Choose the directory where your MIDIbox SEQ V4 files should be located."];
    [oPanel setDelegate:self];
    result = [oPanel runModalForDirectory:NSHomeDirectory() file:nil];
    if (result == NSOKButton) {
		NSArray *selectedDirs = [oPanel filenames];
		SDCARD_Wrapper_setDir([selectedDirs objectAtIndex:0]);  // only the first directory selection is relevant
    } else {
		SDCARD_Wrapper_setDir(nil);  // disable SD card access
	}

}


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;

	_self = self;

	for(i=0; i<MIOS32_SRIO_NUM_SR; ++i)
		dout_sr_shadow[i] = 0x00;

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
	_buttonGP1 = buttonGP1;
	_buttonGP2 = buttonGP2;
	_buttonGP3 = buttonGP3;
	_buttonGP4 = buttonGP4;
	_buttonGP5 = buttonGP5;
	_buttonGP6 = buttonGP6;
	_buttonGP7 = buttonGP7;
	_buttonGP8 = buttonGP8;
	_buttonGP9 = buttonGP9;
	_buttonGP10 = buttonGP10;
	_buttonGP11 = buttonGP11;
	_buttonGP12 = buttonGP12;
	_buttonGP13 = buttonGP13;
	_buttonGP14 = buttonGP14;
	_buttonGP15 = buttonGP15;
	_buttonGP16 = buttonGP16;
	
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
	_buttonRew = buttonRew;
	_buttonFwd = buttonFwd;

	_buttonMenu = buttonMenu;
	_buttonScrub = buttonScrub;
	_buttonMetronome = buttonMetronome;

	_buttonUtility = buttonUtility;
	_buttonCopy = buttonCopy;
	_buttonPaste = buttonPaste;
	_buttonClear = buttonClear;

	_buttonF1 = buttonF1;
	_buttonF2 = buttonF2;
	_buttonF3 = buttonF3;
	_buttonF4 = buttonF4;

	_buttonDown = buttonLeft; // TODO: change name
	_buttonUp = buttonRight;

	// clear LED states
	for(i=0; i<NUM_LEDS; ++i)
		ledState[i] = 0;

	// init application after ca. 1 mS (this ensures that all objects have been initialized)
	NSTimer *init_timer = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(delayedAPP_Init:) userInfo:nil repeats:NO];
	[[NSRunLoop currentRunLoop] addTimer: init_timer forMode: NSDefaultRunLoopMode];
}

@end
