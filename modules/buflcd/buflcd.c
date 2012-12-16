// $Id$
//! \defgroup BUFLCD
//!
//! Buffered LCD output driver
//!
//! This module provides similar functions like MIOS32_LCD, but characters are
//! not print immediately. Instead, they will be buffered.
//!
//! This method has the advantage, that multiple tasks can write to the
//! LCD without accessing the IO pins or the requirement for semaphores (to save time)
//!
//! Only changed characters (marked with flag 7 of each buffer byte) will be 
//! transfered to the LCD. This greatly improves performance as well, especially
//! if a graphical display is connected.
//!
//! Another advantage: LCD access works independent from the physical dimension
//! of the LCDs. E.g. two 2x40 LCDs can be combined to one large 2x80 display,
//! and BUFLCD_Update() function will take care for switching between the devices
//! and setting the cursor.
//!
//! BUFLCD_Update() has to be called periodically from a low-priority task!
//!
//! Usage examples:\n
//!   $MIOS32_PATH/apps/controllers/midibox_lc\n
//!   $MIOS32_PATH/apps/controllers/midibox_mm
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <stdarg.h>
#include <string.h>

#if BUFLCD_SUPPORT_GLCD_FONTS
#include <glcd_font.h>
#endif

#include "buflcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define BUFLCD_MAX_COLUMNS  (BUFLCD_NUM_DEVICES*BUFLCD_COLUMNS_PER_DEVICE)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_buffer[BUFLCD_MAX_LINES][BUFLCD_MAX_COLUMNS];
#if BUFLCD_SUPPORT_GLCD_FONTS
static u8 lcd_buffer_font[BUFLCD_MAX_LINES][BUFLCD_MAX_COLUMNS];
#endif

static u16 lcd_cursor_x;
static u8 lcd_cursor_y;

// BUFLCD_NUM_DEVICES can be changed during runtime to a smaller value
static u8 buflcd_num_devices;

// BUFLCD_COLUMNS_PER_DEVICE can be changed during runtime to a smaller value
static u8 buflcd_columns_per_device;

// BUFLCD_MAX_LINES can be changed during runtime to a smaller value
static u8 buflcd_max_lines;

// allows to add an x/y offset where the buffer is print out
static u8 buflcd_offset_x;
static u8 buflcd_offset_y;

#if BUFLCD_SUPPORT_GLCD_FONTS
// current selected font
static u8 lcd_current_font;
#endif

/////////////////////////////////////////////////////////////////////////////
//! Display Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_Init(u32 mode)
{
  buflcd_num_devices = BUFLCD_NUM_DEVICES;
  buflcd_columns_per_device = BUFLCD_COLUMNS_PER_DEVICE;
  buflcd_max_lines = BUFLCD_MAX_LINES;
  buflcd_offset_x = 0;
  buflcd_offset_y = 0;

  if( !mode )
    BUFLCD_Clear();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! sets the number of devices to which the output is transfered.
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_NumDevicesSet(u8 devices)
{
  if( devices > BUFLCD_NUM_DEVICES )
    return -1; // too many devices
  buflcd_num_devices = devices;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! returns the number of devices to which the output is transfered
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_NumDevicesGet(void)
{
  return buflcd_num_devices;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the number of columns which are print per device
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_ColumnsPerDeviceSet(u8 columns)
{
  if( columns > BUFLCD_MAX_COLUMNS ) // BUFLCD_COLUMNS_PER_DEVICE )
    return -1; // too many columns (note: we allow to change device=1 and columns=full during runtime!)
  buflcd_columns_per_device = columns;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! returns the number of columns which are print per device
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_ColumnsPerDeviceGet(void)
{
  return buflcd_columns_per_device;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the number of lines which are print per device
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_NumLinesSet(u8 lines)
{
  if( lines > BUFLCD_MAX_LINES )
    return -1; // too many lines
  buflcd_max_lines = lines;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! returns the number of lines which are print per device
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_NumLinesGet(void)
{
  return buflcd_max_lines;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the first column at which a line is print
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_OffsetXSet(u8 offset)
{
  buflcd_offset_x = offset;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! returns the first column at which a line is print
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_OffsetXGet(void)
{
  return buflcd_offset_x;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the first line at which characters are print
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_OffsetYSet(u8 offset)
{
  buflcd_offset_y = offset;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! returns the first column at which characters are print
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_OffsetYGet(void)
{
  return buflcd_offset_y;
}


/////////////////////////////////////////////////////////////////////////////
//! copies the current LCD buffer line into the output string
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_BufferGet(char *str, u8 line, u8 len)
{
  if( line >= BUFLCD_MAX_LINES )
    return -1; // invalid line

  if( len > BUFLCD_MAX_COLUMNS )
    len = BUFLCD_MAX_COLUMNS;

  u8 *ptr = &lcd_buffer[line][0];
  int i;
  for(i=0; i<len; ++i)
    *str++ = *ptr++;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! clears the buffer
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_Clear(void)
{
  int i;

  u8 *ptr = (u8 *)lcd_buffer;
  for(i=0; i<BUFLCD_MAX_LINES*BUFLCD_MAX_COLUMNS; ++i)
    *ptr++ = ' ';

#if BUFLCD_SUPPORT_GLCD_FONTS
  u8 *font_ptr = (u8 *)lcd_buffer_font;
  for(i=0; i<BUFLCD_MAX_LINES*BUFLCD_MAX_COLUMNS; ++i)
    *font_ptr++ = 'n';
  lcd_current_font = 0; // force switch to new font
#endif

  lcd_cursor_x = 0;
  lcd_cursor_y = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! selects a font for GLCD (only works with BUFLCD_SUPPORT_GLCD_FONTS)
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_FontInit(u8 *font)
{
#if !BUFLCD_SUPPORT_GLCD_FONTS
  return -1; // not supported
#else

  if     ( font == GLCD_FONT_NORMAL )        lcd_current_font = 'n';
  else if( font == GLCD_FONT_BIG )           lcd_current_font = 'b';
  else if( font == GLCD_FONT_SMALL )         lcd_current_font = 's';
  else if( font == GLCD_FONT_KNOB_ICONS )    lcd_current_font = 'k';
  else if( font == GLCD_FONT_METER_ICONS_H ) lcd_current_font = 'h';
  else if( font == GLCD_FONT_METER_ICONS_V ) lcd_current_font = 'v';
  else {
    lcd_current_font = 'n';
    return -2; // unsupported font
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! prints char into buffer and increments cursor
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_PrintChar(char c)
{
  if( lcd_cursor_y >= BUFLCD_MAX_LINES || lcd_cursor_x >= BUFLCD_MAX_COLUMNS )
    return -1; // invalid cursor range

  u8 *ptr = &lcd_buffer[lcd_cursor_y][lcd_cursor_x];
  if( (*ptr & 0x7f) != c )
    *ptr = c;

#if BUFLCD_SUPPORT_GLCD_FONTS
  u8 *font_ptr = &lcd_buffer_font[lcd_cursor_y][lcd_cursor_x];
  if( *font_ptr != lcd_current_font ) {
    *font_ptr = lcd_current_font;
    *ptr &= 0x7f; // new font: ensure that character will be updated
  }
#endif

  ++lcd_cursor_x;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! sets the cursor to a new buffer location
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_CursorSet(u16 column, u16 line)
{
  // set character position
  lcd_cursor_x = column;
  lcd_cursor_y = line;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! transfers the buffer to LCDs
//! \param[in] force if != 0, it is ensured that the whole screen will be refreshed, regardless
//! if characters have changed or not
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_Update(u8 force)
{
  int next_x = -1;
  int next_y = -1;
  int x, y;

  int phys_y = buflcd_offset_y;
  for(y=0; y<buflcd_max_lines; ++y, ++phys_y) {
    u8 *ptr = (u8 *)lcd_buffer[y];
#if BUFLCD_SUPPORT_GLCD_FONTS
    u8 *font_ptr = (u8 *)lcd_buffer_font[y];
#endif
    int phys_x = buflcd_offset_x;
    for(x=0; x<(buflcd_num_devices*buflcd_columns_per_device); ++x, ++phys_x) {

      u8 device = phys_x / buflcd_columns_per_device;
      if( device >= buflcd_num_devices )
	break;

      if( force || !(*ptr & 0x80) ) {
#if BUFLCD_SUPPORT_GLCD_FONTS
	u8 *glcd_font = NULL;
	switch( *font_ptr ) {
	case 'b': glcd_font = (u8 *)GLCD_FONT_BIG; break;
	case 's': glcd_font = (u8 *)GLCD_FONT_SMALL; break;
	case 'k': glcd_font = (u8 *)GLCD_FONT_KNOB_ICONS; break;
	case 'h': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_H; break;
	case 'v': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_V; break;
	default: // and 'n'
	  glcd_font = (u8 *)GLCD_FONT_NORMAL;
	}

	MIOS32_LCD_FontInit(glcd_font);
#endif

	if( x != next_x || y != next_y ) {
#if BUFLCD_SUPPORT_GLCD_FONTS
	  // temporary use pseudo-font to ensure that Y is handled equaly for all fonts
	  u8 pseudo_font[4];
	  // just to ensure...
#if MIOS32_LCD_FONT_WIDTH_IX != 0 || MIOS32_LCD_FONT_HEIGHT_IX != 1 || MIOS32_LCD_FONT_X0_IX != 2 || MIOS32_LCD_FONT_OFFSET_IX != 3
# error "Please adapt this part for new LCD Font parameter positions!"
#endif
	  pseudo_font[MIOS32_LCD_FONT_WIDTH_IX] = glcd_font[MIOS32_LCD_FONT_WIDTH_IX];
	  pseudo_font[MIOS32_LCD_FONT_HEIGHT_IX] = 1*8; // forced!
	  pseudo_font[MIOS32_LCD_FONT_X0_IX] = glcd_font[MIOS32_LCD_FONT_X0_IX];
	  pseudo_font[MIOS32_LCD_FONT_OFFSET_IX] = glcd_font[MIOS32_LCD_FONT_OFFSET_IX];
	  MIOS32_LCD_FontInit((u8 *)&pseudo_font);
#endif
	  MIOS32_LCD_DeviceSet(device);
	  MIOS32_LCD_CursorSet(phys_x % buflcd_columns_per_device, phys_y);
#if BUFLCD_SUPPORT_GLCD_FONTS
	  // switch back to original font
	  MIOS32_LCD_FontInit(glcd_font);
#endif
	}

	MIOS32_LCD_PrintChar(*ptr & 0x7f);

	MIOS32_IRQ_Disable(); // must be atomic
	*ptr |= 0x80;
	MIOS32_IRQ_Enable();

	next_y = y;
	next_x = x+1;

	// for multiple LCDs: ensure that cursor is set when we reach the next partition
	if( (next_x % buflcd_columns_per_device) == 0 )
	  next_x = -1;
      }
      ++ptr;
#if BUFLCD_SUPPORT_GLCD_FONTS
      ++font_ptr;
#endif
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! prints a string
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_PrintString(char *str)
{
  while( *str != '\0' ) {
    if( lcd_cursor_x >= (BUFLCD_NUM_DEVICES*buflcd_columns_per_device) )
      break;
    BUFLCD_PrintChar(*str);
    ++str;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! prints a formatted string
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_PrintFormattedString(char *format, ...)
{
  char buffer[BUFLCD_MAX_COLUMNS];
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return BUFLCD_PrintString(buffer);
}


/////////////////////////////////////////////////////////////////////////////
//! prints <num> spaces
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_PrintSpaces(int num)
{
  while( num > 0 ) {
    BUFLCD_PrintChar(' ');
    --num;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! prints padded string
//! replacement for not supported "%:-40s" of simple sprintf function
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_PrintStringPadded(char *str, u32 width)
{
  u32 pos;
  u8 fill = 0;
  for(pos=0; pos<width; ++pos) {
    char c = str[pos];
    if( c == 0 )
      fill = 1;
    BUFLCD_PrintChar(fill ? ' ' : c);
  }

  return 0; // no error
}

//! \}
