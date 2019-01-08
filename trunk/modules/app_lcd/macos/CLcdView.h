/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  CLcdView.h
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

class CLcd; // forward reference

@interface CLcdView : NSView {

    CLcd *cLcd;

    unsigned lcdCharWidth; // width of a single character (default 6)
    unsigned lcdCharHeight; // height of a single character (default 8)
        
    unsigned lcdOffsetColumns; // offset between columns (default 0)
    unsigned lcdOffsetLines; // offset between lines (default 4)
        
    unsigned lcdPixelX;      // X dimension of a pixel (default 2)
    unsigned lcdPixelY;      // Y dimension of a pixel (default 2)

    unsigned lcdBorderX;     // left/right distance to frame (default 4)
    unsigned lcdBorderY;     // upper/lower distance to frame (default 8)
}

- (void)setCLcd: (CLcd*)_cLcd;
- (void)periodicUpdate;

- (unsigned)getLcdSizeX;
- (unsigned)getLcdSizeY;

@end
