/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  SeqEncoder.mm
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 30.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SeqEncoder.h"

@implementation SeqEncoder


//////////////////////////////////////////////////////////////////////////////
// init encoder
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
    [super awakeFromNib];
    // events for touchDown/touchUp already created in SeqButton
    // define additional event for encoder movements
    [super addTarget:self action:@selector(moveEncoder:) forControlEvents:UIControlEventTouchDragInside];
}


//////////////////////////////////////////////////////////////////////////////
// Draw encoder
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

    CGContextSetRGBFillColor(context, 0.0, 0.0, 0.0, 1.0);
    CGFloat xReduced = 0.6*width;
    CGFloat yReduced = 0.6*height;
	CGContextFillEllipseInRect(context, CGRectMake(xReduced/2, yReduced/2, width-xReduced, height-yReduced));
	CGContextStrokeEllipseInRect(context, CGRectMake(xReduced/2, yReduced/2, width-xReduced, height-yReduced));
}


//////////////////////////////////////////////////////////////////////////////
// Touch Events
//////////////////////////////////////////////////////////////////////////////
- (void)touchDown:(UIEvent*)event
{
    [self setLedState:2];
}

- (void)touchUp:(UIEvent*)event
{
    [self setLedState:0];
}

- (void)moveEncoder:(UIEvent*)event
{
    [self setLedState:3];
}


@end
