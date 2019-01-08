// $Id$
//
//  SEQEncoder.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SEQEncoder.h"

@implementation SEQEncoder

// to determine the increments
float lastValue;


//////////////////////////////////////////////////////////////////////////////
// init encoder
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	// store initial value
	lastValue = [self floatValue];
	
	// install action on slider moves
	[self setTarget:self];
	[self setAction:@selector(encoderMoved:)];
}


//////////////////////////////////////////////////////////////////////////////
// called on slider moves, determines the incrementer and forwards it to ENC_NotifyChange
//////////////////////////////////////////////////////////////////////////////
- (IBAction)encoderMoved:(id)sender
{
	// determine difference to previous value and convert to integer
	float value = [self floatValue];
	float diff = value - lastValue;
		
	if( diff != 0.0 ) {
		lastValue = value;
		int incrementer = (int)diff;
		if( incrementer == 0 )
			incrementer = diff >= 0 ? 1 : -1;
			
		APP_ENC_NotifyChange([self tag], incrementer); 
	}
}


//////////////////////////////////////////////////////////////////////////////
// called on scrollwheel moves
//////////////////////////////////////////////////////////////////////////////
- (void)scrollWheel:(NSEvent *)event
{
	// determine incrementer and forward to ENC_NotifyChange
	float diff = [event deltaY];
	int incrementer = (int)diff;

	if( incrementer == 0 )
		incrementer = diff >= 0 ? 1 : -1;
			
	APP_ENC_NotifyChange([self tag], incrementer); 
	
	// move dial as well (we have to wrap if end/begin value reached)
	float newValue = [self floatValue] + [event deltaY];
	if( newValue < [self minValue] )
		newValue = [self maxValue];
	if( newValue > [self maxValue] )
		newValue = [self minValue];

	[self setFloatValue:newValue];
}

@end
