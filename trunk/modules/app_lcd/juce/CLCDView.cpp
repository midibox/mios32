/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  CLCDView.cpp
//  
//
//  Created by Thorsten Klose on 28.09.08.
//  Ported to C++ by Phil Taylor 27/01/10
//  Copyright 2008-2010
//

#include "CLCDView.h"
// include default character set from separate file
#include "CLCDView_charset.inc"

// Allow object to be accessed from "C" functions


//////////////////////////////////////////////////////////////////////////////
// Frame initialisation
///////////////////////////////


///////////////////////////////////////////////

CLcdView::CLcdView(unsigned originx, unsigned originy)
{
	setLcdBColor( 0x00, 0x00, 0xff);
	setLcdFColor( 0xff, 0xff, 0xff);

	setLcdColumns(20);
	setLcdLines(2);

	setLcdCharWidth(6);
	setLcdCharHeight(8);

	setLcdOffsetColumns(0);
	setLcdOffsetLines(4);

	setLcdPixelX(2);
	setLcdPixelY(2);

	setLcdBorder(4);

	setLcdOriginX(originx);
	setLcdOriginY(originy);

	
	setSize (getLcdSizeX(), getLcdSizeY());
	setTopLeftPosition(originx,originy);
	lcdClear(); // will also reset cursor position
	
	lcdPrintString("READY.");
	
	
	return;
}


CLcdView::~CLcdView(void)
{
	
}


//////////////////////////////////////////////////
// lcdCmd is called from app_lcd.c 
// Currently there is no support for user defined 
// characters but I am working on this!
//////////////////////////////////////////////////
int CLcdView::lcdCmd(unsigned char cmd)
{
	if (isEnabled())
	{
		if (cmd==0x01) {
			lcdClear();
		}
		else if (cmd >= 0x40 && cmd < 0x80) {
			// User defined character. Next 8xlcdData is the actual character.

		}
		else if (cmd >= 0x80) {
			// This is to set the cursor position
			// Need to come up with a more portable solution as this
			// Is fixed for a single display.
			cmd=cmd&~0x80; // AND with ~0x80 to remove cmd bit. 
			if (cmd & 0x40) {
				setLcdCursorY(1);
				setLcdCursorX(cmd&~0x40);
			} else {
				setLcdCursorY(0);
				setLcdCursorX(cmd);
			}
		}
        return 0;
	}
    return -1;
}

//////////////////////////////////////////////////
// lcdData is called from app_lcd.c 
//////////////////////////////////////////////////
int CLcdView::lcdData(unsigned char data)
{
	// Check that the component is valid to make sure that it has been
	// initialized
	if (isEnabled())
	{
		lcdPrintChar(data);
		return 0;
	}
	return -1;
}



//////////////////////////////////////////////////////////////////////////////
// LCD access functions
//////////////////////////////////////////////////////////////////////////////
void CLcdView::lcdClear(void)
{
	unsigned char line, column;
	
	// clear screen
	for(line=0; line<LCD_MAX_LINES; ++line)
		for(column=0; column<LCD_MAX_COLUMNS; ++column)
			lcdScreen[line][column] = ' ';

	// reset cursor
	setLcdCursorX(0);
	setLcdCursorY(0);
	
	// request display update
	repaint();
}

void CLcdView::lcdPrintChar (unsigned char c)
{
	// copy char into array
	lcdScreen[lcdCursorY][lcdCursorX] = c;
	if( ++lcdCursorX >= LCD_MAX_COLUMNS )
		lcdCursorX = LCD_MAX_COLUMNS-1;
	
	// request display update
	repaint();
}

void CLcdView::lcdPrintString (const char *str)
{
	// send the string to PrintChar function
	while( *str != '\0' ) {
		lcdPrintChar(*str);
		++str;
		if( lcdCursorX == (LCD_MAX_COLUMNS-1) )
			break;
	};

}

void CLcdView::lcdSpecialCharInit (unsigned char num, unsigned char *table)
{
	for(int i=0; i<8; ++i)
		charset[num][i] = table[i];
}

void CLcdView::lcdSpecialCharsInit (unsigned char *table)
{
	for(int i=0; i<8; ++i)
		for(int j=0; j<8; ++j)
			charset[i][j] = table[i*8+j];
}


//////////////////////////////////////////////////////////////////////////////
// Get/Set functions
//////////////////////////////////////////////////////////////////////////////
void CLcdView::setLcdBColor (unsigned char r, unsigned char g, unsigned char b)
{
	lcdBColorR=r;
	lcdBColorG=g;
	lcdBColorB=b;
}

void CLcdView::setLcdFColor (unsigned char r, unsigned char g, unsigned char b)
{
	lcdFColorR=r;
	lcdFColorG=g;
	lcdFColorB=b;
}


int CLcdView::getLcdSizeX (void) 
{ 
	return (2*lcdBorder) + (lcdPixelX*lcdColumns*lcdCharWidth) + (lcdOffsetColumns*(lcdColumns-1)) + 2;
}

int CLcdView::getLcdSizeY (void) 
{ 	
	return (2*lcdBorder) + (lcdPixelY*lcdLines*lcdCharHeight) + (lcdOffsetLines*(lcdLines-1)); 
}


//////////////////////////////////////////////////////////////////////////////
// (Re-)draw view
//////////////////////////////////////////////////////////////////////////////  
void CLcdView::paint(Graphics& g) 
{		
	g.setColour (Colour(lcdBColorR,lcdBColorG,lcdBColorB));
    g.fillRect (0,0, getLcdSizeX(), getLcdSizeY());


	// print each character
	// TODO: optimized version, which buffers characters during printChar() to avoid that complete bitmap has to be re-created on each redraw
	for(int line=0; line<lcdLines; ++line) {
		for(int column=0; column<lcdColumns; ++column) {
			unsigned char *chr = charset[lcdScreen[line][column]];
	
			for(int y=0; y<lcdCharHeight; ++y) {
				for(int x=0; x<lcdCharWidth; ++x) {

					if( chr[y] & (1 << (lcdCharWidth-x-1)) ) {
						g.setColour (Colour(lcdFColorR,lcdFColorG,lcdFColorB));
					} else {
						g.setColour (Colour(lcdBColorR,lcdBColorG,lcdBColorB));
					}
					g.drawRect(lcdBorder + lcdPixelX*(column*lcdCharWidth+x) + column*lcdOffsetColumns,
                               lcdBorder + lcdPixelY*(y+line*(lcdCharHeight+lcdOffsetLines)), 
                               lcdPixelX, lcdPixelY);
				}
			}
		}
	}
}

