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
#include <mios32.h>

@implementation SeqEncoder

int lastAngle;

// located in app.c
extern "C" void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);


//////////////////////////////////////////////////////////////////////////////
// init encoder
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
    [super awakeFromNib];
    // events for touchDown/touchUp already created in SeqButton

    lastAngle = -1;
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
#if 0
    [self setLedState:2];
#endif
    lastAngle = -1;
}

- (void)touchUp:(UIEvent*)event
{
#if 0
    [self setLedState:0];
#endif
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
#if 0
    [self setLedState:3];
#endif

	UITouch *touch = [touches anyObject];
	if( [touch view] == self ) {
		CGPoint location = [touch locationInView:self];
        CGFloat width = CGRectGetWidth(self.frame);
        CGFloat height = CGRectGetHeight(self.frame);
        CGFloat relX = location.x - width/2;
        CGFloat relY = height/2 - location.y;
        CGFloat angle;

        // from my beloved "Mathematische Formelsammlung" by Lothar Papula :)
        if( relX == 0 ) {
            if( relY == 0 )
                angle = 0;
            else
                angle = (relY > 0 ) ? M_PI/2 : (3*M_PI/2);
        } else {
            angle = atan(relY/relX);
            if( relX < 0 )
                angle += M_PI;
            else if( relY < 0 )
                angle += 2*M_PI;
        }

        angle = 180 * angle / M_PI;

#if 0
        NSLog(@"Touched %d: %f %f %f", [self tag], relX, relY, angle);
#endif
        if( lastAngle >= 0 ) {
            CGFloat delta = lastAngle - angle;
            if( delta > 300 ) // we assume a transition from >360°
                delta -= 360;
            else if( delta < -300 )
                delta += 360;

#if 0
            NSLog(@"Delta: %f", delta);
#endif

            // notify application
            APP_ENC_NotifyChange([self tag], (delta >= 0) ? 1 : -1);
        }

        lastAngle = angle;
	}
}

@end
