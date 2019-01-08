/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  SeqButton.mm
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SeqButton.h"

@implementation SeqButton

#include <mios32.h>

// forward declaration - function is located in MbSeqViewController.mm
extern "C" s32 EMU_DIN_NotifyToggle(u32 pin, u32 value);


//////////////////////////////////////////////////////////////////////////////
// init button
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
    [self setLedState:0];
        
    [super addTarget:self action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown|UIControlEventTouchDragEnter];
    [super addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside|UIControlEventTouchDragExit|UIControlEventTouchCancel];
}


//////////////////////////////////////////////////////////////////////////////
// Draw button
//////////////////////////////////////////////////////////////////////////////
- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGFloat width = CGRectGetWidth(rect);
    CGFloat height = CGRectGetHeight(rect);

    CGContextSetLineWidth(context, 2.0);
    CGContextSetRGBStrokeColor(context, 0.3, 0.3, 0.3, 1.0);

    switch( ledState ) {
    case 1:
        CGContextSetRGBFillColor(context,   0.1, 0.9, 0.1, 1.0);
        break;
    case 2:
        CGContextSetRGBFillColor(context,   0.9, 0.1, 0.1, 1.0);
        break;
    case 3:
        CGContextSetRGBFillColor(context,   0.9, 0.6, 0.1, 1.0);
        break;
    default:
        CGContextSetRGBFillColor(context,   1.0, 1.0, 1.0, 1.0);
        break;
    }
    
	CGContextFillEllipseInRect(context, CGRectMake(0.0, 0.0, width, height));
	CGContextStrokeEllipseInRect(context, CGRectMake(0.0, 0.0, width, height));
}


//////////////////////////////////////////////////////////////////////////////
// The "LED" abstraction layer...
//////////////////////////////////////////////////////////////////////////////
- (unsigned)getLedState
{
    return ledState;
}

- (void)setLedState:(unsigned)state
{
    ledState = state;

    // update display
    [self setNeedsDisplay];
}


//////////////////////////////////////////////////////////////////////////////
// Touch Events
//////////////////////////////////////////////////////////////////////////////
- (void)touchDown:(UIEvent*)event
{
	// button pressed
	EMU_DIN_NotifyToggle([self tag], 0);
}

- (void)touchUp:(UIEvent*)event
{
	// button released
	EMU_DIN_NotifyToggle([self tag], 1);
}



@end
