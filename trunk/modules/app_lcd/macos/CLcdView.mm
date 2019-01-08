/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  CLCDView.mm
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "CLcdView.h"

#include "CLcd.h"


@implementation CLcdView

// include default character set from separate file
#include "CLcdView_charset.inc"

//////////////////////////////////////////////////////////////////////////////
// Frame initialisation
//////////////////////////////////////////////////////////////////////////////
- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	return self;
}

- (void)awakeFromNib
{
    lcdCharWidth = 6;
    lcdCharHeight = 8;
	
    lcdOffsetColumns = 0;
    lcdOffsetLines = 4;
	
    lcdPixelX = 2;
    lcdPixelY = 2;
	
    lcdBorderX = 4;
    lcdBorderY = 8;
}


//////////////////////////////////////////////////////////////////////////////
// Sets reference to C++ based CLcd object
//////////////////////////////////////////////////////////////////////////////
- (void)setCLcd: (CLcd*)_cLcd;
{
    cLcd = _cLcd;
}


//////////////////////////////////////////////////////////////////////////////
// Requests display update if required
// should be called from application periodically
//////////////////////////////////////////////////////////////////////////////
- (void)periodicUpdate
{
    // exit so long cLcd object hasn't been assigned
    if( !cLcd )
        return;

    // exit if no update required
    if( !cLcd->updateRequired() )
        return;

    // update display
    [self setNeedsDisplay:YES];
}


//////////////////////////////////////////////////////////////////////////////
// Returns display dimensions
//////////////////////////////////////////////////////////////////////////////
- (unsigned)getLcdSizeX
{ 
    // exit so long cLcd object hasn't been assigned
    if( !cLcd )
        return 0;

	return (2*lcdBorderX) + (lcdPixelX*cLcd->lcdColumns*lcdCharWidth) + (lcdOffsetColumns*(cLcd->lcdColumns-1)) + 2;
}

- (unsigned)getLcdSizeY
{ 	
    // exit so long cLcd object hasn't been assigned
    if( !cLcd )
        return 0;

	return (2*lcdBorderY) + (lcdPixelY*cLcd->lcdLines*lcdCharHeight) + (lcdOffsetLines*(cLcd->lcdLines-1)); 
}


//////////////////////////////////////////////////////////////////////////////
// (Re-)draw view
//////////////////////////////////////////////////////////////////////////////  
- (void)drawRect:(NSRect)rect {

    // exit so long cLcd object hasn't been assigned
    if( !cLcd )
        return;

	unsigned lcdSizeX = [self getLcdSizeX];
	unsigned lcdSizeY = [self getLcdSizeY];

    CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];

    CGContextSetRGBFillColor(context, cLcd->bColorR/255.0, cLcd->bColorG/255.0, cLcd->bColorB/255.0, 1.0);
    CGContextFillRect(context, CGRectMake(0, 0, lcdSizeX, lcdSizeY));

    CGLayerRef layer0 = CGLayerCreateWithContext(context, CGSizeMake(lcdPixelX, lcdPixelY), NULL);
    CGContextRef layerContext0 = CGLayerGetContext(layer0);
    CGContextSetRGBFillColor(layerContext0, cLcd->bColorR/255.0, cLcd->bColorG/255.0, cLcd->bColorB/255.0, 1.0);
    CGContextFillRect(layerContext0, CGRectMake(0, 0, lcdPixelX, lcdPixelY));

    CGLayerRef layer1 = CGLayerCreateWithContext(context, CGSizeMake(lcdPixelX, lcdPixelY), NULL);
    CGContextRef layerContext1 = CGLayerGetContext(layer1);
    CGContextSetRGBFillColor(layerContext1, cLcd->fColorR/255.0, cLcd->fColorG/255.0, cLcd->fColorB/255.0, 1.0);
    CGContextFillRect(layerContext1, CGRectMake(0, 0, lcdPixelX, lcdPixelY));

	// print characters that have been modified
	for(int line=0; line<cLcd->lcdLines; ++line) {
		for(int column=0; column<cLcd->lcdColumns; ++column) {
            unsigned char c = cLcd->lcdScreen[line][column];
            unsigned char *chr = (c < 8) ? cLcd->customCharset[c] : charset[c];
	
            unsigned rectY = lcdSizeY - lcdBorderY - lcdPixelY*line*(lcdCharHeight+lcdOffsetLines);
            for(int y=0; y<lcdCharHeight; ++y, rectY-=lcdPixelY) {
                unsigned rectX = lcdBorderX + lcdPixelX*column*lcdCharWidth + column*lcdOffsetColumns;
                unsigned mask = (1 << (lcdCharWidth-1));
                for(int x=0; x<lcdCharWidth; ++x, rectX+=lcdPixelX, mask>>=1) {
                    CGPoint p = CGPointMake(rectX, rectY);
                    if( chr[y] & mask )
                        CGContextDrawLayerAtPoint (context, p, layer1);
                    else
                        CGContextDrawLayerAtPoint (context, p, layer0);
                }
            }
        }
	}

    CGLayerRelease(layer0);
    CGLayerRelease(layer1);

    cLcd->updateDone();
}

@end
