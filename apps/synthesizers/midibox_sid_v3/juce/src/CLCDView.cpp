// $Id: CLCDView.cpp 261 2009-01-09 00:08:48Z tk $
//
//  CLCDView.cpp
//  
//
//  Created by Thorsten Klose on 28.09.08.
//  Ported to C++ by Phil Taylor 27/01/10
//  Copyright 2008-2010
//
#include "includes.h"

#include "CLCDView.h"
#include "EditorComponent.h"
#include "AudioProcessing.h"

#include "mios32.h"
// include default character set from separate file
#include "CLCDView_charset.inc"

//////////////////////////////////////////////////////////////////////////////
// Frame initialisation
//////////////////////////////////////////////////////////////////////////////

CLCDView::CLCDView(unsigned originx,unsigned originy)
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

	setLCDBorder(4);

	setLCDOriginX(originx);
	setLCDOriginY(originy);

	
	setSize (getLCDSizeX(), getLCDSizeY());
	setTopLeftPosition(originx,originy);
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
	repaint();
}

void CLCDView::LCDPrintChar (unsigned char c)
{
	// copy char into array
	LCDScreen[LCDCursorY][LCDCursorX] = c;
	if( ++LCDCursorX >= LCD_MAX_COLUMNS )
		LCDCursorX = LCD_MAX_COLUMNS-1;
	
	// request display update
	repaint();
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
void CLCDView::setLCDBColor (unsigned char r, unsigned char g, unsigned char b)
{
	LCDBColorR=r;
	LCDBColorG=g;
	LCDBColorB=b;
}

void CLCDView::setLCDFColor (unsigned char r, unsigned char g, unsigned char b)
{
	LCDFColorR=r;
	LCDFColorG=g;
	LCDFColorB=b;
}


int CLCDView::getLCDSizeX (void) 
{ 
	return (2*LCDBorder) + (LCDPixelX*LCDColumns*LCDCharWidth) + (LCDOffsetColumns*(LCDColumns-1)) + 2;
}

int CLCDView::getLCDSizeY (void) 
{ 	
	return (2*LCDBorder) + (LCDPixelY*LCDLines*LCDCharHeight) + (LCDOffsetLines*(LCDLines-1)); 
}


//////////////////////////////////////////////////////////////////////////////
// (Re-)draw view
//////////////////////////////////////////////////////////////////////////////  
void CLCDView::paint(Graphics& g) 
{		
	// help variables
	int column, line, x, y;

	g.setColour (Colour(LCDBColorR,LCDBColorG,LCDBColorB));
    g.fillRect (0,0, getLCDSizeX(), getLCDSizeY());


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
						LCDBorder + LCDPixelX*(column*LCDCharWidth+x) + column*LCDOffsetColumns,
						LCDBorder + LCDPixelY*(y+line*(LCDCharHeight+LCDOffsetLines)), 
						LCDPixelX, LCDPixelY);
				}
			}
		}
	}
}

