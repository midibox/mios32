// $Id: CLCDView.m 261 2009-01-09 00:08:48Z tk $
//
//  CLCDView.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include "includes.h"

#include "CLCDView.h"
#include "EditorComponent.h"


// include default character set from separate file
#include "CLCDView_charset.inc"



//////////////////////////////////////////////////////////////////////////////
// Frame initialisation
//////////////////////////////////////////////////////////////////////////////

CLCDView::CLCDView(int originx,int originy)
{
	//self = [super initWithFrame:frameRect];

	setLCDBColor( 0x00, 0x00, 0xff);
	setLCDFColor( 0xff, 0xff, 0xff);

	setLCDColumns(20);
	setLCDLines(2);

	setLCDCharWidth(6);
	setLCDCharHeight(8);

	setLCDOffsetColumns(0);
	setLCDOffsetLines(4);

	setLCDPixelX(2);
	setLCDPixelY(2);

	setLCDBorderX(4);
	setLCDBorderY(8);

	setLCDOriginX(originx);
	setLCDOriginY(originy);

	LCDClear(); // will also reset cursor position
	
	LCDPrintString("READY.");

	return;
}


CLCDView::~CLCDView(void)
{
	
}


//////////////////////////////////////////////////////////////////////////////
// LCD access functions
//////////////////////////////////////////////////////////////////////////////
void CLCDView::LCDClear(void)
{
	unsigned char line, column;
	
	// clear screen
	for(line=0; line<LCD_MAX_LINES; ++line)
		for(column=0; column<LCD_MAX_COLUMNS; ++column)
			LCDScreen[line][column] = ' ';

	// reset cursor
	setLCDCursorX(0);
	setLCDCursorY(0);
	
	// request display update
	NeedsDisplay=true;
}

void CLCDView::LCDPrintChar (unsigned char c)
{
	// copy char into array
	LCDScreen[LCDCursorY][LCDCursorX] = c;
	if( ++LCDCursorX >= LCD_MAX_COLUMNS )
		LCDCursorX = LCD_MAX_COLUMNS-1;
	
	// request display update
	NeedsDisplay=true;
}

void CLCDView::LCDPrintString (const char *str)
{
	// send the string to PrintChar function
	while( *str != '\0' ) {
		LCDPrintChar(*str);
		++str;
		if( LCDCursorX == (LCD_MAX_COLUMNS-1) )
			break;
	};

}

void CLCDView::LCDSpecialCharInit (unsigned char num, unsigned char *table)
{
	int i;
	
	for(i=0; i<8; ++i)
		charset[num][i] = table[i];
}

void CLCDView::LCDSpecialCharsInit (unsigned char *table)
{
	int i, j;
	
	for(i=0; i<8; ++i)
		for(j=0; j<8; ++j)
			charset[i][j] = table[i*8+j];
}


//////////////////////////////////////////////////////////////////////////////
// Get/Set functions
//////////////////////////////////////////////////////////////////////////////
void CLCDView::setLCDBColor (float r, float g, float b)
{
	LCDBColorR=r;
	LCDBColorG=g;
	LCDBColorB=b;
}

void CLCDView::setLCDFColor (float r, float g, float b)
{
	LCDFColorR=r;
	LCDFColorG=g;
	LCDFColorB=b;
}

unsigned char CLCDView::getLCDColumns(void) { return LCDColumns; }
void CLCDView::setLCDColumns (unsigned char num) { LCDColumns = num; }
unsigned char CLCDView::getLCDLines(void) { return LCDLines; }
void CLCDView::setLCDLines (unsigned char num) { LCDLines = num; }

unsigned char CLCDView::getLCDCharWidth (void) { return LCDCharWidth; }
void CLCDView::setLCDCharWidth (unsigned char num) { LCDCharWidth = num; }
unsigned char CLCDView::getLCDCharHeight (void) { return LCDCharHeight; }
void CLCDView::setLCDCharHeight (unsigned char num) { LCDCharHeight = num; }

unsigned char CLCDView::getLCDOffsetColumns (void) { return LCDOffsetColumns; }
void CLCDView::setLCDOffsetColumns (unsigned char num) { LCDOffsetColumns = num; }
unsigned char CLCDView::getLCDOffsetLines (void) { return LCDOffsetLines; }
void CLCDView::setLCDOffsetLines (unsigned char num) { LCDOffsetLines = num; }

unsigned char CLCDView::getLCDPixelX (void) { return LCDPixelX; }
void CLCDView::setLCDPixelX (unsigned char num) { LCDPixelX = num; }
unsigned char CLCDView::getLCDPixelY (void){ return LCDPixelY; }
void CLCDView::setLCDPixelY (unsigned char num) { LCDPixelY = num; }

unsigned char CLCDView::getLCDBorderX (void) { return LCDBorderX; }
void CLCDView::setLCDBorderX (unsigned char num) { LCDBorderX = num; }
unsigned char CLCDView::getLCDBorderY (void) { return LCDBorderY; }
void CLCDView::setLCDBorderY (unsigned char num) { LCDBorderY = num; }

unsigned char CLCDView::getLCDCursorX (void) { return LCDCursorX; }
void CLCDView::setLCDCursorX (unsigned char num) { LCDCursorX = num; }
unsigned char CLCDView::getLCDCursorY (void) { return LCDCursorY; }
void CLCDView::setLCDCursorY (unsigned char num) { LCDCursorY = num; }

unsigned char CLCDView::getLCDOriginX (void) { return LCDOriginX; }
void CLCDView::setLCDOriginX (unsigned char num) { LCDOriginX = num; }
unsigned char CLCDView::getLCDOriginY (void) { return LCDOriginY; }
void CLCDView::setLCDOriginY (unsigned char num) { LCDOriginY = num; }


//////////////////////////////////////////////////////////////////////////////
// (Re-)draw view
//////////////////////////////////////////////////////////////////////////////  
void CLCDView::paint(Graphics& g) 
{		
	
	// help variables
	unsigned column, line, x, y;

	// calculate X/Y dimensions of LCD
	unsigned lcd_size_x = 2*LCDBorderX + LCDPixelX*LCDColumns*LCDCharWidth + LCDOffsetColumns*(LCDColumns-1) + 2;
	unsigned lcd_size_y = 2*LCDBorderY + LCDPixelY*LCDLines*LCDCharHeight + LCDOffsetLines*(LCDLines-1);
	
	g.setColour (Colour(LCDBColorR,LCDBColorG,LCDBColorB));
    g.fillRect (LCDOriginX, LCDOriginY, lcd_size_x, lcd_size_y);

    g.setColour (Colour (0xffabaeb6)); // Grey border
    g.drawRect (LCDOriginX, LCDOriginY, lcd_size_x, lcd_size_y, LCDBorderX);

	// print each character
	// TODO: optimized version, which buffers characters during printChar() to avoid that complete bitmap has to be re-created on each redraw
	for(line=0; line<LCDLines; ++line) {
		for(column=0; column<LCDColumns; ++column) {
			unsigned char *chr = charset[LCDScreen[line][column]];
	
			for(y=0; y<LCDCharHeight; ++y) {
				for(x=0; x<LCDCharWidth; ++x) {

					if( chr[y] & (1 << (LCDCharWidth-x-1)) ) {
						g.setColour (Colour(LCDFColorR,LCDFColorG,LCDFColorB));
					} else {
						g.setColour (Colour(LCDBColorR,LCDBColorG,LCDBColorB));
					}
					g.drawRect(
						LCDOriginX+LCDBorderX + LCDPixelX*(column*LCDCharWidth+x) + column*LCDOffsetColumns,
						LCDOriginY+LCDBorderY + LCDPixelY*(y+line*(LCDCharHeight+LCDOffsetLines)), 
						LCDPixelX, LCDPixelY);
				}
			}
		}
	}
}

