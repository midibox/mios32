// $Id$
/*
 * LCD access functions for MIDIbox NG
 *
 * ==========================================================================
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
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <buflcd.h>
#include <glcd_font.h>

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_file_l.h"
#include "mbng_event.h"
#include "mbng_patch.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_charset;

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
// This function initializes the LCD handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // init buffered LCD output
  BUFLCD_Init(1); // without clear - we want to initialize the layout first

  // LCD layout
  if( MIOS32_LCD_TypeIsGLCD() ) {
    // TODO
  } else {
    BUFLCD_ColumnsPerDeviceSet(40);
    BUFLCD_NumLinesSet(2);
  }

  first_msg = 0; // message will disappear with first item
  BUFLCD_Clear();
  BUFLCD_CursorSet(0, 0);
  BUFLCD_PrintString("Welcome to");
  BUFLCD_CursorSet(0, 1);
  BUFLCD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);

  MBNG_LCD_SpecialCharsInit(0, 1); // select vertical bars

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// transfers the buffer to LCDs
// \param[in] force if != 0, it is ensured that the whole screen will be refreshed, regardless
// if characters have changed or not
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_Update(u8 force)
{
  return BUFLCD_Update(force);
}


/////////////////////////////////////////////////////////////////////////////
// prints the string of an item
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_PrintItemLabel(mbng_event_item_t *item)
{
  if( !item->label )
    return -1; // no label

#if DEBUG_PRINT_ITEM_PERFORMANCE
  MIOS32_STOPWATCH_Reset();
#endif

  // GLCD: always start with normal font
  BUFLCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  u8 current_font = 'n';

  if( !first_msg ) {
    first_msg = 1;
    BUFLCD_Clear();
  }

  BUFLCD_CursorSet(item->lcd*BUFLCD_ColumnsPerDeviceGet() + (item->lcd_pos % 64), item->lcd_pos / 64);

  char *str = item->label;

  if( *str == '^' ) {
    char *label_str = (char *)MBNG_FILE_L_GetLabel((char *)&str[1], item->value);
    if( label_str != NULL ) {
      str = label_str;
    }
  }
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
	BUFLCD_PrintChar('^');
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
      case '&': BUFLCD_PrintChar('&'); break;
      case 'n': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_NORMAL); break;
      case 's': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_SMALL); break;
      case 'b': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_BIG); break;
      case 'k': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_KNOB_ICONS); break;
      case 'h': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_H); break;
      case 'v': current_font = *str; BUFLCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_V); break;
      default:
	current_font = 'n';
	BUFLCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	BUFLCD_PrintChar('&');
	BUFLCD_PrintChar(*str);
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
	      BUFLCD_CursorSet((lcd_num-1)*BUFLCD_ColumnsPerDeviceGet() + (lcd_x-1), lcd_y-1);
	      str = (char *)&next[1];
	    }
	  }
	}
      }
    }

    // could be 0 meanwhile
    if( *str == 0 )
      continue; // for the case that we've an recursive string - see for() header

    if( *str != '%' )
      BUFLCD_PrintChar(*str);
    else {
      char *format_begin = str;

      ++str;
      if( *str == '%' || *str == 0 )
	BUFLCD_PrintChar('%');
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
	    BUFLCD_PrintFormattedString(format, (int)item->value + item->offset);
	  } break;

	  case 's': { // just print empty string - allows to optimize memory usage for labels, e.g. "%20s"
	    BUFLCD_PrintFormattedString(format, "");
	  } break;

	  case 'i': { // ID
	    *format_type = 'd';
	    BUFLCD_PrintFormattedString(format, item->id & 0xfff);
	  } break;

	  case 'p': { // Matrix pin number
	    *format_type = 'd';
	    BUFLCD_PrintFormattedString(format, item->matrix_pin);
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
	    BUFLCD_PrintFormattedString(format, midi_str);
	  } break;

	  case 'm': { // min value
	    *format_type = 'd';
	    BUFLCD_PrintFormattedString(format, item->min);
	  } break;

	  case 'M': { // max value
	    *format_type = 'd';
	    BUFLCD_PrintFormattedString(format, item->max);
	  } break;

	  case 'b': { // binary digit
	    *format_type = 'c';
	    int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
	    u8 dout_value = (item->min <= item->max) ? ((item->value - item->min) >= (range/2)) : ((item->value - item->max) >= (range/2));
	    BUFLCD_PrintFormattedString(format, dout_value ? '*' : 'o');
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

	    BUFLCD_PrintChar(bar);
	  } break;

	  case 'q': { // selected bank
	    *format_type = 'd';
	    BUFLCD_PrintFormattedString(format, MBNG_EVENT_SelectedBankGet());
	  } break;

	  case 'C': { // clear screens
	    BUFLCD_Clear();
	  } break;

	  default:
	    BUFLCD_PrintString(format);
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
// initializes special characters
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
// re-initializes special characters
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_LCD_SpecialCharsReInit(void)
{
  return MBNG_LCD_SpecialCharsInit(lcd_charset, 1);
}

