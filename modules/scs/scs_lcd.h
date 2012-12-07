// $Id$
/*
 * LCD Access functions for Standard Control Surface
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SCS_LCD_H
#define _SCS_LCD_H

#include <scs.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// can be overruled in mios32_config.h
#ifndef SCS_LCD_NUM_DEVICES
# define SCS_LCD_NUM_DEVICES          1
#endif

#ifndef SCS_LCD_COLUMNS_PER_DEVICE
# define SCS_LCD_COLUMNS_PER_DEVICE  (SCS_NUM_MENU_ITEMS*SCS_MENU_ITEM_WIDTH)
#endif

#ifndef SCS_LCD_MAX_LINES
# define SCS_LCD_MAX_LINES    2
#endif


// shouldn't be overruled
#define SCS_LCD_MAX_COLUMNS  (SCS_LCD_NUM_DEVICES*SCS_LCD_COLUMNS_PER_DEVICE)


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SCS_LCD_CHARSET_None,
  SCS_LCD_CHARSET_Menu,
  SCS_LCD_CHARSET_VBars,
  SCS_LCD_CHARSET_HBars,
} scs_lcd_charset_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SCS_LCD_Init(u32 mode);
extern s32 SCS_LCD_Clear(void);
extern s32 SCS_LCD_PrintChar(char c);
extern s32 SCS_LCD_BufferSet(u16 x, u16 y, char *str);
extern s32 SCS_LCD_CursorSet(u16 column, u16 line);
extern s32 SCS_LCD_Update(u8 force);
extern s32 SCS_LCD_InitSpecialChars(scs_lcd_charset_t charset, u8 force);
extern s32 SCS_LCD_SpecialCharsReInit(void);
extern s32 SCS_LCD_PrintString(char *str);
extern s32 SCS_LCD_PrintFormattedString(char *format, ...);
extern s32 SCS_LCD_PrintSpaces(int num);
extern s32 SCS_LCD_PrintStringPadded(char *str, u32 width);
extern s32 SCS_LCD_PrintStringCentered(char *str, u32 width);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SCS_H */
