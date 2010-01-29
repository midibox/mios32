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
	
	
	int CLCDView::getLCDColumns(void) { return LCDColumns; }
	void CLCDView::setLCDColumns (int num) { LCDColumns = num; }
	int CLCDView::getLCDLines(void) { return LCDLines; }
	void CLCDView::setLCDLines (int num) { LCDLines = num; }

	int CLCDView::getLCDCharWidth (void) { return LCDCharWidth; }
	void CLCDView::setLCDCharWidth (int num) { LCDCharWidth = num; }
	int CLCDView::getLCDCharHeight (void) { return LCDCharHeight; }
	void CLCDView::setLCDCharHeight (int num) { LCDCharHeight = num; }	

	int CLCDView::getLCDOffsetColumns (void) { return LCDOffsetColumns; }
	void CLCDView::setLCDOffsetColumns (int num) { LCDOffsetColumns = num; }
	int CLCDView::getLCDOffsetLines (void) { return LCDOffsetLines; }
	void CLCDView::setLCDOffsetLines (int num) { LCDOffsetLines = num; }
	
	int CLCDView::getLCDPixelX (void) { return LCDPixelX; }
	void CLCDView::setLCDPixelX (int num) { LCDPixelX = num; }
	int CLCDView::getLCDPixelY (void){ return LCDPixelY; }
	void CLCDView::setLCDPixelY (int num) { LCDPixelY = num; }	

	int CLCDView::getLCDBorder (void) { return LCDBorder; }
	void CLCDView::setLCDBorder (int num) { LCDBorder = num; }

	int CLCDView::getLCDCursorX (void) { return LCDCursorX; }
	void CLCDView::setLCDCursorX (int num) { LCDCursorX = num; }
	int CLCDView::getLCDCursorY (void) { return LCDCursorY; }
	void CLCDView::setLCDCursorY (int num) { LCDCursorY = num; }

	int CLCDView::getLCDOriginX (void) { return LCDOriginX; }
	void CLCDView::setLCDOriginX (int num) { LCDOriginX = num; }
	int CLCDView::getLCDOriginY (void) { return LCDOriginY; }
	void CLCDView::setLCDOriginY (int num) { LCDOriginY = num; }

	int getLCDSizeX(void);
	int getLCDSizeY(void);

	void paint (Graphics& g);

private:

// Various variables for holding LCD contents.
	unsigned char	LCDBColorR, LCDBColorG, LCDBColorB; // background color
	unsigned char	LCDFColorR, LCDFColorG, LCDFColorB; // foreground color

	int	LCDColumns; // number of characters per line (default 40)
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
	
	
	unsigned char 	LCDScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];
	bool		NeedsDisplay;
};

#endif /* _CLCDVIEW_H */