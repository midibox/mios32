/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  CLcd.cpp
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#include "CLcd.h"


CLcd::CLcd()
    : specialCharPos(-1)
    , updateRequest(false)
{
	setLcdBColor( 0x00, 0x00, 0xff);
	setLcdFColor( 0xff, 0xff, 0xff);

	setLcdColumns(40);
	setLcdLines(2);

    // init custom charset
    for(int x=0; x<8; ++x)
        for(int y=0; y<8; ++y)
            customCharset[x][y] = 0;

    lcdCmd(0x01); // clear display, will also reset the cursor position
    lcdData('R');
    lcdData('E');
    lcdData('A');
    lcdData('D');
    lcdData('Y');
    lcdData('.');
}


CLcd::~CLcd(void)
{
}


//////////////////////////////////////////////////
// lcdCmd is called from app_lcd.c 
// Currently there is no support for user defined 
// characters but I am working on this!
//////////////////////////////////////////////////
void CLcd::lcdCmd(unsigned char cmd)
{
    if( cmd==0x01 ) {
        // clear screen
        for(int line=0; line<LCD_MAX_LINES; ++line)
            for(int column=0; column<LCD_MAX_COLUMNS; ++column)
                lcdScreen[line][column] = ' ';
        updateRequest = true;

        // reset cursor
        setLcdCursorX(0);
        setLcdCursorY(0);
    } else if( cmd >= 0x40 && cmd < 0x80 ) {
        // User defined character. Next 8xlcdData is the actual character.
        specialCharPos = cmd & 0x3f;
    } else if( cmd >= 0x80 ) {
        // This is to set the cursor position
        specialCharPos = -1; // disable special char preloading
        setLcdCursorY((cmd & 0x40) ? 1 : 0);
        setLcdCursorX(cmd & 0x3f);
    }
}

//////////////////////////////////////////////////
// lcdData is called from app_lcd.c 
//////////////////////////////////////////////////
void CLcd::lcdData(unsigned char data)
{
    if( specialCharPos >= 0 ) {
        if( specialCharPos < 64 ) {
            customCharset[specialCharPos/8][specialCharPos%8] = data;
            ++specialCharPos;
            updateRequest = true;
        }
    } else {
        // copy char into array
        lcdScreen[lcdCursorY][lcdCursorX] = data;
        if( ++lcdCursorX >= LCD_MAX_COLUMNS )
            lcdCursorX = LCD_MAX_COLUMNS-1;
        updateRequest = true;
    }
}


//////////////////////////////////////////////////////////////////////////////
// Get/Set functions
//////////////////////////////////////////////////////////////////////////////
void CLcd::setLcdBColor (unsigned char r, unsigned char g, unsigned char b)
{
	bColorR=r;
	bColorG=g;
	bColorB=b;
}

void CLcd::setLcdFColor (unsigned char r, unsigned char g, unsigned char b)
{
	fColorR=r;
	fColorG=g;
	fColorB=b;
}


//////////////////////////////////////////////////////////////////////////////
// Communication with CLcdView
//////////////////////////////////////////////////////////////////////////////
bool CLcd::updateRequired()
{
    return updateRequest;
}

void CLcd::updateDone()
{
    updateRequest = false;
}
