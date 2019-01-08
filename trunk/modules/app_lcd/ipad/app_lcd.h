/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
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

// C++ check required, since app_lcd.h is also included by mios32.h for C-only code
#ifdef __cplusplus
#include "CLcd.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// only used by GLCDs
// Use dummy values here - define a bitmap as it would be displayed with special chars
// TODO: we could print a 8*5x8 bitmap like done in apps/sequencers/midibox_seq_v4/core/seq_lcd_logo.c
#define APP_LCD_NUM_X 1
#define APP_LCD_WIDTH (4*5)
#define APP_LCD_NUM_Y 1
#define APP_LCD_HEIGHT (2*8)
#define APP_LCD_COLOUR_DEPTH 1
#define APP_LCD_BITMAP_SIZE ((APP_LCD_NUM_X*APP_LCD_WIDTH * APP_LCD_NUM_Y*APP_LCD_HEIGHT * APP_LCD_COLOUR_DEPTH) / 8)


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif


// C++ check required, since app_lcd.h is also included by mios32.h for C-only code
#ifdef __cplusplus
extern CLcd *APP_LCD_GetComponentPtr(u8 device);
#endif


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _APP_LCD_H */
