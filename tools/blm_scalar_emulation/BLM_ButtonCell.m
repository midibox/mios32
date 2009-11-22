// $Id$
//
//  BLM_ButtonCell.m
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "BLM_ButtonCell.h"
#import "BLM.h"

@implementation BLM_ButtonCell


#define DEBUG_MESSAGES 0

#define NUM_CACHED_LED_STATES 4
NSImage *buttonImages[NUM_CACHED_LED_STATES];


//////////////////////////////////////////////////////////////////////////////
// init button cell
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	[self setLED_State:0];
	
	int i;
	for(i=0; i<NUM_CACHED_LED_STATES; ++i)
		buttonImages[i] = NULL;
}


//////////////////////////////////////////////////////////////////////////////
// From http://www.cocoadev.com/index.pl?CCDColoredButtonCell
//////////////////////////////////////////////////////////////////////////////

- (NSColor *)buttonColor
{
	return buttonColor;
}

- (void)setButtonColor:(NSColor *)color
{
	[buttonColor release];
	buttonColor = [color copy];
}


- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	if (!buttonColor || ([buttonColor isEqualTo:[NSColor clearColor]]) || LED_State >= NUM_CACHED_LED_STATES) {
		[super drawWithFrame:cellFrame inView:controlView];
		return;
	}

	NSRect canvasFrame = NSMakeRect(0, 0, cellFrame.size.width, cellFrame.size.height);
	
	// LED State Cache to speed up switching of button images
	if( buttonImages[LED_State] == NULL ) {
		buttonImages[LED_State] = [[NSImage alloc] initWithSize:cellFrame.size];
		NSImage *colorImage = [[NSImage alloc] initWithSize:cellFrame.size];
		NSImage *cellImage = [[NSImage alloc] initWithSize:cellFrame.size];
	
		[buttonImages[LED_State] setFlipped:[controlView isFlipped]];
	
		// Draw the cell into an image
		[cellImage lockFocus];
		[super drawBezelWithFrame:canvasFrame inView:[NSView focusView]];
		[cellImage unlockFocus];
	
		// Draw the color but only over the opaque parts of the cell image
		[colorImage lockFocus];
		[cellImage drawAtPoint:NSZeroPoint fromRect:canvasFrame operation:NSCompositeSourceOver fraction:1];
		[buttonColor set];
		NSRectFillUsingOperation(canvasFrame, NSCompositeSourceIn);
		[colorImage unlockFocus];
	
		// Mix the colored overlay with the cell image using CompositePlusDarker
		[buttonImages[LED_State] lockFocus];
		[colorImage drawAtPoint:NSZeroPoint fromRect:canvasFrame operation:NSCompositeSourceOver fraction:1];
		[cellImage drawAtPoint:NSZeroPoint fromRect:canvasFrame operation:NSCompositePlusDarker fraction:1];
		[buttonImages[LED_State] unlockFocus];
	}
	
	// Draw the final image to the screen
	[buttonImages[LED_State] drawAtPoint:cellFrame.origin fromRect:canvasFrame operation:NSCompositeSourceOver fraction:1];
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
	
	switch( LED_State ) {
		case 1: [self setButtonColor: [NSColor colorWithCalibratedRed:0.1 green:0.9 blue:0.1 alpha:1.0]]; break;
		case 2: [self setButtonColor: [NSColor colorWithCalibratedRed:0.9 green:0.1 blue:0.1 alpha:1.0]]; break;
		case 3: [self setButtonColor: [NSColor colorWithCalibratedRed:0.9 green:0.6 blue:0.1 alpha:1.0]]; break;
		default:
			LED_State = 0;
			[self setButtonColor: [NSColor colorWithCalibratedRed:1.0 green:1.0 blue:1.0 alpha:1.0]];
	}
//	[self display]; // redraw button
	// doesn't work... workaround:
	[self setHighlighted: YES];
	[self setHighlighted: NO];
}


//////////////////////////////////////////////////////////////////////////////
// Overrule tracking events
//////////////////////////////////////////////////////////////////////////////

- (BOOL)startTrackingAt:(NSPoint)startPoint inView:(NSView *)controlView
{
	NSInteger row, column;
	
	if( [(NSMatrix *)controlView getRow:&row column:&column ofCell:self] ) {
		[self setHighlighted: YES];
#if DEBUG_MESSAGES
		NSLog(@"Pressed %d %d\n", column, row);
#endif
		[(BLM *)controlView sendButtonEvent:column:row:0];
	}

	return YES;
}

- (BOOL)continueTracking:(NSPoint)lastPoint at:(NSPoint)currentPoint inView:(NSView *)controlView
{
	return YES;
}


- (void)stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flag
{
	NSInteger row, column;

	if( [(NSMatrix *)controlView getRow:&row column:&column ofCell:self] ) {
		[self setHighlighted: NO];
#if DEBUG_MESSAGES
		NSLog(@"Depressed %d %d\n", column, row);
#endif
		[(BLM *)controlView sendButtonEvent:column:row:1];
	}
}

- (NSColor *)highlightColorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
	return [NSColor colorWithCalibratedRed:1.1 green:1.0 blue:0.1 alpha:1.0];
}

@end
