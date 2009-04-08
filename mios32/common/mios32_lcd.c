// $Id$
//! \defgroup MIOS32_LCD
//!
//! LCD functions for MIOS32
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_LCD)

// requires application specific driver
#include <app_lcd.h>

#include <stdarg.h>


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

s16 mios32_lcd_type;
u8  mios32_lcd_device = 0; // (not done in MIOS32_Init to allow the initialisation of multiple LCDs)
s16 mios32_lcd_column;
s16 mios32_lcd_line;

u8  mios32_lcd_cursor_map[MIOS32_LCD_MAX_MAP_LINES];

s16 mios32_lcd_x;
s16 mios32_lcd_y;

u8 *mios32_lcd_font = NULL;
u8 mios32_lcd_font_width;
u8 mios32_lcd_font_height;
u8 mios32_lcd_font_x0;
u8 mios32_lcd_font_offset;


/////////////////////////////////////////////////////////////////////////////
//! Initializes LCD driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Init(u32 mode)
{
  s32 ret;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // initial LCD type (can be set to a different type in APP_LCD_Init()
  mios32_lcd_type = MIOS32_LCD_TYPE_CLCD;

  // set initial cursor map for character LCDs
  u8 cursor_map[] = {0x00, 0x40, 0x14, 0x54}; // offset line 0/1/2/3
  MIOS32_LCD_CursorMapSet(cursor_map);
  // note: this has to be done before APP_LCD_Init() is called, so that
  // the driver is able to modify the default cursor mapping
  // usage example: "dog" LCDs

  // call application specific init function
  if( (ret=APP_LCD_Init(mode)) < 0 )
    return ret;

  // clear screen
  MIOS32_LCD_Clear();

  // set character and graphical cursor to initial position
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_GCursorSet(0, 0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Selects LCD device
//! \param[in] device LCD device number
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_DeviceSet(u8 device)
{
  mios32_lcd_device = device;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns LCD device
//! \return device number
/////////////////////////////////////////////////////////////////////////////
u8 MIOS32_LCD_DeviceGet(void)
{
  return mios32_lcd_device;
}


/////////////////////////////////////////////////////////////////////////////
//! Sets cursor to given position
//! \param[in] column number
//! \param[in] line number
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_CursorSet(u16 column, u16 line)
{
  // set character position
  mios32_lcd_column = column;
  mios32_lcd_line = line;

  // set graphical cursor depending on font width
  mios32_lcd_x = column * mios32_lcd_font_width;
  mios32_lcd_y = line * mios32_lcd_font_height;

  // forward new cursor position to app driver
  return APP_LCD_CursorSet(column, line);
}


/////////////////////////////////////////////////////////////////////////////
//! Sets graphical cursor to given position<BR>
//! Only relevant for GLCDs
//! \param[in] x position
//! \param[in] y position
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_GCursorSet(u16 x, u16 y)
{
  mios32_lcd_x = x;
  mios32_lcd_y = y;

  // forward new cursor position to app driver
  return APP_LCD_GCursorSet(x, y);
}


/////////////////////////////////////////////////////////////////////////////
//! Set the cursor map for character displays
//!
//! By default the positions are configured for 2x16, 2x20, 4x20 and 2x40 displays:
//! \code
//!   u8 cursor_map[] = {0x00, 0x40, 0x14, 0x54}; // offset line 0/1/2/3
//!   MIOS32_LCD_CursorMapSet(cursor_map);
//! \endcode
//!
//! For 4x16 displays, the configuration has to be changed:
//! \code
//!   u8 cursor_map[] = {0x00, 0x40, 0x10, 0x50}; // offset line 0/1/2/3
//!   MIOS32_LCD_CursorMapSet(cursor_map);
//! \endcode
//!
//! For 3x16 DOG displays use:
//! \code
//!   u8 cursor_map[] = {0x00, 0x10, 0x20, 0x30}; // offset line 0/1/2/3
//!   MIOS32_LCD_CursorMapSet(cursor_map);
//! \endcode
//! \param[in] map_table the cursor map
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_CursorMapSet(u8 map_table[])
{
  s32 i;

  for(i=0; i<MIOS32_LCD_MAX_MAP_LINES; ++i)
    mios32_lcd_cursor_map[i] = map_table[i];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Prints a \\0 (zero) terminated string
//! \param[in] str pointer to string
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_PrintString(char *str)
{
  s32 status = 0;

  while( *str != '\0' )
    status |= MIOS32_LCD_PrintChar(*str++);

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a \\0 (zero) terminated formatted string (like printf)
//! \param[in] *format zero-terminated format string - 64 characters supported maximum!
//! \param ... additional arguments
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_PrintFormattedString(char *format, ...)
{
  char buffer[64]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return MIOS32_LCD_PrintString(buffer);
}


/////////////////////////////////////////////////////////////////////////////
//! Used during startup to print the boot message, which consists of two lines
//! specified with MIOS32_LCD_BOOT_MSG_LINE1 and MIOS32_LCD_BOOT_MSG_LINE2.<BR>
//! Both lines should be adapted in the mios32_config.h file of an application.<BR>
//! The message is automatically print by the programming model after each reset.<BR>
//! It will also be returned on a SysEx query.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_PrintBootMessage(void)
{
  s32 status = 0;

  status |= MIOS32_LCD_DeviceSet(0);
  status |= MIOS32_LCD_CursorSet(0, 0);
  status |= MIOS32_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
  status |= MIOS32_LCD_CursorSet(0, 1);
  status |= MIOS32_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE2);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes all 8 special characters
//! \param[in] table 8 * 8 (=64) bytes which contain the pixel lines
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_SpecialCharsInit(u8 table[64])
{
  int i;

  for(i=0; i<8; ++i)
    if( APP_LCD_SpecialCharInit(i, table + (size_t)i*8) )
      return -1; // error during initialisation

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes the graphical font<BR>
//! Only relevant for GLCDs
//! \param[in] *font pointer to font
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_FontInit(u8 *font)
{
  // get width/height/offset from font header
  mios32_lcd_font_width = font[0];
  mios32_lcd_font_height = font[1];
  mios32_lcd_font_x0 = font[2];
  mios32_lcd_font_offset = font[3];

  // set pointer to characters
  mios32_lcd_font = font+4;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends data byte to LCD
//! \param[in] data byte which should be sent to LCD
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Data(u8 data)
{
  // -> forward to app_lcd
  return APP_LCD_Data(data);
}


/////////////////////////////////////////////////////////////////////////////
//! Sends command byte to LCD
//! \param[in] cmd command byte which should be sent to LCD
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Cmd(u8 cmd)
{
  // -> forward to app_lcd
  return APP_LCD_Cmd(cmd);
}


/////////////////////////////////////////////////////////////////////////////
//! Clear Screen
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Clear(void)
{
  // -> forward to app_lcd
  return APP_LCD_Clear();
}


/////////////////////////////////////////////////////////////////////////////
//! Prints a single character
//! \param[in] c character to be print
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_PrintChar(char c)
{
  // -> forward to app_lcd
  return APP_LCD_PrintChar(c);
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a single special character
//! \param[in] num character number (0-7)
//! \param[in] table 8 byte pattern
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  // -> forward to app_lcd
  return APP_LCD_SpecialCharInit(num, table);
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the background colour<BR>
//! Only relevant for colour GLCDs
//! \param[in] r (red) value
//! \param[in] g (green) value
//! \param[in] b (blue) value
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_BColourSet(u8 r, u8 g, u8 b)
{
  // -> forward to app_lcd
  return APP_LCD_BColourSet(r, g, b);
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the foreground colour<BR>
//! Only relevant for colour GLCDs
//! \param[in] r (red) value
//! \param[in] g (green) value
//! \param[in] b (blue) value
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_FColourSet(u8 r, u8 g, u8 b)
{
  // -> forward to app_lcd
  return APP_LCD_FColourSet(r, g, b);
}

//! \}

#endif /* MIOS32_DONT_USE_LCD */
