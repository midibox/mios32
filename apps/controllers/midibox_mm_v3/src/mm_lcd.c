// $Id$
/*
 * MIDIbox MM V3
 * LCD Access Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "mm_hwcfg.h"
#include "mm_lcd.h"
#include "mm_vpot.h"
#include "mm_gpc.h"
#include "mm_vpot.h"
#include "mm_leddigits.h"
#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_charset;
static u8 msg_cursor;

static u8 msg_host[128];

// the welcome message              <---------------------------------------->
const unsigned char welcome_msg[] = "---===< Motormix Emulation ready >===---";

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


/////////////////////////////////////////////////////////////////////////////
// This function initializes the LCD
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_Init(u32 mode)
{
  int i;

  lcd_charset = 0xff; // force refresh
  MM_LCD_SpecialCharsInit(0); // select vertical bars

  // initial cursor
  msg_cursor = 0;

  // initialize msg array
  for(i=0; i<0x80; ++i) {
    msg_host[i] = ' ';
  }

  // copy welcome message to host buffer
  for(i=0; welcome_msg[i] != 0; ++i ) {
    msg_host[i] = welcome_msg[i];
  }

  // clear screen
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();

  if( !MIOS32_LCD_TypeIsGLCD() ) {
    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_Clear();
    MIOS32_LCD_DeviceSet(0);
  } else {
    MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }

  // request display update
  mm_flags.MSG_UPDATE_REQ = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the LCD
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_Update(u8 force)
{
  // update screen whenever required (or forced)
  if( !mm_flags.MSG_UPDATE_REQ && !force )
    return 0; // no update requested

  mm_flags.MSG_UPDATE_REQ = 0;

  u8 *msg = mm_flags.GPC_SEL ? (u8 *)&gpc_msg[0] : (u8 *)&msg_host[0];

  // set msg_cursor to first line and update cursor position
  MIOS32_LCD_CursorSet(0, 0);

  // print first line
  int i;
  for(i=0x00; i<0x00+40; ++i) // 1st line of host message
    MIOS32_LCD_PrintChar(msg[i]);

  // set msg_cursor to second line and update cursor position
  MIOS32_LCD_CursorSet(0, 1);

  // print second line
  if( mm_flags.PRINT_POINTERS ) {
    for(i=0; i<8; ++i)
      MM_LCD_PointerPrint(i);
  } else {
    for(i=0x40; i<0x40+40; ++i) // 2nd line of host message
      MIOS32_LCD_PrintChar(msg[i]);
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// this function sets the LCD cursor to the msg_cursor position
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_Msg_CursorSet(u8 cursor_pos)
{
  msg_cursor = cursor_pos;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// this function returns the current cursor value
/////////////////////////////////////////////////////////////////////////////
u8 MM_LCD_Msg_CursorGet(void)
{
  return msg_cursor;
}

/////////////////////////////////////////////////////////////////////////////
// this function is called when a character should be print to the host message section
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_Msg_PrintChar(char c)
{
  static u8 first_host_char = 0;

  if( !first_host_char ) {
    first_host_char = 1;
    // clear welcome message
    int i;
    for(i=0; i<0x80; ++i) {
      msg_host[i] = ' ';
    }
  }

  if( msg_cursor >= 128 )
    return -1; // invalid pos

  msg_host[msg_cursor] = c;    // save character
  ++msg_cursor;                // increment message cursor
  mm_flags.MSG_UPDATE_REQ = 1; // request update

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// sets a pointer value
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_PointerInit(u8 type)
{
  MM_VPOT_DisplayTypeSet(type);
  MM_LCD_SpecialCharsInit(type);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// sets a pointer value
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_PointerSet(u8 pointer, u8 value)
{
  // set VPOT value
  MM_VPOT_ValueSet(pointer, value);

  // update screen if pointers are not printed yet
  if( !mm_flags.PRINT_POINTERS ) {
    mm_flags.PRINT_POINTERS = 1;
  }

  mm_flags.MSG_UPDATE_REQ = 1; // request update

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// prints a pointer when not in GPC mode
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_PointerPrint(u8 pointer)
{
  unsigned char value;

  if( mm_flags.GPC_SEL )
    return 0;

  // get value
  value = MM_VPOT_ValueGet(pointer);

  // set cursor
  MIOS32_LCD_CursorSet(pointer*5, 1);

  // print value depending on display and pointer type
  if( MIOS32_LCD_TypeIsGLCD() ) {
    // graphical LCD - here we could add some nice graphic icons
    // currently we just only print a number and a vertical bar...
    MIOS32_LCD_PrintFormattedString("%3d%c ", value, (value >> 4) & 7);
  } else {
    // character LCD
    switch( MM_VPOT_DisplayTypeGet() ) {
      case 0: // "Left justified horizontal bar graph (Aux pots)"
      case 1: // "Centered horizontal Bar graph"
      case 2: // "Right justified horizontal Bar graph"
      case 3: // "Single verticle line (PAN or PAN R)"
      case 4: // "Left justified double verticle line"
      case 5: // "Centered Q spreading bar (Filter Q)"
      case 6: // "Ascending bar graph (Channel Meters)"
      case 7: // "Descending bar graph (Gaon reduction)"
	// different pointer types not implemented yet
	MIOS32_LCD_PrintFormattedString("%3d%c ", value, (value >> 4) & 7);
	break;

      default:
	MIOS32_LCD_PrintFormattedString("%3d%c ", value, (value >> 4) & 7);
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// initializes special characters
/////////////////////////////////////////////////////////////////////////////
s32 MM_LCD_SpecialCharsInit(u8 charset)
{
  int lcd;

  // exit if charset already initialized
  if( charset == lcd_charset )
    return 0;

  // else initialize special characters
  lcd_charset = charset;

  if( MIOS32_LCD_TypeIsGLCD() ) {
    // not relevant
  } else {
    for(lcd=0; lcd<2; ++lcd) {
      MIOS32_LCD_DeviceSet(lcd);
      MIOS32_LCD_CursorSet(0, 0);

//      switch( charset ) {
//      case 1: // horizontal bars
//	MIOS32_LCD_SpecialCharsInit((u8 *)charset_horiz_bars);
//	break;
//      default: // vertical bars
	MIOS32_LCD_SpecialCharsInit((u8 *)charset_vert_bars);
//      }
    }
  }

  MIOS32_LCD_DeviceSet(0);

  return 0; // no error
}
