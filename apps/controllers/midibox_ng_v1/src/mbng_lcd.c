// $Id$
//! \defgroup MBNG_LCD
//! LCD access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

// prints execution time on each print event
#define DEBUG_PRINT_ITEM_PERFORMANCE 0


/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <stdarg.h>
#include <string.h>
#include <glcd_font.h>

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_file_l.h"
#include "mbng_event.h"
#include "mbng_patch.h"

/////////////////////////////////////////////////////////////////////////////
//! Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_charset;

static u16 lcd_cursor_device;
static u16 lcd_cursor_x;
static u16 lcd_cursor_y;

// taken from MIOS32 Bootloader configuration, but can also be changed during runtime
static u8 lcd_device_num_x;
static u8 lcd_device_num_y;
static u8 lcd_device_height;
static u8 lcd_device_width;

u8 *glcd_font = NULL;  

static u8 first_msg;


// default special character set (vertical bars)
static const u8 charset_vert_bars[8*8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
  0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
};

// alternative character set (horizontal bars)
static const u8 charset_horiz_bars[8*8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00,
  0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00,
  0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00,
  0x00, 0x00, 0x04, 0x06, 0x06, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// this table contains the special chars which should be print
// depending on meter values (16 entries, only 14 are used)
// see also LC_LCD_Print_Meter()
static const u8 hmeter_map[5*16] = {
  1, 0, 0, 0, 0,
  2, 0, 0, 0, 0,
  3, 0, 0, 0, 0,
  3, 1, 0, 0, 0,
  3, 2, 0, 0, 0,
  3, 3, 0, 0, 0,
  3, 3, 1, 0, 0,
  3, 3, 2, 0, 0, 
  3, 3, 3, 0, 0,
  3, 3, 3, 1, 0, 
  3, 3, 3, 2, 0, 
  3, 3, 3, 3, 1,
  3, 3, 3, 3, 2, 
  3, 3, 3, 3, 3, 
  3, 3, 3, 3, 3, // (not used)
  3, 3, 3, 3, 3, // (not used)
};


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the LCD handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // how many devices are configured in the MIOS32 Bootloader?
  lcd_device_num_x = mios32_lcd_parameters.num_x;
  lcd_device_num_y = mios32_lcd_parameters.num_y;

  // graphical LCDs?
  if( MIOS32_LCD_TypeIsGLCD() ) {
    // depending on normal font by default - can change depending on font
    lcd_device_width  = mios32_lcd_parameters.width / 6;
    lcd_device_height = mios32_lcd_parameters.height / 8;
  } else {
    // if only two CLCDs, we assume that the user hasn't changed the configuration
    // and therefore support up to two 2x40 CLCDs (combined to 2x80)
    if( (mios32_lcd_parameters.num_x*mios32_lcd_parameters.num_y) <= 2 ) {
      lcd_device_width  = 40;
      lcd_device_height = 2;    
    } else {
      lcd_device_width  = mios32_lcd_parameters.width;
      lcd_device_height = mios32_lcd_parameters.height;
    }
  }

  // initialize all LCDs (programming_models/traditional/main.c will only initialize the first two)
  int lcd;
  for(lcd=0; lcd<(mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y); ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    if( MIOS32_LCD_Init(0) < 0 ) {
      MIOS32_MIDI_SendDebugMessage("[MBNG_LCD] no response from CLCD #%d.%d\n",
				   (lcd % mios32_lcd_parameters.num_x) + 1,
				   (lcd / mios32_lcd_parameters.num_x) + 1);
    }
  }

  // init LCD
  first_msg = 0; // message will disappear with first item
  MBNG_LCD_FontInit('n');
  MBNG_LCD_Clear();
  MBNG_LCD_CursorSet(0, 0, 0);
  MBNG_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);
  MBNG_LCD_CursorSet(0, 0, 1);
  MBNG_LCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE2);

  MBNG_LCD_SpecialCharsInit(0, 1); // select vertical bars

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! Clears all LCDs
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_Clear(void)
{
  int lcd;
  for(lcd=0; lcd<(mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y); ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_Clear();
  }

  lcd_cursor_device = 0;
  lcd_cursor_x = 0;
  lcd_cursor_y = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Selects a GLCD font
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_FontInit(char font_name)
{
  switch( font_name ) {
  case 'n': glcd_font = (u8 *)GLCD_FONT_NORMAL; break;
  case 's': glcd_font = (u8 *)GLCD_FONT_SMALL; break;
  case 'b': glcd_font = (u8 *)GLCD_FONT_BIG; break;
  case 'k': glcd_font = (u8 *)GLCD_FONT_KNOB_ICONS; break;
  case 'h': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_H; break;
  case 'v': glcd_font = (u8 *)GLCD_FONT_METER_ICONS_V; break;
  default:
    return -1; // unsupported font
  }

  if( glcd_font && MIOS32_LCD_TypeIsGLCD() ) {
    MIOS32_LCD_FontInit(glcd_font);

    lcd_device_width  = mios32_lcd_parameters.width / glcd_font[MIOS32_LCD_FONT_WIDTH_IX];
    // note: lcd_device_height stays at is to improve cursor control!
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the cursor on the given LCD
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_CursorSet(u8 lcd, u16 x, u16 y)
{
  // determine effective x/y depending on bootloader configuration
  u32 full_width = lcd_device_num_x * lcd_device_width;
  u32 full_height = lcd_device_num_y * lcd_device_height;

  // following calculation allows to span the line over multiple LCDs
  // e.g. with lcd_device_num_x=2 and lcd_device_width=40, we are able to
  // - address x=0..79 with lcd=0
  // - or alternatively address x=0..39 with lcd=0 and lcd=1
  lcd_cursor_x = x + (lcd % lcd_device_num_x) * lcd_device_width;
  // same for the Y position
  lcd_cursor_y = y + (lcd / lcd_device_num_x) * lcd_device_height;

  // invalidate cursor if resulting cursor position is out of range
  if( lcd_cursor_x >= full_width || lcd_cursor_y >= full_height ) {
    lcd_cursor_x = 0xffff;
    lcd_cursor_y = 0xffff;
    return -1; // out of range
  }

  // set device & cursor
  lcd_cursor_device = (lcd_cursor_x / lcd_device_width) + (lcd_cursor_y / lcd_device_height) * lcd_device_num_x;
  MIOS32_LCD_DeviceSet(lcd_cursor_device);

  if( MIOS32_LCD_TypeIsGLCD() ) {
    u16 font_width = glcd_font ? glcd_font[MIOS32_LCD_FONT_WIDTH_IX] : 6;
    u16 font_height = 8; // force 8 pixels for better cursor control over Y lines
    MIOS32_LCD_GCursorSet((lcd_cursor_x % lcd_device_width) * font_width, (lcd_cursor_y % lcd_device_height) * font_height);
  } else {
    MIOS32_LCD_CursorSet(lcd_cursor_x % lcd_device_width, lcd_cursor_y % lcd_device_height);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Prints a character and increments the cursor
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintChar(char c)
{
  // determine effective x/y depending on bootloader configuration
  u32 full_width = lcd_device_num_x * lcd_device_width;
  u32 full_height = lcd_device_num_y * lcd_device_height;

  // exit if invalid cursor position
  if( lcd_cursor_x >= full_width || lcd_cursor_y >= full_height )
    return -1;

  // set device and change cursor if required
  u8 device = (lcd_cursor_x / lcd_device_width) + (lcd_cursor_y / lcd_device_height) * lcd_device_num_x;
  if( device != lcd_cursor_device ) {
    lcd_cursor_device = device;
    MIOS32_LCD_DeviceSet(lcd_cursor_device);

    if( MIOS32_LCD_TypeIsGLCD() ) {
      u16 font_width = glcd_font ? glcd_font[MIOS32_LCD_FONT_WIDTH_IX] : 6;
      u16 font_height = 8; // force 8 pixels for better cursor control over Y lines
      MIOS32_LCD_GCursorSet((lcd_cursor_x % lcd_device_width) * font_width, (lcd_cursor_y % lcd_device_height) * font_height);
    } else {
      MIOS32_LCD_CursorSet(lcd_cursor_x % lcd_device_width, lcd_cursor_y % lcd_device_height);
    }
  }

  // print char
  MIOS32_LCD_PrintChar(c);

  // increment cursor
  ++lcd_cursor_x;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a string
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintString(char *str)
{
  while( *str != '\0' ) {
    if( lcd_cursor_x >= (lcd_device_num_x * lcd_device_width) )
      break;
    MBNG_LCD_PrintChar(*str);
    ++str;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a formatted string
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintFormattedString(char *format, ...)
{
  char buffer[128]; // hopefully enough?
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return MBNG_LCD_PrintString(buffer);
}

/////////////////////////////////////////////////////////////////////////////
//! Prints the given number of spaces
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintSpaces(int num)
{
  int i;

  for(i=0; i<num; ++i)
    MBNG_LCD_PrintChar(' ');

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Whenever this function is called, the screen will be cleared once a new
//! message is print
/////////////////////////////////////////////////////////////////////////////
extern s32 MBNG_LCD_ClearScreenOnNextMessage(void)
{
  first_msg = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! prints the string of an item
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintItemLabel(mbng_event_item_t *item)
{
  if( !item->label )
    return -1; // no label
  char *str = item->label;

#if DEBUG_PRINT_ITEM_PERFORMANCE
  MIOS32_STOPWATCH_Reset();
#endif

  if( !first_msg ) {
    first_msg = 1;
    MBNG_LCD_Clear();
  }

  if( *str == '^' ) {
    char *label_str = (char *)MBNG_FILE_L_GetLabel((char *)&str[1], item->value);
    if( label_str != NULL ) {
      str = label_str;
    }
  }

  // GLCD: always start with normal font
  // exception: if label starts with & (font selection)
  u8 current_font = 'n';
  MBNG_LCD_FontInit(current_font);
  if( *str == '&' ) {
    switch( *(++str) ) {
    case '&': MBNG_LCD_PrintChar('&'); break;
    default:
      if( MBNG_LCD_FontInit(*str) >= 0 ) {
	current_font = *str;
      } else {
	//current_font = 'n'; // (redundant)
	//MBNG_LCD_FontInit(current_font);
	MBNG_LCD_PrintChar('&');
	MBNG_LCD_PrintChar(*str);
      }
    }
    ++str;
  }

  // set cursor (depending on font...)
  MBNG_LCD_CursorSet(item->lcd, item->lcd_x, item->lcd_y);

  // string already 0?
  if( *str == 0 )
    return 0; // no error

  char *recursive_str = NULL; // only simple one-level recursion for label strings
  for( ; ; ++str) {
    if( *str == 0 && recursive_str ) {
      str = recursive_str;
      recursive_str = NULL;
    }
    if( *str == 0 )
      break;

    if( *str == '^' ) { // label
      ++str;
      if( *str == '^' || *str == 0 )
	MBNG_LCD_PrintChar('^');
      else if( *str == '#' ) {
	// ignore... this is for label termination without spaces, e.g. "^label^#MyText"
	++str;
      } else {
	char label[9];
	int pos;
	for(pos=0; pos<8 && *str != 0 && *str != '^' && *str != ' '; ++pos, ++str) {
	  label[pos] = *str;
	}
	label[pos] = 0;

	char *label_str = (char *)MBNG_FILE_L_GetLabel(label, item->value);
	if( label_str != NULL ) {
	  recursive_str = str;
	  str = label_str;
	}
      }
    }

    if( *str == '&' ) {
      switch( *(++str) ) {
      case '&': MBNG_LCD_PrintChar('&'); break;
      default:
	if( MBNG_LCD_FontInit(*str) >= 0 ) {
	  current_font = *str;
	} else {
	  current_font = 'n';
	  MBNG_LCD_FontInit(current_font);
	  MBNG_LCD_PrintChar('&');
	  MBNG_LCD_PrintChar(*str);
	}
      }
      ++str;
    }

    if( *str == '@' ) {
      if( str[1] == '@' )
	++str; // print single @
      else if( str[1] == '(' ) {
	// new LCD pos
	char *pos_str = (char *)(str + 2);
	char *next;
	long lcd_num;
	long lcd_x;
	long lcd_y;

	if( (lcd_num = strtol(pos_str, &next, 0)) && lcd_num >= 1 && pos_str != next && next[0] == ':' ) {
	  pos_str = (char *)(next + 1);
	  if( (lcd_x = strtol(pos_str, &next, 0)) && lcd_x >= 1 && pos_str != next && next[0] == ':' ) {
	    pos_str = (char *)(next + 1);
	    if( (lcd_y = strtol(pos_str, &next, 0)) && lcd_y >= 1 && pos_str != next && next[0] == ')' ) {
	      MBNG_LCD_CursorSet(lcd_num-1, lcd_x - 1, lcd_y - 1);
	      str = (char *)&next[1];
	    }
	  }
	}
      }
    }

    // could be 0 meanwhile
    if( *str == 0 ) {
      if( recursive_str )
	continue; // for the case that we've an recursive string - see for() header
      else
	break;
    }

    if( *str != '%' )
      MBNG_LCD_PrintChar(*str);
    else {
      char *format_begin = str;

      ++str;
      if( *str == '%' || *str == 0 )
	MBNG_LCD_PrintChar('%');
      else {
	// alignment, padding, etc...
	if( *str == '-' )
	  ++str;
	if( *str == '0' || *str == '-' )
	  ++str;
	for(; *str != 0 && *str >= '0' && *str <= '9'; ++str);

	// finally check for valid string
	int format_len = str - format_begin + 1;
	if( *str != 0 && format_len < 9 ) {
	  char format[10];
	  strncpy(format, format_begin, format_len);
	  format[format_len] = 0;
	  char *format_type = &format[format_len-1];

	  switch( format[format_len-1] ) {
	  case 'd': // value in various formats
	  case 'x':
	  case 'X':
	  case 'u':
	  case 'c': {
	    MBNG_LCD_PrintFormattedString(format, (int)item->value + item->offset);
	  } break;

	  case 's': { // just print empty string - allows to optimize memory usage for labels, e.g. "%20s"
	    MBNG_LCD_PrintFormattedString(format, "");
	  } break;

	  case 'i': { // ID
	    *format_type = 'd';
	    MBNG_LCD_PrintFormattedString(format, item->id & 0xfff);
	  } break;

	  case 'p': { // Matrix pin number
	    *format_type = 'd';
	    MBNG_LCD_PrintFormattedString(format, item->matrix_pin);
	  } break;

	  case 'e': { // (MIDI) event
	    *format_type = 's';
	    char midi_str[10];
	    if( item->stream_size < 1 ) {
	      sprintf(midi_str, "NoMIDI", midi_str);
	    } else if( item->stream_size < 2 ) {
	      sprintf(midi_str, "%02x%02x  ", item->stream[0], item->value & 0x7f);
	    } else {
	      sprintf(midi_str, "%02x%02x%02x", item->stream[0], item->stream[1], item->value & 0x7f);
	    }
	    MBNG_LCD_PrintFormattedString(format, midi_str);
	  } break;

	  case 'm': { // min value
	    *format_type = 'd';
	    MBNG_LCD_PrintFormattedString(format, item->min);
	  } break;

	  case 'M': { // max value
	    *format_type = 'd';
	    MBNG_LCD_PrintFormattedString(format, item->max);
	  } break;

	  case 'b': { // binary digit
	    *format_type = 'c';
	    int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
	    u8 dout_value = (item->min <= item->max) ? ((item->value - item->min) >= (range/2)) : ((item->value - item->max) >= (range/2));
	    MBNG_LCD_PrintFormattedString(format, dout_value ? '*' : 'o');
	  } break;

	  case 'B': { // vertical bar
	    *format_type = 'c';
	    int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);

	    int normalized_value = (item->min <= item->max) ? (item->value - item->min) : (item->value - item->max);
	    if( normalized_value < 0 )
	      normalized_value = 0;
	    if( normalized_value >= range )
	      normalized_value = range - 1;

	    u8 num_icons = 8; // for CLCD and GLCD default fonts
	    u8 icon_offset = 0;
	    if( current_font == 'k' ) {
	      num_icons = 11;
	      icon_offset = 1;
	    } else if( current_font == 'h' ) {
	      num_icons = 14;
	    } else if( current_font == 'v' ) {
	      num_icons = 14;
	    }

	    char bar;
	    if( range < num_icons ) {
	      if( normalized_value >= range )
		bar = icon_offset + num_icons - 1;
	      else
		bar = icon_offset + ((num_icons * normalized_value) / range);
	    } else if( range == num_icons )
	      bar = icon_offset + normalized_value;
	    else {
	      bar = icon_offset + ((num_icons * normalized_value) / range);
	    }

	    MBNG_LCD_PrintChar(bar);
	  } break;

	  case 'q': { // selected bank
	    *format_type = 'd';
	    MBNG_LCD_PrintFormattedString(format, MBNG_EVENT_SelectedBankGet());
	  } break;

	  case 'C': { // clear screens
	    MBNG_LCD_Clear();
	  } break;

	  default:
	    MBNG_LCD_PrintString(format);
	  }
	}
      }
    }
  }

#if DEBUG_PRINT_ITEM_PERFORMANCE
  u32 cycles = MIOS32_STOPWATCH_ValueGet();
  if( cycles == 0xffffffff )
    DEBUG_MSG("[PERF] overrun!\n");
  else
    DEBUG_MSG("[PERF] %5d.%d mS\n", cycles/10, cycles%10);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! initializes special characters
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_SpecialCharsInit(u8 charset, u8 force)
{
  int lcd;

  // exit if charset already initialized
  if( !force && charset == lcd_charset )
    return 0;

  // else initialize special characters
  lcd_charset = charset;

  if( MIOS32_LCD_TypeIsGLCD() ) {
    // not relevant
  } else {
    for(lcd=0; lcd<2; ++lcd) {
      MIOS32_LCD_DeviceSet(lcd);
      MIOS32_LCD_CursorSet(0, 0);
      switch( charset ) {
      case 1: // horizontal bars
        MIOS32_LCD_SpecialCharsInit((u8 *)charset_horiz_bars);
        break;
      default: // vertical bars
        MIOS32_LCD_SpecialCharsInit((u8 *)charset_vert_bars);
      }
    }
  }

  MIOS32_LCD_DeviceSet(0);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! re-initializes special characters
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_SpecialCharsReInit(void)
{
  return MBNG_LCD_SpecialCharsInit(lcd_charset, 1);
}


//! \}
