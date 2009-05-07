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

// maximum number of mapped CLCD lines
#ifndef MIOS32_LCD_MAX_MAP_LINES
#define MIOS32_LCD_MAX_MAP_LINES 4
#endif

// The boot message which is print during startup and returned on a SysEx query
#ifndef MIOS32_LCD_BOOT_MSG_LINE1
#define MIOS32_LCD_BOOT_MSG_LINE1 "Unnamed App."
#endif

#ifndef MIOS32_LCD_BOOT_MSG_LINE2
#define MIOS32_LCD_BOOT_MSG_LINE2 "www.midibox.org"
#endif

// the startup delay in mS
// if 0, no message will be print
#ifndef MIOS32_LCD_BOOT_MSG_DELAY
#define MIOS32_LCD_BOOT_MSG_DELAY 2000
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_LCD_TYPE_CLCD  0
#define MIOS32_LCD_TYPE_GLCD  1


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_LCD_Init(u32 mode);
extern s32 MIOS32_LCD_DeviceSet(u8 device);
extern u8  MIOS32_LCD_DeviceGet(void);
extern s32 MIOS32_LCD_CursorSet(u16 column, u16 line);
extern s32 MIOS32_LCD_GCursorSet(u16 x, u16 y);
extern s32 MIOS32_LCD_CursorMapSet(u8 map_table[]);
extern s32 MIOS32_LCD_PrintString(char *str);
extern s32 MIOS32_LCD_PrintFormattedString(char *format, ...);

extern s32 MIOS32_LCD_PrintBootMessage(void);

extern s32 MIOS32_LCD_SpecialCharsInit(u8 table[64]);

extern s32 MIOS32_LCD_FontInit(u8 *font);

// forward functions to app_lcd
extern s32 MIOS32_LCD_Data(u8 data);
extern s32 MIOS32_LCD_Cmd(u8 cmd);
extern s32 MIOS32_LCD_Clear(void);
extern s32 MIOS32_LCD_PrintChar(char c);
extern s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8]);
extern s32 MIOS32_LCD_BColourSet(u8 r, u8 g, u8 b);
extern s32 MIOS32_LCD_FColourSet(u8 r, u8 g, u8 b);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by APP_LCD driver
extern s16 mios32_lcd_type;
extern u8  mios32_lcd_device;
extern s16 mios32_lcd_column;
extern s16 mios32_lcd_line;

extern u8  mios32_lcd_cursor_map[MIOS32_LCD_MAX_MAP_LINES];

extern s16 mios32_lcd_x;
extern s16 mios32_lcd_y;

extern u8 *mios32_lcd_font;
extern u8 mios32_lcd_font_width;
extern u8 mios32_lcd_font_height;
extern u8 mios32_lcd_font_x0;
extern u8 mios32_lcd_font_offset;

#endif /* _MIOS32_LCD_H */
