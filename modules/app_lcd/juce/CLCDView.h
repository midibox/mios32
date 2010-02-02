/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: CLCDView.h 41 2008-09-30 17:20:12Z tk $
//
//
//  Created by Thorsten Klose on 28.09.08.
//  Ported to C++ by Phil Taylor 27/01/10
//

#ifndef _CLCDVIEW_H
#define _CLCDVIEW_H

// expecting juce installation at the same directory level like mios32
#include "../../../../../juce/juce_amalgamated.h"


// increase if required in your mios32_config.h file
#ifndef LCD_MAX_COLUMNS
#define LCD_MAX_COLUMNS 80
#endif

#ifndef LCD_MAX_LINES
#define LCD_MAX_LINES    8
#endif

class CLcdView : public Component
{
public:
    /** Constructor **/
    CLcdView (unsigned originx, unsigned originy);
  
    /** Destructor **/
    ~CLcdView();

    // used by app_lcd.c
    int lcdData(unsigned char data);
    int lcdCmd(unsigned char cmd);

    // lcd Access functions
	void lcdClear(void);
	void lcdPrintChar (unsigned char c);
	void lcdPrintString (const char *str);
	void lcdSpecialCharInit (unsigned char num, unsigned char * table);
    void lcdSpecialCharsInit (unsigned char * table);

	void setLcdBColor (unsigned char r, unsigned char g, unsigned char b);
    void setLcdFColor (unsigned char r, unsigned char g, unsigned char b);
	
	
	int getLcdColumns(void) { return lcdColumns; }
	void setLcdColumns (int num) { lcdColumns = num; }
	int getLcdLines(void) { return lcdLines; }
	void setLcdLines (int num) { lcdLines = num; }

	int getLcdCharWidth (void) { return lcdCharWidth; }
	void setLcdCharWidth (int num) { lcdCharWidth = num; }
	int getLcdCharHeight (void) { return lcdCharHeight; }
	void setLcdCharHeight (int num) { lcdCharHeight = num; }	

	int getLcdOffsetColumns (void) { return lcdOffsetColumns; }
	void setLcdOffsetColumns (int num) { lcdOffsetColumns = num; }
	int getLcdOffsetLines (void) { return lcdOffsetLines; }
	void setLcdOffsetLines (int num) { lcdOffsetLines = num; }
	
	int getLcdPixelX (void) { return lcdPixelX; }
	void setLcdPixelX (int num) { lcdPixelX = num; }
	int getLcdPixelY (void){ return lcdPixelY; }
	void setLcdPixelY (int num) { lcdPixelY = num; }	

	int getLcdBorder (void) { return lcdBorder; }
	void setLcdBorder (int num) { lcdBorder = num; }

	int getLcdCursorX (void) { return lcdCursorX; }
	void setLcdCursorX (int num) { lcdCursorX = num; }
	int getLcdCursorY (void) { return lcdCursorY; }
	void setLcdCursorY (int num) { lcdCursorY = num; }

	int getLcdOriginX (void) { return lcdOriginX; }
	void setLcdOriginX (int num) { lcdOriginX = num; }
	int getLcdOriginY (void) { return lcdOriginY; }
	void setLcdOriginY (int num) { lcdOriginY = num; }

	int getLcdSizeX(void);
	int getLcdSizeY(void);

	void paint (Graphics& g);


private:

    // Various variables for holding LCD contents.
	unsigned char	lcdBColorR, lcdBColorG, lcdBColorB; // background color
	unsigned char	lcdFColorR, lcdFColorG, lcdFColorB; // foreground color

	int	lcdLines;  // number of lines (default 2)
	
	int	lcdCharWidth; // width of a single character (default 6)
	int	lcdCharHeight; // height of a single character (default 8)
	
	int	lcdOffsetColumns; // offset between columns (default 0)
	int	lcdOffsetLines; // offset between lines (default 4)
	
	int	lcdPixelX;	// X dimension of a pixel (default 2)
	int	lcdPixelY;	// Y dimension of a pixel (default 2)

	int lcdBorder;	// distance to frame (default 4)
	
	int lcdCursorX; // cursor X pos (default 0)
	int lcdCursorY; // cursor Y pos (default 0)
	
	int lcdOriginX; // Origin position (default 16);
	int lcdOriginY; // Origin position (default 60);
	int	lcdColumns; // number of characters per line (default 40)

	unsigned char 	lcdScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];
	bool		NeedsDisplay;
};

#endif /* _CLCDVIEW_H */
