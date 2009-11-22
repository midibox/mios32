// $Id$
//
//  BLM.m
//  blm_scalar_emulation
//
//  Created by Thorsten Klose on 19.11.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "BLM.h"
#import "BLM_ButtonCell.h"
#import "blm_scalar_emulationAppDelegate.h"

@implementation BLM

#define DEBUG_MESSAGES 0

blm_scalar_emulationAppDelegate *delegate;


//////////////////////////////////////////////////////////////////////////////
// init Button/LED Matrix
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	[self setColumns:16 rows:16];
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
// Changes BLM dimensions
//////////////////////////////////////////////////////////////////////////////
- (void)setColumns:(NSInteger)columns rows:(NSInteger)rows
{
	[self renewRows:rows columns:columns];
	[self sizeToCells];

	int column, row;
	for(column=0; column<[self numberOfColumns]; ++column)
		for(row=0; row<[self numberOfRows]; ++row)
			[self setLED_State:column:row:0];
}



//////////////////////////////////////////////////////////////////////////////
// Called from Button Cells
//////////////////////////////////////////////////////////////////////////////
- (void)sendButtonEvent:(NSInteger)column:(NSInteger)row:(NSInteger)depressed
{
#if DEBUG_MESSAGES
	NSLog(@"BLM got event from button %d %d: %d\n", column, row, depressed);
#endif

	NSInteger chn = row;
	NSInteger key = column;
	NSInteger velocity = depressed ? 0x00 : 0x7f;
	[delegate sendNoteEvent:chn:key:velocity];
}


//////////////////////////////////////////////////////////////////////////////
// Returns number of supported LED Colours
//////////////////////////////////////////////////////////////////////////////
- (NSInteger)numberOfLEDColours
{
	return 2;
}


//////////////////////////////////////////////////////////////////////////////
// Called from delegate to access a LED
//////////////////////////////////////////////////////////////////////////////
- (NSInteger)LED_State:(NSInteger)column:(NSInteger)row
{
	return [(BLM_ButtonCell *)[self cellAtRow:row column:column] LED_State];
}

- (void)setLED_State:(NSInteger)column:(NSInteger)row:(NSInteger)state
{
#if DEBUG_MESSAGES
	NSLog(@"BLM changes LED state of %d %d: %d\n", column, row, state);
#endif
	[(BLM_ButtonCell *)[self cellAtRow:row column:column] setLED_State:state];
//	[self setState:0 atRow:row column:column]; // redraws cell
}


@end
