// $Id: CLCDView.h 41 2008-09-30 17:20:12Z tk $
//
//
//  Created by Thorsten Klose on 28.09.08.
//  Ported to C++ by Phil Taylor 27/01/10
//

#ifndef _CLCDVIEW_H
#define _CLCDVIEW_H

// increase if required
#define LCD_MAX_COLUMNS 80
#define LCD_MAX_LINES    8

class CLCDView : public Component
{
public:
    CLCDView (unsigned originx, unsigned originy);
  
    /** Destructor. */
    ~CLCDView();

// LCD Access functions
	void LCDClear(void);
	void LCDPrintChar (unsigned char c);
	void LCDPrintString (const char *str);
	void LCDSpecialCharInit (unsigned char num, unsigned char * table);
    void LCDSpecialCharsInit (unsigned char * table);

	void setLCDBColor (unsigned char r, unsigned char g, unsigned char b);
    void setLCDFColor (unsigned char r, unsigned char g, unsigned char b);
	
	
	int getLCDColumns(void) { return LCDColumns; }
	void setLCDColumns (int num) { LCDColumns = num; }
	int getLCDLines(void) { return LCDLines; }
	void setLCDLines (int num) { LCDLines = num; }

	int getLCDCharWidth (void) { return LCDCharWidth; }
	void setLCDCharWidth (int num) { LCDCharWidth = num; }
	int getLCDCharHeight (void) { return LCDCharHeight; }
	void setLCDCharHeight (int num) { LCDCharHeight = num; }	

	int getLCDOffsetColumns (void) { return LCDOffsetColumns; }
	void setLCDOffsetColumns (int num) { LCDOffsetColumns = num; }
	int getLCDOffsetLines (void) { return LCDOffsetLines; }
	void setLCDOffsetLines (int num) { LCDOffsetLines = num; }
	
	int getLCDPixelX (void) { return LCDPixelX; }
	void setLCDPixelX (int num) { LCDPixelX = num; }
	int getLCDPixelY (void){ return LCDPixelY; }
	void setLCDPixelY (int num) { LCDPixelY = num; }	

	int getLCDBorder (void) { return LCDBorder; }
	void setLCDBorder (int num) { LCDBorder = num; }

	int getLCDCursorX (void) { return LCDCursorX; }
	void setLCDCursorX (int num) { LCDCursorX = num; }
	int getLCDCursorY (void) { return LCDCursorY; }
	void setLCDCursorY (int num) { LCDCursorY = num; }

	int getLCDOriginX (void) { return LCDOriginX; }
	void setLCDOriginX (int num) { LCDOriginX = num; }
	int getLCDOriginY (void) { return LCDOriginY; }
	void setLCDOriginY (int num) { LCDOriginY = num; }

	int getLCDSizeX(void);
	int getLCDSizeY(void);

	void paint (Graphics& g);


private:

// Various variables for holding LCD contents.
	unsigned char	LCDBColorR, LCDBColorG, LCDBColorB; // background color
	unsigned char	LCDFColorR, LCDFColorG, LCDFColorB; // foreground color

	int	LCDLines;  // number of lines (default 2)
	
	int	LCDCharWidth; // width of a single character (default 6)
	int	LCDCharHeight; // height of a single character (default 8)
	
	int	LCDOffsetColumns; // offset between columns (default 0)
	int	LCDOffsetLines; // offset between lines (default 4)
	
	int	LCDPixelX;	// X dimension of a pixel (default 2)
	int	LCDPixelY;	// Y dimension of a pixel (default 2)

	int 	LCDBorder;	// distance to frame (default 4)
	
	int 	LCDCursorX; // cursor X pos (default 0)
	int 	LCDCursorY; // cursor Y pos (default 0)
	
	int    LCDOriginX; // Origin position (default 16);
	int   LCDOriginY; // Origin position (default 60);
	int	LCDColumns; // number of characters per line (default 40)

	
	unsigned char 	LCDScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];
	bool		NeedsDisplay;
};

#endif /* _CLCDVIEW_H */