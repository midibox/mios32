// $Id: SEQButton.m 159 2008-12-06 01:33:47Z tk $
//
//  SEQButton.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SEQButton.h"
#import "UI.h"

@implementation SEQButton


//////////////////////////////////////////////////////////////////////////////
// init special button behaviour
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	// by default, button would only send on MouseUp
	// I haven't found an option in Interface Builder to change this behaviour
	// setting the MouseDown flag helps to overcome this limitation
	[super sendActionOn:NSLeftMouseDownMask | NSLeftMouseUpMask];
}

//////////////////////////////////////////////////////////////////////////////
// Overrule MouseDown/Up events, call DIN_NotifyToggle instead
//////////////////////////////////////////////////////////////////////////////

- (void)mouseDown:(NSEvent *)event
{
	// button pressed

	EMU_DIN_NotifyToggle([self tag], 0);
}

- (void)mouseUp:(NSEvent *)event
{
	// button released
	EMU_DIN_NotifyToggle([self tag], 1);
}

@end
