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


// fixed format, don't change!
#define MIOS32_LCD_FONT_WIDTH_IX    0
#define MIOS32_LCD_FONT_HEIGHT_IX   1
#define MIOS32_LCD_FONT_X0_IX       2
#define MIOS32_LCD_FONT_OFFSET_IX   3
#define MIOS32_LCD_FONT_BITMAP_IX   4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  // character LCDs
  MIOS32_LCD_TYPE_CLCD     = 0x00,
  MIOS32_LCD_TYPE_CLCD_DOG = 0x01,

  // graphical LCDs (bit #7 set)
  MIOS32_LCD_TYPE_GLCD_CUSTOM       = 0x80, // for app_lcd drivers so that MIOS32_LCD_TypeIsGLCD() returns 1
  MIOS32_LCD_TYPE_GLCD_KS0108       = 0x81,
  MIOS32_LCD_TYPE_GLCD_KS0108_INVCS = 0x82,
  MIOS32_LCD_TYPE_GLCD_DOG          = 0x83,
  MIOS32_LCD_TYPE_GLCD_SSD1306      = 0x84,
  MIOS32_LCD_TYPE_GLCD_SSD1306_ROTATED = 0x85,
  MIOS32_LCD_TYPE_GLCD_SED1520      = 0x86,
} mios32_lcd_type_t;


typedef struct {
  mios32_lcd_type_t lcd_type; // also stored in bootloader info range, see mios32_sys.h for address
  u8 num_x;                   // also stored in bootloader info range, see mios32_sys.h for address
  u8 num_y;                   // also stored in bootloader info range, see mios32_sys.h for address
  u16 width;                  // also stored in bootloader info range, see mios32_sys.h for address
  u16 height;                 // also stored in bootloader info range, see mios32_sys.h for address
  u8 colour_depth;            // NOT stored in bootloader info range (but set by LCD driver)
} mios32_lcd_parameters_t;


typedef struct {
  u8  *memory;
  u16 width;
  u16 height;
  u16 line_offset;
  u8  colour_depth;
} mios32_lcd_bitmap_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_LCD_Init(u32 mode);
extern s32 MIOS32_LCD_ParametersSet(mios32_lcd_parameters_t parameters);
extern mios32_lcd_parameters_t *MIOS32_LCD_ParametersGet(void);
extern const char* MIOS32_LCD_LcdTypeName(mios32_lcd_type_t lcd_type);
extern s32 MIOS32_LCD_TypeIsGLCD(void);
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
extern s32 MIOS32_LCD_BColourSet(u32 rgb);
extern s32 MIOS32_LCD_FColourSet(u32 rgb);

extern mios32_lcd_bitmap_t MIOS32_LCD_BitmapInit(u8 *memory, u16 width, u16 height, u16 line_offset, u8 colour_depth);
extern s32 MIOS32_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour);
extern s32 MIOS32_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by APP_LCD driver
extern mios32_lcd_parameters_t mios32_lcd_parameters;
extern u8  mios32_lcd_device;
extern u16 mios32_lcd_column;
extern u16 mios32_lcd_line;

extern u8  mios32_lcd_cursor_map[MIOS32_LCD_MAX_MAP_LINES];

extern u16 mios32_lcd_x;
extern u16 mios32_lcd_y;

#endif /* _MIOS32_LCD_H */
