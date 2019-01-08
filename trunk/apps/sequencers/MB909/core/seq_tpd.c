// $Id: seq_tpd.c 1911 2014-01-02 15:40:40Z tk $
/*
 * Routines for relative track position matrix LED display (by hawkeye)
 *
 * ==========================================================================
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
#include <string.h>
#include <glcd_font.h>

#include "seq_tpd.h"
#include "seq_hwcfg.h"
#include "seq_core.h"
#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_led.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_tpd_mode_t tpd_mode;

#define TPD_TEXT_LENGTH 41
static char tpd_scroll_text[TPD_TEXT_LENGTH];

#define TPD_NUM_COL_SR 2 // left/right half
#define TPD_NUM_ROW_SR 2 // green/red colour
#define TPD_NUM_ROWS   8 // don't change

static u8 tpd_display[TPD_NUM_COL_SR][TPD_NUM_ROW_SR][TPD_NUM_ROWS];

#define TPD_GREEN 0 // green layer of TPD_NUM_ROW_SR dimension
#define TPD_RED   1 // red layer of TPD_NUM_ROW_SR dimension


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_TPD_ScrollTextHandler(void);


/////////////////////////////////////////////////////////////////////////////
// Init function
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_Init(u32 mode)
{
  // initial mode
  tpd_mode = SEQ_TPD_Mode_PosAndTrack;

  SEQ_TPD_PrintString(MIOS32_LCD_BOOT_MSG_LINE1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Mode Selection
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_ModeSet(seq_tpd_mode_t mode)
{
  tpd_mode = mode;
  return 0; // no error
}

seq_tpd_mode_t SEQ_TPD_ModeGet(void)
{
  return tpd_mode;
}


/////////////////////////////////////////////////////////////////////////////
// Optional Scroll Text
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_PrintString(char *str)
{
  // first three chars should be ' '
  int i;
  for(i=0; i<3; ++i)
    tpd_scroll_text[i] = ' ';

  strncpy(tpd_scroll_text+3, str, TPD_TEXT_LENGTH-3);
  tpd_scroll_text[TPD_TEXT_LENGTH-1] = 0; // to ensure that the text is terminated

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// LED update routine (should be called from APP_SRIO_ServicePrepare)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_LED_Update(void)
{
  static u8 cycle_ctr = 0;

  if( !seq_hwcfg_tpd.enabled )
    return 0; // TPD disabled

  if( ++cycle_ctr >= 8 )
    cycle_ctr = 0;

  // transfer pattern to SRs
  {
    int i;
    u8 inversion_mask = (seq_hwcfg_tpd.enabled == 2) ? 0xff : 0x00;

    // row selection
    u8 select_pattern = ~(1 << cycle_ctr);
    for(i=0; i<2; ++i) {
      if( seq_hwcfg_tpd.columns_sr[i] )
	MIOS32_DOUT_SRSet(seq_hwcfg_tpd.columns_sr[i] - 1, mios32_dout_reverse_tab[select_pattern ^ inversion_mask]);
    }

    // row patterns
    for(i=0; i<2; ++i) {
      if( seq_hwcfg_tpd.rows_sr_green[i] )
	MIOS32_DOUT_SRSet(seq_hwcfg_tpd.rows_sr_green[i] - 1, mios32_dout_reverse_tab[tpd_display[i][TPD_GREEN][cycle_ctr] ^ inversion_mask]);
      
      if( seq_hwcfg_tpd.rows_sr_red[i] )
	MIOS32_DOUT_SRSet(seq_hwcfg_tpd.rows_sr_red[i] - 1, mios32_dout_reverse_tab[tpd_display[i][TPD_RED][cycle_ctr] ^ inversion_mask]);
    }
  }
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called periodically from a low prio task to update the displayed TPD content
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_Handler(void)
{
  if( !seq_hwcfg_tpd.enabled )
    return 0; // TPD disabled

#if TPD_NUM_COL_SR != 2 || TPD_NUM_ROW_SR != 2 || TPD_NUM_ROWS != 8
# error "only hardcoded for specific display dimension with up to 16x8 LEDs"
#endif

  // text scroll mode?
  if( tpd_scroll_text[0] ) {
    return SEQ_TPD_ScrollTextHandler();
  }

  // clear display
  {
    int i;
    u8 *tpd_display_ptr = (u8 *)&tpd_display[0];
    for(i=0; i<TPD_NUM_COL_SR*TPD_NUM_ROW_SR*TPD_NUM_ROWS; ++i)
      *tpd_display_ptr++ = 0;
  }

  // display pattern depending on mode
  switch( tpd_mode ) {
  case SEQ_TPD_Mode_MeterAndPos:
  case SEQ_TPD_Mode_RotatedMeterAndPos:
  case SEQ_TPD_Mode_DotMeterAndPos:
  case SEQ_TPD_Mode_RotatedDotMeterAndPos: {
    u8 horizontal_display = tpd_mode == SEQ_TPD_Mode_RotatedMeterAndPos || tpd_mode == SEQ_TPD_Mode_RotatedDotMeterAndPos;
    u8 dotmeter = tpd_mode == SEQ_TPD_Mode_DotMeterAndPos || tpd_mode == SEQ_TPD_Mode_RotatedDotMeterAndPos;

    u8 col, row;
    for(col=0; col<TPD_NUM_COL_SR; ++col) {
      for(row=0; row<TPD_NUM_ROWS; ++row) {
	u8 track = 8*col + row;

	// determine track position
	s32 num_steps = seq_cc_trk[track].length + 1;
	s32 cur_step = seq_core_trk[track].step;
	s32 rel_pos = (8*cur_step) / num_steps;

	// set pos LED only if not muted
	if( !(seq_core_trk_muted & (1 << track)) ) {
	  if( horizontal_display ) {
	    tpd_display[col][TPD_RED][row] = (1 << rel_pos);
	  } else {
	    tpd_display[col][TPD_RED][7-rel_pos] |= (1 << row);
	  }
	}

	// meter
	u8 meter = seq_core_trk[track].vu_meter;
	if( meter ) {
	  u8 meter_pos = (meter >> 4) & 7;
	  if( horizontal_display ) {
	    u8 pattern = dotmeter ? (1 << meter_pos) : ((1 << (meter_pos+1))-1);
	    tpd_display[col][TPD_GREEN][row] = pattern;
	  } else {
	    if( dotmeter ) {
	      tpd_display[col][TPD_GREEN][7-meter_pos] |= (1 << row);
	    } else {
	      int i;
	      u8 mask = 1 << row;
	      u8 *tpd_display_ptr = &tpd_display[col][TPD_GREEN][7];
	      for(i=0; i<=meter_pos; ++i) {
		*tpd_display_ptr-- |= mask;
	      }
	    }
	  }
	}
      }      
    }
  } break;

  case SEQ_TPD_Mode_PosAndTrack:
  case SEQ_TPD_Mode_RotatedPosAndTrack:
  default: {
    u8 horizontal_display = tpd_mode == SEQ_TPD_Mode_RotatedPosAndTrack;
    u8 col, row;
    for(col=0; col<TPD_NUM_COL_SR; ++col) {
      for(row=0; row<TPD_NUM_ROWS; ++row) {
	u8 track = 8*col + row;

	// determine relative track position
	s32 num_steps = seq_cc_trk[track].length + 1;
	s32 cur_step = seq_core_trk[track].step;
	s32 rel_pos = (8*cur_step) / num_steps;

	// set pos LED only if not muted
	if( !(seq_core_trk_muted & (1 << track)) ) {
	  if( horizontal_display ) {
	    tpd_display[col][TPD_GREEN][row] = (1 << rel_pos);
	  } else {
	    tpd_display[col][TPD_GREEN][7-rel_pos] |= (1 << row);
	  }
	}

	// if track is selected: turn on red LED as well
	if( SEQ_UI_IsSelectedTrack(track) ) {
	  if( horizontal_display ) {
	    tpd_display[col][TPD_RED][row] = (1 << rel_pos);
	  } else {
	    tpd_display[col][TPD_RED][7-rel_pos] |= (1 << row);
	  }
	}
      }      
    }
  }
  }
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Print text on TPD
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_TPD_ScrollTextHandler(void)
{
  static u32 prev_timestamp = 0;
  static u8 pixel_ctr = 0;

  // update each 50 mS
  if( MIOS32_TIMESTAMP_GetDelay(prev_timestamp) < 50 ) // should also work correctly on overruns...
    return 0;
  prev_timestamp = MIOS32_TIMESTAMP_Get();

  const u8 *font = (const u8 *)&GLCD_FONT_NORMAL; // this is a 8x6 font -> almost 3 characters can be displayed
  mios32_lcd_bitmap_t font_bitmap;
  font_bitmap.memory = (u8 *)&font[MIOS32_LCD_FONT_BITMAP_IX] + (size_t)font[MIOS32_LCD_FONT_X0_IX];
  font_bitmap.width = font[MIOS32_LCD_FONT_WIDTH_IX];
  font_bitmap.height = font[MIOS32_LCD_FONT_HEIGHT_IX];
  font_bitmap.line_offset = font[MIOS32_LCD_FONT_OFFSET_IX];

  if( ++pixel_ctr > font_bitmap.line_offset ) {
    pixel_ctr = 0;
    memmove(tpd_scroll_text, tpd_scroll_text+1, TPD_TEXT_LENGTH-1);
  }

  // copy 4 characters to temporary 8x32 screen
  u32 screen_buffer[8];
  {
    int i;
    for(i=0; i<8; ++i)
      screen_buffer[i] = 0;
  }

  int pos;
  for(pos=0; pos<4; ++pos) {
    char c = tpd_scroll_text[pos];
    if( !c )
      break;

    u8 *font_ptr = font_bitmap.memory + (font_bitmap.height>>3) * font_bitmap.line_offset * (size_t)c;
    u8 x, y;
    for(x=0; x<font_bitmap.width; ++x) {
      u8 x_pos = pos*font_bitmap.line_offset + x;

      if( x_pos < 32 ) { // limited by u32
	u8 b = *font_ptr++;
	u32 mask = 1 << x_pos;
	for(y=0; y<8; ++y) {
	  if( b & (1 << y)  )
	    screen_buffer[y] |= mask;
	  else
	    screen_buffer[y] &= ~mask;
	}
      }
    }      
  }

  // copy temporary screen into TPD display
  {
    u8 col, row;
    for(col=0; col<TPD_NUM_COL_SR; ++col) {
      u32 *screen_buffer_ptr = (u32 *)&screen_buffer[0];
      for(row=0; row<TPD_NUM_ROWS; ++row) {
	u8 pattern = (*screen_buffer_ptr++) >> (col*8 + pixel_ctr);
	tpd_display[col][TPD_GREEN][row] = pattern;
	tpd_display[col][TPD_RED][row] = pattern;
      }
    }
  }

  return 0; // no error
}
