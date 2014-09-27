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

mios32_lcd_parameters_t mios32_lcd_parameters;
u8  mios32_lcd_device = 0; // (not done in MIOS32_Init to allow the initialisation of multiple LCDs)
u16 mios32_lcd_column;
u16 mios32_lcd_line;

u8  mios32_lcd_cursor_map[MIOS32_LCD_MAX_MAP_LINES];

u16 mios32_lcd_x;
u16 mios32_lcd_y;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mios32_lcd_bitmap_t font_bitmap;


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

  // fetch config from BSL info range
  MIOS32_LCD_ParametersFetchFromBslInfoRange();

  // disable font bitmap
  font_bitmap.width = 0;

  // set initial cursor map for character LCDs
  u8 cursor_map[] = {0x00, 0x40, 0x14, 0x54}; // offset line 0/1/2/3
  MIOS32_LCD_CursorMapSet(cursor_map);
  // note: this has to be done before APP_LCD_Init() is called, so that
  // the driver is able to modify the default cursor mapping
  // usage example: "dog" LCDs

#if !defined(MIOS32_FAMILY_EMULATION)
  // initial delay - some LCDs need this!
  MIOS32_DELAY_Wait_uS(50000);
#endif

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
//! Sets LCD Parameters during runtime.
//!
//! Note that the default parameters are stored in Bootloader Info range, and
//! fetched during startup via \ref MIOS32_LCD_ParametersFetchFromBslInfoRange
//!
//! But it's possible to change them during runtime,
//! and it's also possible that an alternative LCD driver (!= MIOS32_LCD=universal)
//! overrides them!
//! \param[in] the new lcd_parameters
//! \return 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_ParametersSet(mios32_lcd_parameters_t lcd_parameters)
{
  mios32_lcd_parameters = lcd_parameters;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Fetches the LCD Parameters from Bootloader Info range.
//!
//! Note that this function is alreay called during startup. It can be used
//! if the original type and dimensions should be restored after another 
//! LCD type has been selected during runtime.
//! \return 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_ParametersFetchFromBslInfoRange(void)
{
  // initial LCD parameters
  static const mios32_lcd_parameters_t default_parameters = {
    .lcd_type = MIOS32_LCD_TYPE_CLCD,
    .num_x = 2, // since MBHP_CORE_STM32 and MBHP_CORE_LPC17 has two J15 ports
    .num_y = 1,
    .width = 20, // since most people will (probably) build the SCS
    .height = 2,
    .colour_depth = 1,
  };
  mios32_lcd_parameters = default_parameters;

#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  // read from bootloader info range
  u8 *lcd_par_confirm = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_CONFIRM;
  if( *lcd_par_confirm == 0x42 ) {
    u8 *lcd_par_type = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_TYPE;
    mios32_lcd_parameters.lcd_type = *lcd_par_type;
    u8 *lcd_par_num_x = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_NUM_X;
    mios32_lcd_parameters.num_x = *lcd_par_num_x;
    u8 *lcd_par_num_y = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_NUM_Y;
    mios32_lcd_parameters.num_y = *lcd_par_num_y;
    u8 *lcd_par_width = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_WIDTH;
    mios32_lcd_parameters.width = *lcd_par_width;
    // extra: if width == 255, increase to 256 (255 is very uncommon, and 256 can't be configured due to byte limitation)
    if( mios32_lcd_parameters.width == 255 )
      mios32_lcd_parameters.width = 256; // in future, we could also provide additional codings for higher widths, e.g. 254 for 320 pixel
    u8 *lcd_par_height = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_HEIGHT;
    mios32_lcd_parameters.height = *lcd_par_height;
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the current LCD parameters
/////////////////////////////////////////////////////////////////////////////
mios32_lcd_parameters_t *MIOS32_LCD_ParametersGet(void)
{
  return &mios32_lcd_parameters;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns 1 if the currently selected LCD type is a GLCD, otherwise 0
//! 
//! Note: GLCDs are determined by checking the mios32_lcd_parameters.lcd_type
//! setting - if bit #7 is set (>= 0x80), the LCD is a GLCD
//! \return 1 or 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_TypeIsGLCD(void)
{
  return mios32_lcd_parameters.lcd_type >= 0x80;
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of LCD type
// returns NULL if type unknown
/////////////////////////////////////////////////////////////////////////////
const char* MIOS32_LCD_LcdTypeName(mios32_lcd_type_t lcd_type)
{
  switch( lcd_type ) {
  case MIOS32_LCD_TYPE_CLCD: return "CLCD";
  case MIOS32_LCD_TYPE_CLCD_DOG: return "CLCD_DOG";
  
  case MIOS32_LCD_TYPE_GLCD_CUSTOM: return "GLCD_CUSTOM";
  case MIOS32_LCD_TYPE_GLCD_KS0108: return "GLCD_KS0108";
  case MIOS32_LCD_TYPE_GLCD_KS0108_INVCS: return "GLCD_KS0108_INVCS";
  case MIOS32_LCD_TYPE_GLCD_DOG: return "GLCD_DOG";
  case MIOS32_LCD_TYPE_GLCD_SSD1306: return "GLCD_SSD1306";
  case MIOS32_LCD_TYPE_GLCD_SSD1306_ROTATED: return "GLCD_SSD1306_ROTATED";
  case MIOS32_LCD_TYPE_GLCD_SED1520: return "GLCD_SED1520";
  }

  return NULL;
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
  u8 font_width = 6;
  u8 font_height = 8;
  if( font_bitmap.width ) {
    font_width = font_bitmap.width;
    font_height = font_bitmap.height;
  }

  mios32_lcd_x = column * font_width;
  mios32_lcd_y = line * font_height;

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
s32 MIOS32_LCD_PrintString(const char *str)
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
s32 MIOS32_LCD_PrintFormattedString(const char *format, ...)
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
  font_bitmap.memory = (u8 *)&font[MIOS32_LCD_FONT_BITMAP_IX] + (size_t)font[MIOS32_LCD_FONT_X0_IX];
  font_bitmap.width = font[MIOS32_LCD_FONT_WIDTH_IX];
  font_bitmap.height = font[MIOS32_LCD_FONT_HEIGHT_IX];
  font_bitmap.line_offset = font[MIOS32_LCD_FONT_OFFSET_IX];
  font_bitmap.colour_depth = 1;

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
  s32 status;

  if( MIOS32_LCD_TypeIsGLCD() ) { // GLCD
    if( !font_bitmap.width )
      return -1;    // font not initialized yet!

    mios32_lcd_bitmap_t bitmap = font_bitmap;
    bitmap.memory += (bitmap.height>>3) * bitmap.line_offset * (size_t)c;
    status = APP_LCD_BitmapPrint(bitmap);
  } else {
    status = APP_LCD_Data(c);
  }

  if( status >= 0 ) {
    // increment cursor
    ++mios32_lcd_column;
  }

  return status;
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
//! \param[in] rgb red/green/blue value, each colour with 8bit resolution:
//! \code
//!    u32 colour = (r << 16) | (g << 8) | (b << 0);
//! \endcode
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_BColourSet(u32 rgb)
{
  // -> forward to app_lcd
  return APP_LCD_BColourSet(rgb);
}


/////////////////////////////////////////////////////////////////////////////
//! Sets the foreground colour<BR>
//! Only relevant for colour GLCDs
//! \param[in] rgb red/green/blue value, each colour with 8bit resolution:
//! \code
//!    u32 colour = (r << 16) | (g << 8) | (b << 0);
//! \endcode
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_FColourSet(u32 rgb)
{
  // -> forward to app_lcd
  return APP_LCD_FColourSet(rgb);
}


/////////////////////////////////////////////////////////////////////////////
//! Only supported for graphical LCDs: initializes a bitmap type and clears
//! it with the background colour.
//!
//! Example:
//! \code
//!   // global array (!)
//!   u8 bitmap_array[APP_LCD_BITMAP_SIZE];
//!
//!   // Initialisation:
//!   mios32_lcd_bitmap_t bitmap = MIOS32_LCD_BitmapClear(bitmap_array, 
//!   						    APP_LCD_NUM_X*APP_LCD_WIDTH,
//!   						    APP_LCD_NUM_Y*APP_LCD_HEIGHT,
//!   						    APP_LCD_NUM_X*APP_LCD_WIDTH.
//!                                                 APP_LCD_COLOUR_DEPTH);
//! \endcode
//!
//! \param[in] memory pointer to the bitmap array
//! \param[in] width width of the bitmap (usually APP_LCD_NUM_X*APP_LCD_WIDTH)
//! \param[in] height height of the bitmap (usually APP_LCD_NUM_Y*APP_LCD_HEIGHT)
//! \param[in] line_offset byte offset between each line (usually same value as width)
//! \param[in] colour_depth how many bits are allocated by each pixel (usually APP_LCD_COLOUR_DEPTH)
//! \return a configured bitmap as mios32_lcd_bitmap_t
/////////////////////////////////////////////////////////////////////////////
mios32_lcd_bitmap_t MIOS32_LCD_BitmapInit(u8 *memory, u16 width, u16 height, u16 line_offset, u8 colour_depth)
{
  mios32_lcd_bitmap_t bitmap;

  bitmap.memory = memory;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.line_offset = line_offset;
  bitmap.colour_depth = colour_depth;

  return bitmap;
}



/////////////////////////////////////////////////////////////////////////////
//! Inserts a pixel at the given x/y position into the bitmap with the given colour.
//!
//! Example:
//! \code
//! \endcode
//!
//! \param[in] bitmap the bitmap where the pixel should be inserted
//! \param[in] x the X position of the pixel
//! \param[in] y the Y position of the pixel
//! \param[in] colour the colour of the pixel (value range depends on APP_LCD_COLOUR_DEPTH, mono GLCDs should use 0 or 1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  // -> forward to app_lcd
  return APP_LCD_BitmapPixelSet(bitmap, x, y, colour);
}


/////////////////////////////////////////////////////////////////////////////
//! Only supported for graphical LCDs: transfers a bitmap to the LCD 
//! at the current graphical cursor position.
//!
//! Example:
//! \code
//!   MIOS32_LCD_CursorSet(0, 0);
//!   MIOS32_LCD_BitmapPrint(bitmap);
//! \endcode
//!
//! \param[in] bitmap the bitmap which should be print
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  // -> forward to app_lcd
  return APP_LCD_BitmapPrint(bitmap);
}


//! \}

#endif /* MIOS32_DONT_USE_LCD */
