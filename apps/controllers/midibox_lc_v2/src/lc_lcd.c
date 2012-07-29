// $Id$
/*
 * MIDIbox LC V2
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
#include "lc_hwcfg.h"
#include "lc_lcd.h"
#include "lc_vpot.h"
#include "lc_gpc.h"
#include "lc_meters.h"
#include "lc_vpot.h"
#include "lc_leddigits.h"
#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 x; // x position
  u8 y; // y position
  u8 d; // distance between characters (or GLCD: pixels)
  u8 *font; // only relevant for GLCD
  u8 update_req;
} lcd_lay_t;


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 lcd_charset;
static u8 msg_cursor;

static u8 msg_host[128];

// layout elements
static lcd_lay_t lay_host_msg;
static lcd_lay_t lay_status;
static lcd_lay_t lay_smpte_beats;
static lcd_lay_t lay_mtc;
static lcd_lay_t lay_meters;
static lcd_lay_t lay_knobs;
static lcd_lay_t lay_rsm;




// the welcome message          <-------|------|------|-----|------|------|------|------>
static const u8 welcome_msg[] = "---===< Logic Control Emulation is ready.       >===---";

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

// the display mode
static display_mode_t display_mode;

// mapped cursor positions for "extra large spacing"
// used to spread the 55 character host message over the 2*40 LCD screen
#if MBSEQ_HARDWARE_OPTION
static const u8 large_screen_cursor_map[56] = {
  0x00 +  1, 0x00 +  2, 0x00 +  3, 0x00 +  4, 0x00 +  5, 0x00 +  6, 0x00 +  7,
  0x00 + 11, 0x00 + 12, 0x00 + 13, 0x00 + 14, 0x00 + 15, 0x00 + 16, 0x00 + 17,
  0x00 + 21, 0x00 + 22, 0x00 + 23, 0x00 + 24, 0x00 + 25, 0x00 + 26, 0x00 + 27,
  0x00 + 31, 0x00 + 32, 0x00 + 33, 0x00 + 34, 0x00 + 35, 0x00 + 36, 0x00 + 37,

  0x80 +  1, 0x80 +  2, 0x80 +  3, 0x80 +  4, 0x80 +  5, 0x80 +  6, 0x80 +  7,
  0x80 + 11, 0x80 + 12, 0x80 + 13, 0x80 + 14, 0x80 + 15, 0x80 + 16, 0x80 + 17,
  0x80 + 21, 0x80 + 22, 0x80 + 23, 0x80 + 24, 0x80 + 25, 0x80 + 26, 0x80 + 27,
  0x80 + 31, 0x80 + 32, 0x80 + 33, 0x80 + 34, 0x80 + 35, 0x80 + 36, 0x80 + 37,
  };
#else
static const u8 large_screen_cursor_map[56] = {
  0x00 +  7, 0x00 +  8, 0x00 +  9, 0x00 + 10, 0x00 + 11, 0x00 + 12, 0x00 + 13,
  0x00 + 16, 0x00 + 17, 0x00 + 18, 0x00 + 19, 0x00 + 20, 0x00 + 21, 0x00 + 22,
  0x00 + 25, 0x00 + 26, 0x00 + 27, 0x00 + 28, 0x00 + 29, 0x00 + 30, 0x00 + 31,
  0x00 + 34, 0x00 + 35, 0x00 + 36, 0x00 + 37, 0x00 + 38, 0x00 + 39,

  0x80 +  0, 0x80 +  1, 0x80 +  2, 0x80 +  3, 0x80 +  4, 0x80 +  5, 0x80 +  6,
  0x80 +  9, 0x80 + 10, 0x80 + 11, 0x80 + 12, 0x80 + 13, 0x80 + 14, 0x80 + 15,
  0x80 + 18, 0x80 + 19, 0x80 + 20, 0x80 + 21, 0x80 + 22, 0x80 + 23, 0x80 + 24,
  0x80 + 27, 0x80 + 28, 0x80 + 29, 0x80 + 30, 0x80 + 31, 0x80 + 32, 0x80 + 33, 0x80 + 34,
  };
#endif  


/////////////////////////////////////////////////////////////////////////////
// This function initializes the LCD
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Init(u32 mode)
{
  int i;

  // init display mode
  if( MIOS32_LCD_TypeIsGLCD() ) {
    LC_LCD_DisplayPageSet(lc_hwcfg_display_cfg.initial_page_glcd);
  } else {
    LC_LCD_DisplayPageSet(lc_hwcfg_display_cfg.initial_page_clcd);
  }

  lcd_charset = 0xff; // force refresh
  LC_LCD_SpecialCharsInit(1); // select horizontal bars

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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the current display page
/////////////////////////////////////////////////////////////////////////////
u8 LC_LCD_DisplayPageGet(void)
{
  return display_mode.PAGE;
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the display page and requests a screen refresh
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_DisplayPageSet(u8 page)
{
  // set page and request update of all elements
  display_mode.PAGE = page;
  lay_host_msg.update_req = 1;
  lay_meters.update_req = 0xff;
  lay_knobs.update_req = 0xff;
  lay_mtc.update_req = 1;
  lay_status.update_req = 1;
  lay_smpte_beats.update_req = 1;
  lay_rsm.update_req = 1;

  // clear screen
  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();

  if( !MIOS32_LCD_TypeIsGLCD() ) {
    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_Clear();
    MIOS32_LCD_DeviceSet(0);
  }

  // set layout
  // possible x coordinates: x = 0..39 (0xff to disable message)
  // possible y coordinates: y = n * 0x40 - see below
  //    0: first LCD, first line
  //    1: first LCD, second line
  //    2: second LCD, first line
  //    3: second LCD, second line
  switch( page ) {
    case 0:

      if( MIOS32_LCD_TypeIsGLCD() ) {

	// GLCD

	// status digits at 12/4 (16 pixels between chars)
	lay_status.font = (u8 *)GLCD_FONT_BIG;
	lay_status.x = 12; lay_status.y = 4; lay_status.d = 16;

	// print "SMPTE/BEATS" at 56/5
	lay_smpte_beats.font = (u8 *)GLCD_FONT_SMALL;
	lay_smpte_beats.x = 56; lay_smpte_beats.y = 5; lay_smpte_beats.d = 5;

	// print MTC digits at 70/4 (12 pixels between chars)
	lay_mtc.font = (u8 *)GLCD_FONT_BIG;
	lay_mtc.x = 70; lay_mtc.y = 4; lay_mtc.d = 12;

	// print host message at 2/1 (4 pixels between chars)
	lay_host_msg.font = (u8 *)GLCD_FONT_SMALL;
	lay_host_msg.x = 2; lay_host_msg.y = 1; lay_host_msg.d = 4;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 1;

	// don't print meters
	lay_meters.font = NULL;
	lay_meters.x = 0; lay_meters.y = 0; lay_meters.d = 0;

	// don't print rec/solo/mute status
	lay_rsm.font = NULL;
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;

      } else {

	// CLCD:

	// status digits at 38/2
	lay_status.x = 38; lay_status.y = 2; lay_status.d = 1;

	// print "SMPTE/BEATS" at 28/2
	lay_smpte_beats.x = 28; lay_smpte_beats.y = 2; lay_smpte_beats.d = 1;

	// print MTC digits at 27/3
	lay_mtc.x = 27; lay_mtc.y = 3; lay_mtc.d = 1;

	// print host message at 12/0 (normal spacing, both lines)
	lay_host_msg.x = 12; lay_host_msg.y = 0; lay_host_msg.d = 1;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 1;

	// don't print meters
	lay_meters.x = 0; lay_meters.y = 0; lay_meters.d = 0;
	LC_LCD_SpecialCharsInit(1); // select horizontal bars

	// print rec/solo/mute status at 0/0
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 1;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;
      }
      break;

    case 1:

      if( MIOS32_LCD_TypeIsGLCD() ) {

	// GLCD

	// status digits at 12/4 (16 pixels between chars)
	lay_status.font = (u8 *)GLCD_FONT_BIG;
	lay_status.x = 12; lay_status.y = 4; lay_status.d = 16;

	// print "SMPTE/BEATS" at 56/5
	lay_smpte_beats.font = (u8 *)GLCD_FONT_SMALL;
	lay_smpte_beats.x = 56; lay_smpte_beats.y = 5; lay_smpte_beats.d = 5;

	// print MTC digits at 70/4 (12 pixels between chars)
	lay_mtc.font = (u8 *)GLCD_FONT_BIG;
	lay_mtc.x = 70; lay_mtc.y = 4; lay_mtc.d = 12;

	// print host message at 2/1 (4 pixels between chars)
	lay_host_msg.font = (u8 *)GLCD_FONT_SMALL;
	lay_host_msg.x = 2; lay_host_msg.y = 1; lay_host_msg.d = 4;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 1;

	// print horizontal meters at position 10/0 (28 pixels between icons)
	lay_meters.font = (u8 *)GLCD_FONT_METER_ICONS_H;
	lay_meters.x = 10; lay_meters.y = 0; lay_meters.d = 28;

	// don't print rec/solo/mute status
	lay_rsm.font = NULL;
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;

      } else {

	// status digits at 38/2
	lay_status.x = 38; lay_status.y = 2; lay_status.d = 1;

	// print "SMPTE/BEATS" at 28/2
	lay_smpte_beats.x = 28; lay_smpte_beats.y = 2; lay_smpte_beats.d = 1;

	// print MTC digits at 27/3
	lay_mtc.x = 27; lay_mtc.y = 3; lay_mtc.d = 1;

	// print host message at 12/0 (normal spacing, only first line)
	lay_host_msg.x = 12; lay_host_msg.y = 0; lay_host_msg.d = 1;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 0;

	// print meters at position 12/1 (distance between meters: 7)
	lay_meters.x = 12; lay_meters.y = 1; lay_meters.d = 7;
	LC_LCD_SpecialCharsInit(1); // select horizontal bars

	// print rec/solo/mute status at 0/0
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 1;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;
      }
      break;

    case 2:

      if( MIOS32_LCD_TypeIsGLCD() ) {

	// GLCD

	// status digits at 12/7 (8 pixels between chars)
	lay_status.font = (u8 *)GLCD_FONT_NORMAL;
	lay_status.x = 12; lay_status.y = 7; lay_status.d = 8;

	// print "SMPTE/BEATS" at 92/7
	lay_smpte_beats.font = (u8 *)GLCD_FONT_NORMAL;
	lay_smpte_beats.x = 92; lay_smpte_beats.y = 7; lay_smpte_beats.d = 6;

	// print MTC digits at 124/7 (8 pixels between chars)
	lay_mtc.font = (u8 *)GLCD_FONT_NORMAL;
	lay_mtc.x = 124; lay_mtc.y = 7; lay_mtc.d = 8;

	// print host message at 2/1 (4 pixels between chars)
	lay_host_msg.font = (u8 *)GLCD_FONT_SMALL;
	lay_host_msg.x = 2; lay_host_msg.y = 1; lay_host_msg.d = 4;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 1;

	// print vertical meters at position 18/3 (28 pixels between icons)
	lay_meters.font = (u8 *)GLCD_FONT_METER_ICONS_V;
	lay_meters.x = 18; lay_meters.y = 3; lay_meters.d = 28;

	// don't print rec/solo/mute status
	lay_rsm.font = NULL;
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;

      } else {

	// CLCD

	// don't print status digits
	lay_status.x = 0; lay_status.y = 0; lay_status.d = 0;

	// don't print "SMPTE/BEATS"
	lay_smpte_beats.x = 0; lay_smpte_beats.y = 0; lay_smpte_beats.d = 0;

	// don't print MTC digits
	lay_mtc.x = 0; lay_mtc.y = 0; lay_mtc.d = 0;

	// print host message at 0/0 (large spacing, both lines)
	lay_host_msg.x = 0; lay_host_msg.y = 0; lay_host_msg.d = 1;
	display_mode.LARGE_SCREEN = 1;
	display_mode.SECOND_LINE = 1;

	// don't print meters
	lay_meters.x = 0; lay_meters.y = 0; lay_meters.d = 0;
	LC_LCD_SpecialCharsInit(1); // select horizontal bars

	// don't print rec/solo/mute
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;
      }
      break;

    default: // 3

      if( MIOS32_LCD_TypeIsGLCD() ) {

	// GLCD

	// status digits at 12/7 (8 pixels between chars)
	lay_status.font = (u8 *)GLCD_FONT_NORMAL;
	lay_status.x = 12; lay_status.y = 7; lay_status.d = 8;

	// print "SMPTE/BEATS" at 86/7
	lay_smpte_beats.font = (u8 *)GLCD_FONT_NORMAL;
	lay_smpte_beats.x = 86; lay_smpte_beats.y = 7; lay_smpte_beats.d = 6;

	// print MTC digits at 124/7 (8 pixels between chars)
	lay_mtc.font = (u8 *)GLCD_FONT_NORMAL;
	lay_mtc.x = 124; lay_mtc.y = 7; lay_mtc.d = 8;

	// print host message at 2/1 (4 pixels between chars)
	lay_host_msg.font = (u8 *)GLCD_FONT_SMALL;
	lay_host_msg.x = 2; lay_host_msg.y = 1; lay_host_msg.d = 4;
	display_mode.LARGE_SCREEN = 0;
	display_mode.SECOND_LINE = 1;

	// print horizontal meters at position 10/3 (28 pixels between icons)
	lay_meters.font = (u8 *)GLCD_FONT_METER_ICONS_H;
	lay_meters.x = 10; lay_meters.y = 3; lay_meters.d = 28;

	// don't print rec/solo/mute status
	lay_rsm.font = NULL;
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// print knobs at position 8/4 (28 pixels between icons)
	lay_knobs.font = (u8 *)GLCD_FONT_KNOB_ICONS;
	lay_knobs.x = 8; lay_knobs.y = 4; lay_knobs.d = 28;

      } else {

	// CLCD

	// don't print status digits
	lay_status.x = 0; lay_status.y = 0; lay_status.d = 0;

	// don't print "SMPTE/BEATS"
	lay_smpte_beats.x = 0; lay_smpte_beats.y = 0; lay_smpte_beats.d = 0;

	// don't print MTC digits
	lay_mtc.x = 0; lay_mtc.y = 0; lay_mtc.d = 0;

	// print host message at 0/0 (large spacing, only one line)
	lay_host_msg.x = 0; lay_host_msg.y = 0; lay_host_msg.d = 1;
	display_mode.LARGE_SCREEN = 1;
	display_mode.SECOND_LINE = 0;

	// print meters at position 2/1 (distance between meters: 10)
	lay_meters.x = 2; lay_meters.y = 1; lay_meters.d = 10;
	LC_LCD_SpecialCharsInit(1); // select horizontal bars

	// don't print rec/solo/mute
	lay_rsm.x = 0; lay_rsm.y = 0; lay_rsm.d = 0;

	// don't print knobs
	lay_knobs.font = NULL;
	lay_knobs.x = 0; lay_knobs.y = 0; lay_knobs.d = 0;
      }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the LCD
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update(u8 force)
{
  // update screen whenever required (or forced)


  // host message
  if( lay_host_msg.d ) {
    MIOS32_IRQ_Disable();
    u8 local_force = force || lay_host_msg.update_req;
    lay_host_msg.update_req = 0;
    MIOS32_IRQ_Enable();

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_host_msg.font )
	MIOS32_LCD_FontInit(lay_host_msg.font);
    }

    int next_x = -1;
    int next_y = -1;
    int x, y;
    int lcd_x, lcd_y;

    for(y=0; y<2; ++y) {
      u8 *msg = lc_flags.GPC_SEL ? (u8 *)&gpc_msg[y*0x40] : (u8 *)&msg_host[y*0x40];

      for(x=0; x<lc_hwcfg_display_cfg.num_columns; ++x, ++msg) {

	if( local_force || !(*msg & 0x80) ) {
	  if( display_mode.LARGE_SCREEN ) {
	    lcd_x = lay_host_msg.x + (large_screen_cursor_map[x] & 0x7f) + ((large_screen_cursor_map[x] & 0x80) ? 40 : 0);
	    lcd_y = lay_host_msg.y + y;
	  } else {
	    lcd_x = lay_host_msg.x + x;
	    lcd_y = lay_host_msg.y + y;
	  }

	  if( lcd_x != next_x || lcd_y != next_y ) {
	    if( MIOS32_LCD_TypeIsGLCD() ) {
	      MIOS32_LCD_CursorSet(lcd_x, lcd_y);
	    } else {
	      MIOS32_LCD_DeviceSet((lcd_x >= 40) ? 1 : 0);
	      MIOS32_LCD_CursorSet(lcd_x % 40, lcd_y);
	    }
	  }
	  MIOS32_LCD_PrintChar(*msg & 0x7f);
	  *msg |= 0x80;
	  next_y = lcd_y;
	  next_x = lcd_x + 1;

	  if( !MIOS32_LCD_TypeIsGLCD() ) {
	    // for 2 * 2x40 LCDs: ensure that cursor is set when we reach the second half
	    if( next_x == 40 )
	      next_x = -1;
	  }
	}
      }
    }
  }



  // meters
  if( lay_meters.d && (lay_meters.update_req || force) ) {
    u8 meter;

    // done for each single meter
    // lay_meters.update_req = 0;

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_meters.font )
	MIOS32_LCD_FontInit(lay_meters.font);
    }

    for(meter=0; meter<8; ++meter) {
      if( force || (lay_meters.update_req & (1 << meter)) ) {
	int i;

	MIOS32_IRQ_Disable();
	lay_meters.update_req &= ~(1 << meter);
	MIOS32_IRQ_Enable();

	u8 level = LC_METERS_LevelGet(meter);

	if( MIOS32_LCD_TypeIsGLCD() ) {
	  // set cursor depending on layout constraints
	  LC_LCD_PhysCursorSet(lay_meters.x + meter * lay_meters.d, lay_meters.y);
	  // print icon (if bit 7 is set: overrun, add 14)
	  MIOS32_LCD_PrintChar((level & 0x0f) + ((level & 0x80) ? 14 : 0));
	} else {
	  // set cursor depending on layout constraints
	  int x = lay_meters.x + meter * lay_meters.d;
	  int y = lay_meters.y;

	  if( x >= 40 ) {
	    x %= 40;
	    y += 2;
	  }
	  LC_LCD_PhysCursorSet(x, y);

	  // print string
	  int map_offset = 5 * (level & 0x7f);
	  for(i=0; i<5; ++i)
	    MIOS32_LCD_PrintChar(hmeter_map[map_offset + i]);

	  // print overrun flag if bit 7 of level is set, else space
	  MIOS32_LCD_PrintChar(level & (1 << 7) ? 0x04 : ' ');
	}
      }
    }
  }



  // SMPTE/Beat indiciator
  if( lay_smpte_beats.d && (lay_smpte_beats.update_req || force) ) {
    lay_smpte_beats.update_req = 0;

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_smpte_beats.font )
	MIOS32_LCD_FontInit(lay_smpte_beats.font);
    }

    // set cursor depending on layout constraints
    LC_LCD_PhysCursorSet(lay_smpte_beats.x, lay_smpte_beats.y);

    // print SMPTE or BEATS
    MIOS32_LCD_PrintString(display_mode.SMPTE_VIEW ? "SMPTE" : "BEATS");
  }



  // MTC
  if( lay_mtc.d && (lay_mtc.update_req || force) ) {
    lay_mtc.update_req = 0;

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_mtc.font )
	MIOS32_LCD_FontInit(lay_mtc.font);
    }

    // print a space at first position (will be overwritten in BEATS mode)

    // print digits and colons
    if( display_mode.SMPTE_VIEW ) { // SMPTE View
      LC_LCD_PhysCursorSet(lay_mtc.x + 0*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(' ');
      LC_LCD_PhysCursorSet(lay_mtc.x + 1*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[0] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 2*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[1] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 3*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[2] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 4*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':');
      LC_LCD_PhysCursorSet(lay_mtc.x + 5*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[3] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 6*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[4] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 7*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':');
      LC_LCD_PhysCursorSet(lay_mtc.x + 8*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[5] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 9*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[6] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 10*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':'); // digit 7 not used
      LC_LCD_PhysCursorSet(lay_mtc.x + 11*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[8] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 12*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[9] & 0x3f) ^ 0x20) + 0x20);
    } else { // Beats View
      LC_LCD_PhysCursorSet(lay_mtc.x + 0*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[0] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 1*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[1] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 2*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[2] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 3*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':');
      LC_LCD_PhysCursorSet(lay_mtc.x + 4*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[3] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 5*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[4] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 6*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':');
      LC_LCD_PhysCursorSet(lay_mtc.x + 7*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[5] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 8*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[6] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 9*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(':');
      LC_LCD_PhysCursorSet(lay_mtc.x + 10*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[7] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 11*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[8] & 0x3f) ^ 0x20) + 0x20);
      LC_LCD_PhysCursorSet(lay_mtc.x + 12*lay_mtc.d, lay_mtc.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_mtc[9] & 0x3f) ^ 0x20) + 0x20);
    }
  }


  // Status Digits
  if( lay_status.d && (lay_status.update_req || force) ) {
    int i;

    lay_status.update_req = 0;
    
    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_status.font )
	MIOS32_LCD_FontInit(lay_status.font);
    }

    // print digits
    for(i=0; i<2; ++i) {
      LC_LCD_PhysCursorSet(lay_status.x + i*lay_status.d, lay_status.y);
      MIOS32_LCD_PrintChar(((lc_leddigits_status[i] & 0x3f) ^ 0x20) + 0x20);
    }
  }


  // Rec/Solo/Mute Status
  if( lay_rsm.d && (lay_rsm.update_req || force) ) {
    lay_rsm.update_req = 0;

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_rsm.font )
	MIOS32_LCD_FontInit(lay_rsm.font);
    }
  }


  // Knobs
  if( lay_knobs.d && (lay_knobs.update_req || force) ) {
    u8 knob;

    // done for each single knob
    // lay_knobs.update_req = 0;

    if( MIOS32_LCD_TypeIsGLCD() ) {
      if( lay_knobs.font )
	MIOS32_LCD_FontInit(lay_knobs.font);
    }

    for(knob=0; knob<8; ++knob) {
      if( force || (lay_knobs.update_req & (1 << knob)) ) {
	int i;

	MIOS32_IRQ_Disable();
	lay_knobs.update_req &= ~(1 << knob);
	MIOS32_IRQ_Enable();

	u8 level = (lc_flags.GPC_SEL ? (LC_GPC_VPotValueGet(knob) >> 3) : LC_VPOT_ValueGet(knob)) & 0x0f; // 16 icons available

	if( MIOS32_LCD_TypeIsGLCD() ) {
	  // set cursor depending on layout constraints
	  LC_LCD_PhysCursorSet(lay_knobs.x + knob * lay_knobs.d, lay_knobs.y);
	  // print icon (if bit 7 is set: overrun, add 14)
	  MIOS32_LCD_PrintChar((level & 0x0f) + ((level & 0x80) ? 14 : 0));
	} else {
	  // set cursor depending on layout constraints
	  LC_LCD_PhysCursorSet(lay_knobs.x + knob * lay_knobs.d, lay_knobs.y);

	  // print string
	  int map_offset = 5 * (level & 0x7f);
	  for(i=0; i<5; ++i)
	    MIOS32_LCD_PrintChar(hmeter_map[map_offset + i]);

	  // print overrun flag if bit 7 of level is set, else space
	  MIOS32_LCD_PrintChar(level & (1 << 7) ? 0x04 : ' ');
	}
      }
    }
  }



  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This functions sets the cursor with x/y coordinate to the physical
// position (x, y, first or second display)
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_PhysCursorSet(u8 cursor_pos_x, u8 cursor_pos_y)
{
  if( MIOS32_LCD_TypeIsGLCD() ) {
    return MIOS32_LCD_GCursorSet(cursor_pos_x, cursor_pos_y*8);
  } else {
    MIOS32_LCD_DeviceSet((cursor_pos_y >= 2) ? 1 : 0);
    return MIOS32_LCD_CursorSet(cursor_pos_x, cursor_pos_y % 2);
  }
}


/////////////////////////////////////////////////////////////////////////////
// this function sets the LCD cursor to the msg_cursor position
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Msg_CursorSet(u8 cursor_pos)
{
  msg_cursor = cursor_pos;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// this function returns the current cursor value
/////////////////////////////////////////////////////////////////////////////
u8 LC_LCD_Msg_CursorGet(void)
{
  return msg_cursor;
}

/////////////////////////////////////////////////////////////////////////////
// stores a character in msg_host at a given cursor position and prints it
// if in HOST mode (!gpc)
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Msg_PrintHost(char c)
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
  lay_host_msg.update_req = 1; // request update

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// initializes special characters
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_SpecialCharsInit(u8 charset)
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
// requests update of host message
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_HostMsg(void)
{
  lay_host_msg.update_req = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of a meter
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_Meter(u8 meter)
{
  MIOS32_IRQ_Disable();
  lay_meters.update_req |= (1 << meter);
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of a knob
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_Knob(u8 knob)
{
  MIOS32_IRQ_Disable();
  lay_knobs.update_req |= (1 << knob);
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of MTC digits
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_MTC(void)
{
  lay_mtc.update_req = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of SMPTE/Beats indicator
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_SMPTE_Beats(void)
{
  lay_smpte_beats.update_req = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of status digits
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_Status(void)
{
  lay_status.update_req = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// requests update of Rec/Solo/Mute status
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_Update_RSM(void)
{
  lay_rsm.update_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_MIDI_Received() in lc_midi.c when a 
// Note event has been received (which updates the LED status)
// Here we can do some additional parsing for LCD messages
/////////////////////////////////////////////////////////////////////////////
s32 LC_LCD_LEDStatusUpdate(u8 evnt1, u8 evnt2)
{
  // SMPTE status:
  if( evnt1 == 0x71 || evnt1 == 0x72 ) {
    display_mode.SMPTE_VIEW = ((evnt1 == 0x71 && evnt2 != 0x00) || (evnt1 == 0x72 && evnt2 == 0x00)) ? 1 : 0;
    lay_smpte_beats.update_req = 1;
    lay_mtc.update_req = 1;
  }

  return 0; // no error
}
