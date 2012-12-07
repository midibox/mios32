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

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <buflcd.h>

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_event.h"


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
  BUFLCD_Init(0);

  first_msg = 0; // message will disappear with first item
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
s32 MBNG_LCD_PrintItemLabel(mbng_event_item_t *item, u16 item_value)
{
  if( !item->label )
    return -1; // no label

  if( !first_msg ) {
    first_msg = 1;
    BUFLCD_Clear();
  }

  BUFLCD_CursorSet(item->lcd_pos % 64, item->lcd_pos / 64);

  int min = item->min;
  int max = item->max;
  if( min > max ) { // swap
    int swap = max;
    max = min;
    min = swap;
  }

  char *str = item->label;
  for(; *str != 0; ++str) {
    if( *str != '%' )
      BUFLCD_PrintChar(*str);
    else {
      char *format = str;
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
	if( *str != 0 ) {
	  char memo[2]; // store current char and temporary terminate *format string
	  memo[0] = str[0];
	  memo[1] = str[1];
	  str[1] = 0;

	  switch( *str ) {
	  case 'd': // value in various formats
	  case 'x':
	  case 'X':
	  case 'u':
	  case 'c': {
	    BUFLCD_PrintFormattedString(format, item_value);
	  } break;

	  case 's': { // just print empty string - allows to optimize memory usage for labels, e.g. "%20s"
	    BUFLCD_PrintFormattedString(format, "");
	  } break;

	  case 'i': { // ID
	    *str = 'd';
	    BUFLCD_PrintFormattedString(format, item->id & 0xfff);
	  } break;

	  case 'm': { // min value
	    *str = 'd';
	    BUFLCD_PrintFormattedString(format, item->min);
	  } break;

	  case 'M': { // max value
	    *str = 'd';
	    BUFLCD_PrintFormattedString(format, item->max);
	  } break;

	  case 'b': { // binary digit
	    *str = 'c';
	    if( (max-min) == 1 )
	      BUFLCD_PrintFormattedString(format, (item_value > min) ? '*' : 'o');
	    else
	      BUFLCD_PrintFormattedString(format, (item_value > ((max-min)/2)) ? '*' : 'o');
	  } break;

	  case 'B': { // vertical bar
	    *str = 'c';
	    int range = max - min + 1;
	    char bar = ((8 * item_value) / range); // between 0..7
	    // 0x08 ensures that string won't be terminated.. common CLCDs will output the special char of 0 instead
	    if( bar == 0 )
	      bar = 0x08;
	    BUFLCD_PrintFormattedString(format, bar);
	  } break;

	  default:
	    BUFLCD_PrintString(format);
	  }

	  str[0] = memo[0]; // restore formatted string
	  str[1] = memo[1];
	}
      }
    }
  }

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

