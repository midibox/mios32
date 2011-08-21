// $Id$
//! \defgroup SCS
//!
//! LCD utility functions for Standard Control Surface
//!
//! Re-Used from MIDIbox SEQ V4 project, here the original doc:
//!
//! ==========================================================================
//! The 2x80 screen is buffered and can be output over multiple LCDs 
//! (e.g. 2 * 2x40, but also 4 * 2x20)
//!
//! The application should only access the displays via SEQ_LCD_* commands.
//!
//! The buffer method has the advantage, that multiple tasks can write to the
//! LCD without accessing the IO pins or the requirement for semaphores (to save time)
//!
//! Only changed characters (marked with flag 7 of each buffer byte) will be 
//! transfered to the LCD. This greatly improves performance as well, especially
//! if a graphical display should ever be supported by MBSEQ, but also in the
//! emulation.
//!
//! Another advantage: LCD access works independent from the physical dimension
//! of the LCDs. They are combined to one large 2x80 display, and SEQ_LCD_Update()
//! will take care for switching between the devices and setting the cursor.
//! If different LCDs should be used, only SCS_LCD_Update() needs to be changed.
//! ==========================================================================
//! 
//! By default we configure this driver to support a single LCD with the size
//! 2x<SCS_NUM_MENU_ITEMS*SCS_MENU_ITEM_WIDTH> (see scs_lcd.h, this size
//! can be overruled from external, e.g. in mios32_config.h)
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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

#include "scs_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 charset_menu[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare (this character can't be used in C strings)
  0x01, 0x03, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, // left-arrow
  0x01, 0x03, 0x07, 0x13, 0x19, 0x1c, 0x18, 0x10, // left/right arrow
  0x00, 0x00, 0x00, 0x10, 0x18, 0x1c, 0x18, 0x10, // right-arrow
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // spare
};

static const u8 charset_vbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x00, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
  0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e
};

static const u8 charset_hbars[64] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // empty bar
  0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, // "|  "
  0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // "|| "
  0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00, // "|||"
  0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, // " o "
  0x00, 0x10, 0x14, 0x15, 0x15, 0x14, 0x10, 0x00, // " > "
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // not used
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // not used
};

static u8 lcd_buffer[SCS_LCD_MAX_LINES][SCS_LCD_MAX_COLUMNS];

static u16 lcd_cursor_x;
static u16 lcd_cursor_y;


/////////////////////////////////////////////////////////////////////////////
// Display Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_Init(u32 mode)
{
#if 0
  // obsolete
  // now done in main.c

  u8 dev;

  // first LCD already initialized
  for(dev=1; dev<SCS_LCD_NUM_DEVICES; ++dev) {
    MIOS32_LCD_DeviceSet(dev);
    MIOS32_LCD_Init(0);
  }

  // switch back to first LCD
  MIOS32_LCD_DeviceSet(0);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Buffer handling functions
/////////////////////////////////////////////////////////////////////////////

// clears the buffer
s32 SCS_LCD_Clear(void)
{
  int i;
  
  u8 *ptr = (u8 *)lcd_buffer;
  for(i=0; i<SCS_LCD_MAX_LINES*SCS_LCD_MAX_COLUMNS; ++i)
    *ptr++ = ' ';

  lcd_cursor_x = 0;
  lcd_cursor_y = 0;

  return 0; // no error
}

// prints char into buffer and increments cursor
s32 SCS_LCD_PrintChar(char c)
{
  if( lcd_cursor_y >= SCS_LCD_MAX_LINES || lcd_cursor_x >= SCS_LCD_MAX_COLUMNS )
    return -1; // invalid cursor range

  u8 *ptr = &lcd_buffer[lcd_cursor_y][lcd_cursor_x++];
  if( (*ptr & 0x7f) != c )
      *ptr = c;

  return 0; // no error
}

// allows to change the buffer from other tasks w/o the need for semaphore handling
// it doesn't change the cursor
s32 SCS_LCD_BufferSet(u16 x, u16 y, char *str)
{
  // we assume, that the CPU allows atomic accesses to bytes, 
  // therefore no thread locking is required

  if( lcd_cursor_y >= SCS_LCD_MAX_LINES )
    return -1; // invalid cursor range

  u8 *ptr = &lcd_buffer[y][x];
  while( *str != '\0' ) {
    if( x++ >= SCS_LCD_MAX_COLUMNS )
      break;
    if( (*ptr & 0x7f) != *str )
      *ptr = *str;
    ++ptr;
    ++str;
  }

  return 0; // no error
}

// sets the cursor to a new buffer location
s32 SCS_LCD_CursorSet(u16 column, u16 line)
{
  // set character position
  lcd_cursor_x = column;
  lcd_cursor_y = line;

  return 0;
}

// transfers the buffer to LCDs
// if force != 0, it is ensured that the whole screen will be refreshed, regardless
// if characters have changed or not
s32 SCS_LCD_Update(u8 force)
{
  int next_x = -1;
  int next_y = -1;
  int remote_first_x[SCS_LCD_MAX_LINES];
  int remote_last_x[SCS_LCD_MAX_LINES];
  int x, y;

  for(y=0; y<2; ++y) {
    remote_first_x[y] = -1;
    remote_last_x[y] = -1;
  }

#ifdef MUTEX_LCD_TAKE
  MUTEX_LCD_TAKE;
#endif

  u8 *ptr = (u8 *)lcd_buffer;
  for(y=0; y<SCS_LCD_MAX_LINES; ++y)
    for(x=0; x<SCS_LCD_MAX_COLUMNS; ++x) {

      if( force || !(*ptr & 0x80) ) {
	if( remote_first_x[y] == -1 )
	  remote_first_x[y] = x;
	remote_last_x[y] = x;

	if( x != next_x || y != next_y ) {
	  MIOS32_LCD_DeviceSet(x / SCS_LCD_COLUMNS_PER_DEVICE);
	  MIOS32_LCD_CursorSet(x % SCS_LCD_COLUMNS_PER_DEVICE, y);
	}
	MIOS32_LCD_PrintChar(*ptr & 0x7f);

	MIOS32_IRQ_Disable(); // must be atomic
	*ptr |= 0x80;
	MIOS32_IRQ_Enable();

	next_y = y;
	next_x = x+1;

	// for multiple LCDs: ensure that cursor is set when we reach the next partition
	if( (next_x % SCS_LCD_COLUMNS_PER_DEVICE) == 0 )
	  next_x = -1;
      }
      ++ptr;
    }

#ifdef MUTEX_LCD_GIVE
  MUTEX_LCD_GIVE;
#endif

#if 0
  // forward display changes to remote client
  if( seq_ui_remote_mode == SCS_UI_REMOTE_MODE_SERVER || seq_ui_remote_active_mode == SCS_UI_REMOTE_MODE_SERVER ) {
    for(y=0; y<SCS_LCD_MAX_LINES; ++y)
      if( remote_first_x[y] >= 0 )
	SCS_MIDI_SYSEX_REMOTE_Server_SendLCD(remote_first_x[y],
					     y,
					     (u8 *)&lcd_buffer[y][remote_first_x[y]],
					     remote_last_x[y]-remote_first_x[y]+1);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// initialise character set (if not already active)
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_InitSpecialChars(scs_lcd_charset_t charset)
{
  static scs_lcd_charset_t current_charset = SCS_LCD_CHARSET_None;
  s32 status = 0;

  if( charset != current_charset ) {
    current_charset = charset;

#ifdef MUTEX_LCD_TAKE
    MUTEX_LCD_TAKE;
#endif
    int dev;
    for(dev=0; dev<SCS_LCD_NUM_DEVICES; ++dev) {
      MIOS32_LCD_DeviceSet(dev);
      switch( charset ) {
        case SCS_LCD_CHARSET_Menu:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_menu);
	  break;
        case SCS_LCD_CHARSET_VBars:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_vbars);
	  break;
        case SCS_LCD_CHARSET_HBars:
	  MIOS32_LCD_SpecialCharsInit((u8 *)charset_hbars);
	  break;
        default:
	  status = -1; // charset doesn't exist
      }
    }

#ifdef MUTEX_LCD_GIVE
    MUTEX_LCD_GIVE;
#endif

#if 0
    // forward charset change to remote client
    if( seq_ui_remote_mode == SCS_UI_REMOTE_MODE_SERVER )
      SCS_MIDI_SYSEX_REMOTE_Server_SendCharset(charset);
#endif
  }

  return status; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a string
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_PrintString(char *str)
{
  while( *str != '\0' ) {
    if( lcd_cursor_x >= SCS_LCD_MAX_COLUMNS )
      break;
    SCS_LCD_PrintChar(*str);
    ++str;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints a formatted string
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_PrintFormattedString(char *format, ...)
{
  char buffer[SCS_LCD_MAX_COLUMNS]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return SCS_LCD_PrintString(buffer);
}


/////////////////////////////////////////////////////////////////////////////
// prints <num> spaces
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_PrintSpaces(int num)
{
  while( num > 0 ) {
    SCS_LCD_PrintChar(' ');
    --num;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints padded string
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_PrintStringPadded(char *str, u32 width)
{
  // replacement for not supported "%:-40s" of simple sprintf function

  u32 pos;
  u8 fill = 0;
  for(pos=0; pos<width; ++pos) {
    char c = str[pos];
    if( c == 0 )
      fill = 1;
    SCS_LCD_PrintChar(fill ? ' ' : c);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// prints centered string
/////////////////////////////////////////////////////////////////////////////
s32 SCS_LCD_PrintStringCentered(char *str, u32 width)
{
  u32 strLen = strlen(str);
  if( strLen > width )
    return SCS_LCD_PrintStringPadded(str, width);

  u32 leftSpaces = (width - strLen) / 2;
  if( leftSpaces )
    SCS_LCD_PrintSpaces(++leftSpaces);
  SCS_LCD_PrintString(str);
  SCS_LCD_PrintSpaces(width-leftSpaces-strLen); // PrintSpaces checks by itself if values is <= 0

  return 0; // no error
}

//! \}
