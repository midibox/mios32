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
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_buffer[BUFLCD_BUFFER_SIZE];

static u16 lcd_cursor_x;
static u8 lcd_cursor_y;

// taken from MIOS32 Bootloader configuration, but can also be changed during runtime
static u8 buflcd_device_num_x;
static u8 buflcd_device_num_y;
static u8 buflcd_device_height;
static u8 buflcd_device_width;

// allows to add an x/y offset where the buffer is print out
static u8 buflcd_offset_x;
static u8 buflcd_offset_y;

#if BUFLCD_SUPPORT_GLCD_FONTS
// current selected font
static u8 glcd_font_handling;
static u8 lcd_current_font;
#endif

/////////////////////////////////////////////////////////////////////////////
//! Display Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_Init(u32 mode)
{
  // how many devices are configured in the MIOS32 Bootloader?
  buflcd_device_num_x = mios32_lcd_parameters.num_x;
  buflcd_device_num_y = mios32_lcd_parameters.num_y;

  // graphical LCDs?
  if( MIOS32_LCD_TypeIsGLCD() ) {
    // take 80x2 by default - if bigger displays are used (which could display more), change during runtime
    buflcd_device_width  = mios32_lcd_parameters.width / 6;
    buflcd_device_height = mios32_lcd_parameters.height / 8;

#if BUFLCD_SUPPORT_GLCD_FONTS
    glcd_font_handling = (buflcd_device_num_x * buflcd_device_num_y * buflcd_device_width * buflcd_device_height) <= (BUFLCD_BUFFER_SIZE/2);
#endif
  } else {
    // if only two CLCDs, we assume that the user hasn't changed the configuration
    // and therefore support up to two 2x40 CLCDs (combined to 2x80)
    if( (mios32_lcd_parameters.num_x*mios32_lcd_parameters.num_y) <= 2 ) {
      buflcd_device_width  = 40;
      buflcd_device_height = 2;      
    } else {
      buflcd_device_width  = mios32_lcd_parameters.width;
      buflcd_device_height = mios32_lcd_parameters.height;
    }
  }

  buflcd_offset_x = 0;
  buflcd_offset_y = 0;

  if( !mode )
    BUFLCD_Clear();

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
//! max. number of characters depends on GLCD and BUFLCD_SUPPORT_GLCD_FONTS
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_MaxBufferGet(void)
{
#if BUFLCD_SUPPORT_GLCD_FONTS
  return glcd_font_handling ? (BUFLCD_BUFFER_SIZE/2) : BUFLCD_BUFFER_SIZE;
#else
  return BUFLCD_BUFFER_SIZE;
#endif
}




/////////////////////////////////////////////////////////////////////////////
//! sets the number of devices which are combined to a single line
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceNumXSet(u8 num_x)
{
  buflcd_device_num_x = num_x;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the number of devices which are combined to a single line
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceNumXGet(void)
{
  return buflcd_device_num_x;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the number of devices which are available in Y direction
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceNumYSet(u8 num_y)
{
  buflcd_device_num_y = num_y;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the number of devices which are available in Y direction
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceNumYGet(void)
{
  return buflcd_device_num_y;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the display width (in characters) of a single LCD
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceWidthSet(u8 width)
{
  buflcd_device_width = width;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the display width (in characters) of a single LCD
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceWidthGet(void)
{
  return buflcd_device_width;
}


/////////////////////////////////////////////////////////////////////////////
//! sets the display height (in characters) of a single LCD
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceHeightSet(u8 height)
{
  buflcd_device_height = height;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \returns the display height (in characters) of a single LCD
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceHeightGet(void)
{
  return buflcd_device_height;
}


/////////////////////////////////////////////////////////////////////////////
//! \returns 1 if GLCD font handling enabled
/////////////////////////////////////////////////////////////////////////////
s32 BUFLCD_DeviceFontHandlingEnabled(void)
{
#if BUFLCD_SUPPORT_GLCD_FONTS
  return glcd_font_handling;
#else
  return 0;
#endif
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
  u32 bufpos = line * buflcd_device_num_x * buflcd_device_width;
  if( bufpos >= BUFLCD_MaxBufferGet() )
    return -1; // invalid line

  u8 *ptr = &lcd_buffer[bufpos];
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

#if BUFLCD_SUPPORT_GLCD_FONTS
  u8 use_font_buffer = glcd_font_handling;
#else
  u8 use_font_buffer = 0;
#endif

  if( use_font_buffer ) {
    u8 *ptr = (u8 *)lcd_buffer;
    for(i=0; i<BUFLCD_BUFFER_SIZE/2; ++i)
      *ptr++ = ' ';
    for(; i<BUFLCD_BUFFER_SIZE; ++i)
      *ptr++ = 'n';
  } else {
    u8 *ptr = (u8 *)lcd_buffer;
    for(i=0; i<BUFLCD_BUFFER_SIZE; ++i)
      *ptr++ = ' ';
  }
#if BUFLCD_SUPPORT_GLCD_FONTS
  lcd_current_font = 'n';
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
  u32 bufpos = lcd_cursor_y * buflcd_device_num_x * buflcd_device_width + lcd_cursor_x;
  if( bufpos >= BUFLCD_MaxBufferGet() )
    return -1; // invalid line

  u8 *ptr = &lcd_buffer[bufpos];
  if( (*ptr & 0x7f) != c )
    *ptr = c;

#if BUFLCD_SUPPORT_GLCD_FONTS
  if( glcd_font_handling ) {
    u8 *font_ptr = &lcd_buffer[bufpos + (BUFLCD_BUFFER_SIZE/2)];
    if( *font_ptr != lcd_current_font ) {
      *font_ptr = lcd_current_font;
      *ptr &= 0x7f; // new font: ensure that character will be updated
    }
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

  u32 bufpos_len = BUFLCD_MaxBufferGet();
  int phys_y = buflcd_offset_y;
  for(y=0; y<buflcd_device_num_y*buflcd_device_height; ++y, ++phys_y) {
    u32 bufpos = y * buflcd_device_num_x * buflcd_device_width;
    if( bufpos >= bufpos_len )
      break;

    u8 *ptr = (u8 *)&lcd_buffer[bufpos];
#if BUFLCD_SUPPORT_GLCD_FONTS
    u8 *font_ptr = (u8 *)&lcd_buffer[bufpos + (BUFLCD_BUFFER_SIZE/2)];
#endif
    int phys_x = buflcd_offset_x;
    int device = (buflcd_device_num_x * (phys_y / buflcd_device_height)) - 1;
    for(x=0; x<(buflcd_device_num_x * buflcd_device_width) && bufpos < bufpos_len; ++x, ++phys_x, ++bufpos) {
      if( (phys_x % buflcd_device_width) == 0 )
	++device;

      if( force || !(*ptr & 0x80)
#if BUFLCD_SUPPORT_GLCD_FONTS
	  || (glcd_font_handling && !(*font_ptr & 0x80))
#endif
	  ) {
#if BUFLCD_SUPPORT_GLCD_FONTS
	u8 *glcd_font = NULL;
	if( glcd_font_handling ) {
	  switch( *font_ptr & 0x7f ) {
	  case 'n': glcd_font = (u8 *)GLCD_FONT_NORMAL; break;
	  case 'b': glcd_font = (u8 *)GLCD_FONT_BIG; break;
	  case 's': glcd_font = (u8 *)GLCD_FONT_SMALL; break;
	  case 'k': glcd_font = (u8 *)GLCD_FONT_KNOB_ICONS; break;
	  case 'h': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_H; break;
	  case 'v': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_V; break;
	  default:
	    glcd_font = NULL; // no character will be print
	  }

	  MIOS32_LCD_FontInit(glcd_font);
	}
#endif

	if( x != next_x || y != next_y ) {
#if BUFLCD_SUPPORT_GLCD_FONTS
	  if( glcd_font_handling && glcd_font ) {
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
	  }
#endif
	  MIOS32_LCD_DeviceSet(device);
	  MIOS32_LCD_CursorSet(phys_x % buflcd_device_width, phys_y % buflcd_device_height);
#if BUFLCD_SUPPORT_GLCD_FONTS
	  if( glcd_font_handling ) {
	    // switch back to original font
	    MIOS32_LCD_FontInit(glcd_font);
	  }
#endif
	}

#if BUFLCD_SUPPORT_GLCD_FONTS
	if( !glcd_font_handling || glcd_font )
#endif
	  MIOS32_LCD_PrintChar(*ptr & 0x7f);

	MIOS32_IRQ_Disable(); // must be atomic
	*ptr |= 0x80;
#if BUFLCD_SUPPORT_GLCD_FONTS
	*font_ptr |= 0x80;
#endif
	MIOS32_IRQ_Enable();

	next_y = y;
	next_x = x+1;

	// for multiple LCDs: ensure that cursor is set when we reach the next partition
	if( (next_x % buflcd_device_width) == 0 )
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
    if( lcd_cursor_x >= (buflcd_device_num_x * buflcd_device_width) )
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
  char buffer[128]; // hopefully enough?
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
