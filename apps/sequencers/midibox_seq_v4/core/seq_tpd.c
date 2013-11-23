// $Id$
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

#define TPD_NUM_COL_SR 2 // left/right half
#define TPD_NUM_ROW_SR 2 // green/red colour
#define TPD_NUM_ROWS   8 // don't change

static u8 tpd_display[TPD_NUM_COL_SR][TPD_NUM_ROW_SR][TPD_NUM_ROWS];

#define TPD_GREEN 0 // green layer of TPD_NUM_ROW_SR dimension
#define TPD_RED   1 // red layer of TPD_NUM_ROW_SR dimension


/////////////////////////////////////////////////////////////////////////////
// Init function
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_Init(u32 mode)
{
  // initial mode
  tpd_mode = SEQ_TPD_Mode_PosAndTrack;

  return 0; // no error
}

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
