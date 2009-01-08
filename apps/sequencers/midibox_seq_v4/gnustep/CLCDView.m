// $Id: CLCDView.m 41 2008-09-30 17:20:12Z tk $
//
//  CLCDView.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "CLCDView.h"

@implementation CLCDView



// include default character set from separate file
#include "CLCDView_charset.inc"

//////////////////////////////////////////////////////////////////////////////
// Frame initialisation
//////////////////////////////////////////////////////////////////////////////
- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];

	[self setLCDBColor: 0.0: 0.0: 1.0];
	[self setLCDFColor: 0.8: 1.0: 0.8];

	[self setLCDColumns:40];
	[self setLCDLines:2];

	[self setLCDCharWidth:6];
	[self setLCDCharHeight:8];

	[self setLCDOffsetColumns:0];
	[self setLCDOffsetLines:4];

	[self setLCDPixelX:2];
	[self setLCDPixelY:2];

	[self setLCDBorderX:4];
	[self setLCDBorderY:8];

	[self LCDClear]; // will also reset cursor position
	
	[self LCDPrintString:"READY."];
	NSLog(@"LCD Initialized");

	return self;
}


//////////////////////////////////////////////////////////////////////////////
// LCD access functions
//////////////////////////////////////////////////////////////////////////////
- (void)LCDClear
{
	unsigned line, column;
	
	// clear screen
	for(line=0; line<LCD_MAX_LINES; ++line)
		for(column=0; column<LCD_MAX_COLUMNS; ++column)
			LCDScreen[line][column] = ' ';

	// reset cursor
	[self setLCDCursorX:0];
	[self setLCDCursorY:0];
	
	// request display update
	[self setNeedsDisplay:YES];
}

- (void)LCDPrintChar: (char)c
{
	// copy char into array
	LCDScreen[LCDCursorY][LCDCursorX] = c;
	if( ++LCDCursorX >= LCD_MAX_COLUMNS )
		LCDCursorX = LCD_MAX_COLUMNS-1;
	
	// request display update
	[self setNeedsDisplay:YES];
}

- (void)LCDPrintString: (char *)str
{
	// send the string to PrintChar function
	while( *str != '\0' ) {
		[self LCDPrintChar:*str];
		++str;
		if( LCDCursorX == (LCD_MAX_COLUMNS-1) )
			break;
	};

}

- (void)LCDSpecialCharInit: (unsigned char)num: (unsigned char *)table
{
	int i;
	
	for(i=0; i<8; ++i)
		charset[num][i] = table[i];
}

- (void)LCDSpecialCharsInit: (unsigned char *)table
{
	int i, j;
	
	for(i=0; i<8; ++i)
		for(j=0; j<8; ++j)
			charset[i][j] = table[i*8+j];
}


//////////////////////////////////////////////////////////////////////////////
// Get/Set functions
//////////////////////////////////////////////////////////////////////////////
- (void)setLCDBColor: (float)r: (float)g: (float)b
{
	LCDBColorR=r;
	LCDBColorG=g;
	LCDBColorB=b;
}

- (void)setLCDFColor: (float)r: (float)g: (float)b
{
	LCDFColorR=r;
	LCDFColorG=g;
	LCDFColorB=b;
}

- (unsigned)getLCDColumns { return LCDColumns; }
- (void)setLCDColumns: (unsigned)num { LCDColumns = num; }
- (unsigned)getLCDLines { return LCDLines; }
- (void)setLCDLines: (unsigned)num { LCDLines = num; }

- (unsigned)getLCDCharWidth { return LCDCharWidth; }
- (void)setLCDCharWidth: (unsigned)num { LCDCharWidth = num; }
- (unsigned)getLCDCharHeight { return LCDCharHeight; }
- (void)setLCDCharHeight: (unsigned)num { LCDCharHeight = num; }

- (unsigned)getLCDOffsetColumns { return LCDOffsetColumns; }
- (void)setLCDOffsetColumns: (unsigned)num { LCDOffsetColumns = num; }
- (unsigned)getLCDOffsetLines { return LCDOffsetLines; }
- (void)setLCDOffsetLines: (unsigned)num { LCDOffsetLines = num; }

- (unsigned)getLCDPixelX { return LCDPixelX; }
- (void)setLCDPixelX: (unsigned)num { LCDPixelX = num; }
- (unsigned)getLCDPixelY { return LCDPixelY; }
- (void)setLCDPixelY: (unsigned)num { LCDPixelY = num; }

- (unsigned)getLCDBorderX { return LCDBorderX; }
- (void)setLCDBorderX: (unsigned)num { LCDBorderX = num; }
- (unsigned)getLCDBorderY { return LCDBorderY; }
- (void)setLCDBorderY: (unsigned)num { LCDBorderY = num; }

- (unsigned)getLCDCursorX { return LCDCursorX; }
- (void)setLCDCursorX: (unsigned)num { LCDCursorX = num; }
- (unsigned)getLCDCursorY { return LCDCursorY; }
- (void)setLCDCursorY: (unsigned)num { LCDCursorY = num; }


//////////////////////////////////////////////////////////////////////////////
// (Re-)draw view
//////////////////////////////////////////////////////////////////////////////  
- (void)drawRect:(NSRect)rect {		
	

	// help variables
	unsigned column, line, x, y;

	// calculate X/Y dimensions of LCD
	unsigned lcd_size_x = 2*LCDBorderX + LCDPixelX*LCDColumns*LCDCharWidth + LCDOffsetColumns*(LCDColumns-1) + 2;
	unsigned lcd_size_y = 2*LCDBorderY + LCDPixelY*LCDLines*LCDCharHeight + LCDOffsetLines*(LCDLines-1);

	
	// for debugging (values can be used to re-size the NSView object
#if 0
	NSLog(@"X: %d  Y: %d\n", lcd_size_x, lcd_size_y);
#endif

	NSBezierPath *BP = [NSBezierPath bezierPathWithRect: [self bounds]];
	[[NSColor colorWithDeviceRed:LCDBColorR green:LCDBColorG blue:LCDBColorB alpha:1.] set];
	[BP fill];
    BP = [NSBezierPath bezierPathWithRect: NSMakeRect(0, 0,
                                                     lcd_size_x,
                                                     lcd_size_y)];

	// print each character
	// TODO: optimized version, which buffers characters during printChar() to avoid that complete bitmap has to be re-created on each redraw
	for(line=0; line<LCDLines; ++line) {
		for(column=0; column<LCDColumns; ++column) {
			unsigned char *chr = charset[LCDScreen[line][column]];
	
			for(y=0; y<LCDCharHeight; ++y) {
				for(x=0; x<LCDCharWidth; ++x) {
					//NSBezierPath *BP2 = [NSBezierPath bezierPathWithRect: [self bounds]];

					if( chr[y] & (1 << (LCDCharWidth-x-1)) ) {
						[[NSColor colorWithDeviceRed:LCDFColorR green:LCDFColorG blue:LCDFColorB alpha:1.] set];
					} else {
						[[NSColor colorWithDeviceRed:LCDBColorR green:LCDBColorG blue:LCDBColorB alpha:1.] set];
					}
					[BP fill];
					BP = [NSBezierPath bezierPathWithRect: NSMakeRect(
						LCDBorderX + LCDPixelX*(column*LCDCharWidth+x) + column*LCDOffsetColumns,
						lcd_size_y - LCDBorderY - LCDPixelY*(y+line*(LCDCharHeight+LCDOffsetLines)), 
						LCDPixelX, LCDPixelY)]; 
				}
			}
		}
	}
	
}

@end
