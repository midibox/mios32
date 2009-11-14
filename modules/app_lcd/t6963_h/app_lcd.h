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

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef APP_LCD_OUTPUT_MODE
#define APP_LCD_OUTPUT_MODE  1
#endif

// don't change these values for this GLCD type
#define APP_LCD_NUM_X 1
#define APP_LCD_WIDTH 240
#define APP_LCD_NUM_Y 1
#define APP_LCD_HEIGHT 64
#define APP_LCD_COLOUR_DEPTH 1
#define APP_LCD_BITMAP_SIZE ((APP_LCD_NUM_X*APP_LCD_WIDTH * APP_LCD_NUM_Y*APP_LCD_HEIGHT * APP_LCD_COLOUR_DEPTH) / 8)


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
extern s32 APP_LCD_CursorSet(u16 column, u16 line);
extern s32 APP_LCD_GCursorSet(u16 x, u16 y);
extern s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8]);
extern s32 APP_LCD_BColourSet(u32 rgb);
extern s32 APP_LCD_FColourSet(u32 rgb);
extern s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour);
extern s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _APP_LCD_H */
