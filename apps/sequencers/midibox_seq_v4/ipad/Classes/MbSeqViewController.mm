/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  MbSeqViewController.m
//  MbSeq
//
//  Created by Thorsten Klose on 15.04.10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "MbSeqViewController.h"
#import "MIOS32_SDCARD_Wrapper.h"

#include <mios32.h>
#include <app.h>
#include <app_lcd.h>
#include <tasks.h>

@implementation MbSeqViewController

// local variables to bridge objects to C functions
static NSObject *_self;

// shadow registers for DOUT pins
u8 dout_sr_shadow[MIOS32_SRIO_NUM_SR];

// LED reverences
#define NUM_LEDS 17
u8 ledState[NUM_LEDS]; // for dual-colour option

static SeqButton *_buttonGP1;
static SeqButton *_buttonGP2;
static SeqButton *_buttonGP3;
static SeqButton *_buttonGP4;
static SeqButton *_buttonGP5;
static SeqButton *_buttonGP6;
static SeqButton *_buttonGP7;
static SeqButton *_buttonGP8;
static SeqButton *_buttonGP9;
static SeqButton *_buttonGP10;
static SeqButton *_buttonGP11;
static SeqButton *_buttonGP12;
static SeqButton *_buttonGP13;
static SeqButton *_buttonGP14;
static SeqButton *_buttonGP15;
static SeqButton *_buttonGP16;

static SeqButton *_buttonTrack1;
static SeqButton *_buttonTrack2;
static SeqButton *_buttonTrack3;
static SeqButton *_buttonTrack4;

static SeqButton *_buttonGroup1;
static SeqButton *_buttonGroup2;
static SeqButton *_buttonGroup3;
static SeqButton *_buttonGroup4;

static SeqButton *_buttonPLayerA;
static SeqButton *_buttonPLayerB;
static SeqButton *_buttonPLayerC;

static SeqButton *_buttonTLayerA;
static SeqButton *_buttonTLayerB;
static SeqButton *_buttonTLayerC;

static SeqButton *_buttonEdit;
static SeqButton *_buttonMute;
static SeqButton *_buttonPattern;
static SeqButton *_buttonSong;

static SeqButton *_buttonSolo;
static SeqButton *_buttonFast;
static SeqButton *_buttonAll;

static SeqButton *_buttonStepView;

static SeqButton *_buttonPlay;
static SeqButton *_buttonStop;
static SeqButton *_buttonPause;
static SeqButton *_buttonRew;
static SeqButton *_buttonFwd;

static SeqButton *_buttonMenu;
static SeqButton *_buttonScrub;
static SeqButton *_buttonMetronome;

static SeqButton *_buttonUtility;
static SeqButton *_buttonCopy;
static SeqButton *_buttonPaste;
static SeqButton *_buttonClear;

static SeqButton *_buttonF1;
static SeqButton *_buttonF2;
static SeqButton *_buttonF3;
static SeqButton *_buttonF4;

static SeqButton *_buttonUp;
static SeqButton *_buttonDown;

static SeqEncoder *_encDataWheel;
static SeqEncoder *_encGP1;
static SeqEncoder *_encGP2;
static SeqEncoder *_encGP3;
static SeqEncoder *_encGP4;
static SeqEncoder *_encGP5;
static SeqEncoder *_encGP6;
static SeqEncoder *_encGP7;
static SeqEncoder *_encGP8;
static SeqEncoder *_encGP9;
static SeqEncoder *_encGP10;
static SeqEncoder *_encGP11;
static SeqEncoder *_encGP12;
static SeqEncoder *_encGP13;
static SeqEncoder *_encGP14;
static SeqEncoder *_encGP15;
static SeqEncoder *_encGP16;



//////////////////////////////////////////////////////////////////////////////
// Emulation specific LED access functions
//////////////////////////////////////////////////////////////////////////////
extern "C" s32 EMU_DOUT_PinSet(u32 pin, u32 value)
{
	int gp_led = -1;
	
	pin ^= 7;
	
	// GP LEDs
	if( pin >= 16 && pin < 32 ) {
		// disabled - using highlight function of GP button instead
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
			[_encDataWheel setLedState:2];
		else
			[_encDataWheel setLedState:0];
	}

	// forward LED functions to encoders
    switch( gp_led ) {
    case 0: [_encGP1 setLedState:ledState[gp_led]]; break;
    case 1: [_encGP2 setLedState:ledState[gp_led]]; break;
    case 2: [_encGP3 setLedState:ledState[gp_led]]; break;
    case 3: [_encGP4 setLedState:ledState[gp_led]]; break;
    case 4: [_encGP5 setLedState:ledState[gp_led]]; break;
    case 5: [_encGP6 setLedState:ledState[gp_led]]; break;
    case 6: [_encGP7 setLedState:ledState[gp_led]]; break;
    case 7: [_encGP8 setLedState:ledState[gp_led]]; break;
    case 8: [_encGP9 setLedState:ledState[gp_led]]; break;
    case 9: [_encGP10 setLedState:ledState[gp_led]]; break;
    case 10: [_encGP11 setLedState:ledState[gp_led]]; break;
    case 11: [_encGP12 setLedState:ledState[gp_led]]; break;
    case 12: [_encGP13 setLedState:ledState[gp_led]]; break;
    case 13: [_encGP14 setLedState:ledState[gp_led]]; break;
    case 14: [_encGP15 setLedState:ledState[gp_led]]; break;
    case 15: [_encGP16 setLedState:ledState[gp_led]]; break;
	}

	
	switch( pin ) {
		case  0: [_buttonTrack1 setLedState:(value ? 3 : 0)]; break;
		case  1: [_buttonTrack2 setLedState:(value ? 3 : 0)]; break;
		case  2: [_buttonTrack3 setLedState:(value ? 3 : 0)]; break;
		case  3: [_buttonTrack4 setLedState:(value ? 3 : 0)]; break;
		
		case  4: [_buttonPLayerA setLedState:(value ? 3 : 0)]; break;
		case  5: [_buttonPLayerB setLedState:(value ? 3 : 0)]; break;
		case  6: [_buttonPLayerC setLedState:(value ? 3 : 0)]; break;

		case 16: [_buttonGP1 setLedState:(value ? 3 : 0)]; break;
		case 17: [_buttonGP2 setLedState:(value ? 3 : 0)]; break;
		case 18: [_buttonGP3 setLedState:(value ? 3 : 0)]; break;
		case 19: [_buttonGP4 setLedState:(value ? 3 : 0)]; break;
		case 20: [_buttonGP5 setLedState:(value ? 3 : 0)]; break;
		case 21: [_buttonGP6 setLedState:(value ? 3 : 0)]; break;
		case 22: [_buttonGP7 setLedState:(value ? 3 : 0)]; break;
		case 23: [_buttonGP8 setLedState:(value ? 3 : 0)]; break;
		case 24: [_buttonGP9 setLedState:(value ? 3 : 0)]; break;
		case 25: [_buttonGP10 setLedState:(value ? 3 : 0)]; break;
		case 26: [_buttonGP11 setLedState:(value ? 3 : 0)]; break;
		case 27: [_buttonGP12 setLedState:(value ? 3 : 0)]; break;
		case 28: [_buttonGP13 setLedState:(value ? 3 : 0)]; break;
		case 29: [_buttonGP14 setLedState:(value ? 3 : 0)]; break;
		case 30: [_buttonGP15 setLedState:(value ? 3 : 0)]; break;
		case 31: [_buttonGP16 setLedState:(value ? 3 : 0)]; break;

		case 80: [_buttonGroup1 setLedState:(value ? 3 : 0)]; break;
		case 82: [_buttonGroup2 setLedState:(value ? 3 : 0)]; break;
		case 84: [_buttonGroup3 setLedState:(value ? 3 : 0)]; break;
		case 86: [_buttonGroup4 setLedState:(value ? 3 : 0)]; break;

		case 88: [_buttonTLayerA setLedState:(value ? 3 : 0)]; break;
		case 89: [_buttonTLayerB setLedState:(value ? 3 : 0)]; break;
		case 90: [_buttonTLayerC setLedState:(value ? 3 : 0)]; break;

		case  8: [_buttonEdit setLedState:(value ? 3 : 0)]; break;
		case  9: [_buttonMute setLedState:(value ? 3 : 0)]; break;
		case 10: [_buttonPattern setLedState:(value ? 3 : 0)]; break;
		case 11: [_buttonSong setLedState:(value ? 3 : 0)]; break;
			
		case 12: [_buttonSolo setLedState:(value ? 3 : 0)]; break;
		case 13: [_buttonFast setLedState:(value ? 3 : 0)]; break;
		case 14: [_buttonAll setLedState:(value ? 3 : 0)]; break;
		
		case 91: [_buttonPlay setLedState:(value ? 3 : 0)]; break;
		case 92: [_buttonStop setLedState:(value ? 3 : 0)]; break;
		case 93: [_buttonPause setLedState:(value ? 3 : 0)]; break;
		case 94: [_buttonRew setLedState:(value ? 3 : 0)]; break;
		case 95: [_buttonFwd setLedState:(value ? 3 : 0)]; break;
			
		case 96: [_buttonMenu setLedState:(value ? 3 : 0)]; break;
		case 97: [_buttonScrub setLedState:(value ? 3 : 0)]; break;
		case 98: [_buttonMetronome setLedState:(value ? 3 : 0)]; break;

		case 99: [_buttonUtility setLedState:(value ? 3 : 0)]; break;
		case 100: [_buttonCopy setLedState:(value ? 3 : 0)]; break;
		case 101: [_buttonPaste setLedState:(value ? 3 : 0)]; break;
		case 102: [_buttonClear setLedState:(value ? 3 : 0)]; break;

		case 104: [_buttonF1 setLedState:(value ? 3 : 0)]; break;
		case 105: [_buttonF2 setLedState:(value ? 3 : 0)]; break;
		case 106: [_buttonF3 setLedState:(value ? 3 : 0)]; break;
		case 107: [_buttonF4 setLedState:(value ? 3 : 0)]; break;

		case 109: [_buttonStepView setLedState:(value ? 3 : 0)]; break;
		
		case 110: [_buttonDown setLedState:(value ? 3 : 0)]; break;
		case 111: [_buttonUp setLedState:(value ? 3 : 0)]; break;
	}

	return 0;
}

extern "C" s32 EMU_DOUT_SRSet(u32 sr, u8 value)
{
	// optimisation: only update DOUT pins in GUI if SR value has changed
	if( dout_sr_shadow[sr] == value )
		return 0;

	// same optimisation for individual pins
	u8 mask = 1;
	for(int i=0; i<8; ++i) {
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
extern "C" s32 EMU_DIN_NotifyToggle(u32 pin, u32 value)
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
extern "C" s32 UI_printf(char *format, ...)
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

extern "C" void SRIO_ServiceFinish(void)
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
	MIOS32_IRQ_Disable(); // since no IRQs with higher priority are emulated, we have to use a lock in the MIOS32 IRQ Wrapper
	MIOS32_SRIO_ScanStart((void *)&SRIO_ServiceFinish);

	// emulation: transfer new DOUT SR values to GUI LED elements
	int sr;
	for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
		EMU_DOUT_SRSet(sr, mios32_srio_dout[MIOS32_SRIO_NUM_SR-sr-1]);

	MIOS32_IRQ_Enable();

#ifndef MIOS32_DONT_USE_DIN
	// check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
	MIOS32_DIN_Handler((void *)&APP_DIN_NotifyToggle);
#endif
}

- (void)periodicMIDITask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	while (YES) {
		// TODO: find better more FreeRTOS/MIOS32 compliant solution
		// check for incoming MIDI messages and call hooks
		MIOS32_MIDI_Receive_Handler((void *)&APP_MIDI_NotifyPackage);

		// -> forward to application
		SEQ_TASK_MIDI();
	
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}

- (void)periodic1mSTask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	while (YES) {		
		SEQ_TASK_Period1mS();
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}

- (void)periodic1mSTask_LowPrio:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	while (YES) {		
		SEQ_TASK_Period1mS_LowPrio();

        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    }
	
	[pool release];
	[NSThread exit];
}

- (void)periodic1STask:(id)anObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	while (YES) {		
		SEQ_TASK_Period1S();
        [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.000]];
    }
	
	[pool release];
	[NSThread exit];
}

/////////////////////////////////////////////////////////////////////////////
// This task is triggered from SEQ_PATTERN_Change to transport the new patch
// into RAM
/////////////////////////////////////////////////////////////////////////////
extern "C" void TASK_Pattern(void *pvParameters)
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
extern "C" void SEQ_TASK_PatternResume(void)
{
//    vTaskResume(xPatternHandle);

	// MacOS: call task directly

    SEQ_TASK_Pattern();
}


//////////////////////////////////////////////////////////////////////////////
// MSD access not supported by Emulation
//////////////////////////////////////////////////////////////////////////////

extern "C" s32 TASK_MSD_EnableSet(u8 enable)
{
	return -1; // not supported
}

extern "C" s32 TASK_MSD_EnableGet()
{
	return -1; // not supported
}

extern "C" s32 TASK_MSD_FlagStrGet(char str[5])
{
	str[0] = '-';
	str[1] = '-';
	str[2] = '-';
	str[3] = '-';
	str[4] = 0;
	
	return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// initialisation hook for OS specific tasks
// (called from APP_Init() after everything has been prepared)
//////////////////////////////////////////////////////////////////////////////
extern "C" s32 TASKS_Init(u32 mode)
{
	// install SRIO task
	NSTimer *timer1 = [NSTimer timerWithTimeInterval:0.001 target:_self selector:@selector(periodicSRIOTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer1 forMode: NSDefaultRunLoopMode];

	// Detach the new threads
	[NSThread detachNewThreadSelector:@selector(periodicMIDITask:) toTarget:_self withObject:nil];
	[NSThread detachNewThreadSelector:@selector(periodic1mSTask:) toTarget:_self withObject:nil];
	[NSThread detachNewThreadSelector:@selector(periodic1mSTask_LowPrio:) toTarget:_self withObject:nil];
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
    CLcd* cLcd0 = APP_LCD_GetComponentPtr(0);
    cLcd0->setLcdColumns(40);
    cLcd0->setLcdLines(2);
    [cLcdLeft setCLcd:cLcd0];

    CLcd* cLcd1 = APP_LCD_GetComponentPtr(1);
    cLcd1->setLcdColumns(40);
    cLcd1->setLcdLines(2);
    [cLcdRight setCLcd:cLcd1];
	MIOS32_LCD_Init(0);

    for(int lcd=0; lcd<2; ++lcd) {
        MIOS32_LCD_DeviceSet(lcd);
        MIOS32_LCD_BColourSet(0x1efd00);
        MIOS32_LCD_FColourSet(0x333333);
        MIOS32_LCD_Clear();
    }
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

#if 0
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
#else
		SDCARD_Wrapper_setDir(@".");  // always select local dir
#endif

}


//////////////////////////////////////////////////////////////////////////////
// periodic LCD update
//////////////////////////////////////////////////////////////////////////////
- (void)lcdUpdate:(id)anObject
{
    [cLcdLeft periodicUpdate];
    [cLcdRight periodicUpdate];
}


/*
// The designated initializer. Override to perform setup that is required before the view is loaded.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];

    // initialize global C variables
	_self = self;

	for(int i=0; i<MIOS32_SRIO_NUM_SR; ++i)
		dout_sr_shadow[i] = 0x00;

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

    _encDataWheel = encDataWheel;
    _encGP1 = encGP1;
    _encGP2 = encGP2;
    _encGP3 = encGP3;
    _encGP4 = encGP4;
    _encGP5 = encGP5;
    _encGP6 = encGP6;
    _encGP7 = encGP7;
    _encGP8 = encGP8;
    _encGP9 = encGP9;
    _encGP10 = encGP10;
    _encGP11 = encGP11;
    _encGP12 = encGP12;
    _encGP13 = encGP13;
    _encGP14 = encGP14;
    _encGP15 = encGP15;
    _encGP16 = encGP16;


	// clear LED states
	for(int i=0; i<NUM_LEDS; ++i)
		ledState[i] = 0;

	// init application after ca. 1 mS (this ensures that all objects have been initialized)
	NSTimer *initTimer = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(delayedAPP_Init:) userInfo:nil repeats:NO];
	[[NSRunLoop currentRunLoop] addTimer: initTimer forMode: NSDefaultRunLoopMode];

	// periodic LCD update
	NSTimer *lcdTimer = [NSTimer timerWithTimeInterval:0.010 target:self selector:@selector(lcdUpdate:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: lcdTimer forMode: NSDefaultRunLoopMode];
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return YES;
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}

@end
