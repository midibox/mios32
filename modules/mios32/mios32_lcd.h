// $Id$
/*
 * Header file for LCD Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_LCD_H
#define _MIOS32_LCD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef MIOS32_LCD_MAX_MAP_LINES
#define MIOS32_LCD_MAX_MAP_LINES 4
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_LCD_TYPE_CLCD  0
#define MIOS32_LCD_TYPE_GLCD  1


/////////////////////////////////////////////////////////////////////////////
// Aliases to app_lcd functions
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_LCD_Data(data) APP_LCD_Data(data)
#define MIOS32_LCD_Cmd(cmd) APP_LCD_Cmd(cmd)
#define MIOS32_LCD_PrintChar(c) APP_LCD_PrintChar(c)
#define MIOS32_LCD_Clear() APP_LCD_Clear()
#define MIOS32_LCD_SpecialCharInit(num, table) APP_LCD_SpecialCharInit(num, table)
#define MIOS32_LCD_BColourSet(r, g, b) APP_LCD_BColourSet(r, g, b)
#define MIOS32_LCD_FColourSet(r, g, b) APP_LCD_FColourSet(r, g, b)


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_LCD_Init(u32 mode);
extern s32 MIOS32_LCD_DeviceSet(u8 device);
extern u8  MIOS32_LCD_DeviceGet(void);
extern s32 MIOS32_LCD_CursorSet(u16 line, u16 column);
extern s32 MIOS32_LCD_GCursorSet(u16 x, u16 y);
extern s32 MIOS32_LCD_CursorMapSet(u8 map_table[]);
extern s32 MIOS32_LCD_PrintString(char *str);

extern s32 MIOS32_LCD_SpecialCharsInit(u8 table[64]);

extern s32 MIOS32_LCD_FontInit(u8 *font);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by APP_LCD driver
extern s16 mios32_lcd_type;
extern u8  mios32_lcd_device;
extern s16 mios32_lcd_line;
extern s16 mios32_lcd_column;

extern u8  mios32_lcd_cursor_map[MIOS32_LCD_MAX_MAP_LINES];

extern s16 mios32_lcd_x;
extern s16 mios32_lcd_y;

extern u8 *mios32_lcd_font;
extern u8 mios32_lcd_font_width;
extern u8 mios32_lcd_font_height;
extern u8 mios32_lcd_font_x0;
extern u8 mios32_lcd_font_offset;

#endif /* _MIOS32_LCD_H */
