// $Id: CLCDView.h 41 2008-09-30 17:20:12Z tk $
//
//
//  Created by Thorsten Klose on 28.09.08.
//  Ported to C++ by Phil Taylor 27/01/10
//


// increase if required
#define LCD_MAX_COLUMNS 80
#define LCD_MAX_LINES    8

class CLCDView
{
public:
    CLCDView (int originx, int originy);
  
    /** Destructor. */
    ~CLCDView();

// LCD Access functions
	void LCDClear(void);
	void LCDPrintChar (unsigned char c);
	void LCDPrintString (const char *str);
	void LCDSpecialCharInit (unsigned char num, unsigned char * table);
    void LCDSpecialCharsInit (unsigned char * table);

	void setLCDBColor (float r, float g, float b);
    void setLCDFColor (float r, float g, float b);
	unsigned char getLCDColumns(void);
	void setLCDColumns (unsigned char num);
    unsigned char getLCDLines(void);
    void setLCDLines (unsigned char num);
	unsigned char getLCDCharWidth(void);
	void setLCDCharWidth (unsigned char num);
	unsigned char getLCDCharHeight(void);
	void setLCDCharHeight (unsigned char num);
	unsigned char getLCDOffsetColumns(void);
	void setLCDOffsetColumns(unsigned char num);
	unsigned char getLCDOffsetLines();
	void setLCDOffsetLines (unsigned char num);
	unsigned char getLCDPixelX(void);
	void setLCDPixelX (unsigned char num);
	unsigned char getLCDPixelY(void);
	void setLCDPixelY(unsigned char num);
	unsigned char getLCDBorderX(void);
	void setLCDBorderX (unsigned char num);
	unsigned char getLCDBorderY(void);
	void setLCDBorderY (unsigned char num);
	unsigned char getLCDCursorX(void);
	void setLCDCursorX (unsigned char num);
	unsigned char getLCDCursorY(void);
	void setLCDCursorY (unsigned char num);
	unsigned char getLCDOriginX(void);
	void setLCDOriginX (unsigned char num);
	unsigned char getLCDOriginY(void);
	void setLCDOriginY (unsigned char num);
	void drawRect (Graphics& g);

private:

// Various variables for holding LCD contents.
	unsigned char		LCDBColorR, LCDBColorG, LCDBColorB; // background color
	unsigned char		LCDFColorR, LCDFColorG, LCDFColorB; // foreground color

	unsigned char	LCDColumns; // number of characters per line (default 40)
	unsigned char	LCDLines;  // number of lines (default 2)
	
	unsigned char	LCDCharWidth; // width of a single character (default 6)
	unsigned char	LCDCharHeight; // height of a single character (default 8)
	
	unsigned char	LCDOffsetColumns; // offset between columns (default 0)
	unsigned char	LCDOffsetLines; // offset between lines (default 4)
	
	unsigned char	LCDPixelX;	// X dimension of a pixel (default 2)
	unsigned char	LCDPixelY;	// Y dimension of a pixel (default 2)

	unsigned char	LCDBorderX;	// left/right distance to frame (default 4)
	unsigned char	LCDBorderY;	// upper/lower distance to frame (default 8)
	
	unsigned char	LCDCursorX; // cursor X pos (default 0)
	unsigned char	LCDCursorY; // cursor Y pos (default 0)
	
	unsigned char   LCDOriginX; // Origin position (default 16);
	unsigned char   LCDOriginY; // Origin position (default 60);
	
	
	unsigned char		LCDScreen[LCD_MAX_LINES][LCD_MAX_COLUMNS];
	bool		NeedsDisplay;
};