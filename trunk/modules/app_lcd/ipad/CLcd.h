/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
//
//  CLcd.h
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _CLCD_H
#define _CLCD_H


// don't change these constants - they are matching with the maximum
// amount of accessible char RAM (128 bytes)
#define LCD_MAX_COLUMNS 64
#define LCD_MAX_LINES    2

class CLcd
{
public:
    /** Constructor **/
    CLcd();
  
    /** Destructor **/
    ~CLcd();

    // used by app_lcd.cpp
    void lcdData(unsigned char data);
    void lcdCmd(unsigned char cmd);

    // used by CLcdView
    bool updateRequired();
    void updateDone();

	void setLcdBColor(unsigned char r, unsigned char g, unsigned char b);
    void setLcdFColor(unsigned char r, unsigned char g, unsigned char b);
	
	
	int getLcdColumns(void) { return lcdColumns; }
	void setLcdColumns (int num) { lcdColumns = num; }
	int getLcdLines(void) { return lcdLines; }
	void setLcdLines (int num) { lcdLines = num; }

	int getLcdCursorX (void) { return lcdCursorX; }
	void setLcdCursorX (int num) { lcdCursorX = num; }
	int getLcdCursorY (void) { return lcdCursorY; }
	void setLcdCursorY (int num) { lcdCursorY = num; }

	unsigned char lcdScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];

	unsigned char	bColorR, bColorG, bColorB; // background color
	unsigned char	fColorR, fColorG, fColorB; // foreground color

    unsigned char customCharset[8][8];

    // Various variables for holding LCD contents.
	int	lcdLines;  // number of lines (default 2)
	int	lcdColumns; // number of characters per line (default 40)

private:
    int specialCharPos;

    bool updateRequest;

	int lcdCursorX; // cursor X pos (default 0)
	int lcdCursorY; // cursor Y pos (default 0)

};

#endif /* _CLCD_H */
