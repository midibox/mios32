// $Id$
//
//  ControlButton.m
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "ControlButton.h"
#import "blm_scalar_emulationAppDelegate.h"

@implementation ControlButton


#define DEBUG_MESSAGES 0

blm_scalar_emulationAppDelegate *delegate;

NSInteger LED_State;

//////////////////////////////////////////////////////////////////////////////
// init button cell
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	[self setLED_State:0];
	
	// by default, button would only send on MouseUp
	// I haven't found an option in Interface Builder to change this behaviour
	// setting the MouseDown flag helps to overcome this limitation
	[super sendActionOn:NSLeftMouseDownMask | NSLeftMouseUpMask];	
}


//////////////////////////////////////////////////////////////////////////////
// sets the delegate to forward MIDI events
//////////////////////////////////////////////////////////////////////////////
- (id)delegate 
{
	return delegate;
}

- (void)setDelegate:(id)newDelegate {
	delegate = newDelegate;
}


//////////////////////////////////////////////////////////////////////////////
// sets MIDI channel and CC number
//////////////////////////////////////////////////////////////////////////////
- (NSInteger)buttonChn
{
	return buttonChn;
}

- (void)setButtonChn:(NSInteger)chn
{
	buttonChn = chn;
}

- (NSInteger)buttonCC
{
	return buttonCC;
}

- (void)setButtonCC:(NSInteger)cc
{
	buttonCC = cc;
}



//////////////////////////////////////////////////////////////////////////////
// The "LED" abstraction layer...
//////////////////////////////////////////////////////////////////////////////
- (NSInteger)LED_State
{
	return LED_State;
}

- (void)setLED_State:(NSInteger)state
{
	LED_State = state;
	
	// TODO: support colours
	switch( LED_State ) {
		case 1:
		case 2:
		case 3:
			[self highlight:YES];
			break;
		default:
			LED_State = 0;
			[self highlight:NO];
	}
}


//////////////////////////////////////////////////////////////////////////////
// Overrule MouseDown/Up events, send CCs instead
//////////////////////////////////////////////////////////////////////////////

- (void)mouseDown:(NSEvent *)event
{
	// button pressed
	[delegate sendCCEvent_Chn:buttonChn cc:buttonCC value:0x7f];	
}

- (void)mouseUp:(NSEvent *)event
{
	// button released
	[delegate sendCCEvent_Chn:buttonChn cc:buttonCC value:0x00];
}

@end
