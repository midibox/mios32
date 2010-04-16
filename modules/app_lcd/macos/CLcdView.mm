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

	// calculate X/Y dimensions of LCD
	unsigned lcd_size_x = [self getLcdSizeX];
	unsigned lcd_size_y = [self getLcdSizeY];

	
	NSBezierPath *BP = [NSBezierPath bezierPathWithRect: [self bounds]];
	[[NSColor colorWithDeviceRed:cLcd->bColorR/255.0 green:cLcd->bColorG/255.0 blue:cLcd->bColorB/255.0 alpha:1.] set];
	[BP fill];
    BP = [NSBezierPath bezierPathWithRect: NSMakeRect(0, 0, lcd_size_x, lcd_size_y)];

	// print each character
	// TODO: optimized version, which buffers characters during printChar() to avoid that complete bitmap has to be re-created on each redraw
	for(int line=0; line<cLcd->lcdLines; ++line) {
		for(int column=0; column<cLcd->lcdColumns; ++column) {
            unsigned char c = cLcd->lcdScreen[line][column];
			unsigned char *chr = c < 8 ? cLcd->customCharset[c] : charset[c];
	
			for(int y=0; y<lcdCharHeight; ++y) {
				for(int x=0; x<lcdCharWidth; ++x) {
					if( chr[y] & (1 << (lcdCharWidth-x-1)) ) {
						[[NSColor colorWithDeviceRed:cLcd->fColorR/255.0 green:cLcd->fColorG/255.0 blue:cLcd->fColorB/255.0 alpha:1.] set];
					} else {
						[[NSColor colorWithDeviceRed:cLcd->bColorR/255.0 green:cLcd->bColorG/255.0 blue:cLcd->bColorB/255.0 alpha:1.] set];
					}
					[BP fill];
                    BP = [NSBezierPath bezierPathWithRect: NSMakeRect(
                       lcdBorderX + lcdPixelX*(column*lcdCharWidth+x) + column*lcdOffsetColumns,
                       lcd_size_y - lcdBorderY - lcdPixelY*(y+line*(lcdCharHeight+lcdOffsetLines)), 
                       lcdPixelX, lcdPixelY)]; 
				}
			}
		}
	}

    cLcd->updateDone();
}

@end
