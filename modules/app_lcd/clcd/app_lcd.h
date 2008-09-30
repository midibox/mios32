// $Id$
/*
 * Header file for application specific LCD Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _APP_LCD_H
#define _APP_LCD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// hooks to MIOS32_LCD
extern s32 APP_LCD_Init(u32 mode);
extern s32 APP_LCD_Data(u8 data);
extern s32 APP_LCD_Cmd(u8 cmd);
extern s32 APP_LCD_Clear(void);
extern s32 APP_LCD_CursorSet(u16 line, u16 column);
extern s32 APP_LCD_PrintChar(char c);
extern s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8]);
extern s32 APP_LCD_FontInit(u8 *font);
extern s32 APP_LCD_BColourSet(u8 r, u8 g, u8 b);
extern s32 APP_LCD_FColourSet(u8 r, u8 g, u8 b);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _APP_LCD_H */
