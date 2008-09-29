// $Id: //
//
//  CLCDView.h
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

// increase if required
#define LCD_MAX_COLUMNS 80
#define LCD_MAX_LINES    8


@interface CLCDView : NSView {

	float		LCDBColorR, LCDBColorG, LCDBColorB; // background color
	float		LCDFColorR, LCDFColorG, LCDFColorB; // foreground color

	unsigned	LCDColumns; // number of characters per line (default 40)
	unsigned	LCDLines;  // number of lines (default 2)
	
	unsigned	LCDCharWidth; // width of a single character (default 6)
	unsigned	LCDCharHeight; // height of a single character (default 8)
	
	unsigned	LCDOffsetColumns; // offset between columns (default 0)
	unsigned	LCDOffsetLines; // offset between lines (default 4)
	
	unsigned	LCDPixelX;	// X dimension of a pixel (default 2)
	unsigned	LCDPixelY;	// Y dimension of a pixel (default 2)

	unsigned	LCDBorderX;	// left/right distance to frame (default 4)
	unsigned	LCDBorderY;	// upper/lower distance to frame (default 8)
	
	unsigned	LCDCursorX; // cursor X pos (default 0)
	unsigned	LCDCursorY; // cursor Y pos (default 0)
	
	char		LCDScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];
}

// LCD Access functions
- (void)LCDClear;
- (void)LCDPrintChar: (char)c;
- (void)LCDPrintString: (char *)str;
- (void)LCDSpecialCharInit: (unsigned char)num: (unsigned char *)table;
- (void)LCDSpecialCharsInit: (unsigned char *)table;

// get/set functions (don't use synthesize feature as described in tutorial to stay compatible with older Xcode versions)
- (void)setLCDBColor: (float)r: (float)g: (float)b;
- (void)setLCDFColor: (float)r: (float)g: (float)b;
- (unsigned)getLCDColumns;
- (void)setLCDColumns: (unsigned)num;
- (unsigned)getLCDLines;
- (void)setLCDLines: (unsigned)num;
- (unsigned)getLCDCharWidth;
- (void)setLCDCharWidth: (unsigned)num;
- (unsigned)getLCDCharHeight;
- (void)setLCDCharHeight: (unsigned)num;
- (unsigned)getLCDOffsetColumns;
- (void)setLCDOffsetColumns: (unsigned)num;
- (unsigned)getLCDOffsetLines;
- (void)setLCDOffsetLines: (unsigned)num;
- (unsigned)getLCDPixelX;
- (void)setLCDPixelX: (unsigned)num;
- (unsigned)getLCDPixelY;
- (void)setLCDPixelY: (unsigned)num;
- (unsigned)getLCDBorderX;
- (void)setLCDBorderX: (unsigned)num;
- (unsigned)getLCDBorderY;
- (void)setLCDBorderY: (unsigned)num;
- (unsigned)getLCDCursorX;
- (void)setLCDCursorX: (unsigned) num;
- (unsigned)getLCDCursorY;
- (void)setLCDCursorY: (unsigned) num;

@end
