// $Id$
/*
 * Scan Matrix functions for MIDIbox NG
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

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_matrix.h"
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
// local definitions
/////////////////////////////////////////////////////////////////////////////

// execution time of MBNG_MATRIX_PrepareCol() can be monitored on J5.A0 with a scope
// current values:
//   - no matrix assignments: ca. 10 uS
//   - 1 DIN, 1 DOUT matrix assigment: ca. 12..17 uS (jittering)
//   - 8 DIN, 8 DOUT matrix assigments: ca. 33..40 uS (jittering)
// -> with update cycle of 1 mS the system loaded is by ca. 2.5 % in worst case -> no additional optimisation required
#define MEASURE_PERFORMANCE 0


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// just to ensure
#if MBNG_PATCH_NUM_MATRIX_ROWS_MAX != 16
# error "not prepared for != 16 rows - many variable types have to be adapted!"
#endif

// note: to simplify synchronisation between multiple matrices driven by a single row,
// we currently only support 4, 8 or 16 rows.
// this is ensured in mbng_file_p
static u8 selected_row;

static u16 button_row[MBNG_PATCH_NUM_MATRIX_DIN][MBNG_PATCH_NUM_MATRIX_ROWS_MAX];
static u16 button_row_changed[MBNG_PATCH_NUM_MATRIX_DIN][MBNG_PATCH_NUM_MATRIX_ROWS_MAX];
static u16 button_debounce_ctr[MBNG_PATCH_NUM_MATRIX_DIN];

static u16 led_row[MBNG_PATCH_NUM_MATRIX_DOUT][MBNG_PATCH_NUM_MATRIX_ROWS_MAX][MBNG_PATCH_NUM_MATRIX_COLORS_MAX];

static u8 debounce_reload;


// pre-calculated selection patterns, since we need them very often
const u16 selection_4rows[MBNG_PATCH_NUM_MATRIX_ROWS_MAX] = {
  0xeeee, 0xdddd, 0xbbbb, 0x7777, // duplicate selection lines on all nibbles so that the remaining DOUT pins can be used as well (MBSEQ V3 BLM matrix)
  0xeeee, 0xdddd, 0xbbbb, 0x7777,
  0xeeee, 0xdddd, 0xbbbb, 0x7777,
  0xeeee, 0xdddd, 0xbbbb, 0x7777,
};

const u16 selection_8rows[MBNG_PATCH_NUM_MATRIX_ROWS_MAX] = {
  0xfefe, 0xfdfd, 0xfbfb, 0xf7f7, // duplicate selection lines at second byte for the case that the second DOUT is used as well (for whatever reason)
  0xefef, 0xdfdf, 0xbfbf, 0x7f7f,
  0xfefe, 0xfdfd, 0xfbfb, 0xf7f7,
  0xefef, 0xdfdf, 0xbfbf, 0x7f7f,
};

const u16 selection_16rows[MBNG_PATCH_NUM_MATRIX_ROWS_MAX] = {
  0xfffe, 0xfffd, 0xfffb, 0xfff7,
  0xffef, 0xffdf, 0xffbf, 0xff7f,
  0xfeff, 0xfdff, 0xfbff, 0xf7ff,
  0xefff, 0xdfff, 0xbfff, 0x7fff,
};

// the b'...' patterns have been converted with tools/patterns.pl to hexadecimal numbers
static const u16 ledring_pattern[MBNG_MATRIX_DOUT_LEDRING_PATTERNS][33] = {

  {
    // 33 entries for LED pattern #1 - requires 11 LEDs
    0x0001, // [ 0] b'0000000000000001'
    0x0001, // [ 1] b'0000000000000001'
    0x0003, // [ 2] b'0000000000000011'
    0x0003, // [ 3] b'0000000000000011'
    0x0003, // [ 4] b'0000000000000011'
    0x0007, // [ 5] b'0000000000000111'
    0x0007, // [ 6] b'0000000000000111'
    0x0007, // [ 7] b'0000000000000111'
    0x000f, // [ 8] b'0000000000001111'
    0x000f, // [ 9] b'0000000000001111'
    0x000f, // [10] b'0000000000001111'
    0x001f, // [11] b'0000000000011111'
    0x001f, // [12] b'0000000000011111'
    0x001f, // [13] b'0000000000011111'
    0x003f, // [14] b'0000000000111111'
    0x003f, // [15] b'0000000000111111'
    0x003f, // [16] b'0000000000111111' // taken when mid value has been selected
    0x003f, // [17] b'0000000000111111'
    0x003f, // [18] b'0000000000111111'
    0x007f, // [19] b'0000000001111111'
    0x007f, // [20] b'0000000001111111'
    0x007f, // [21] b'0000000001111111'
    0x00ff, // [22] b'0000000011111111'
    0x00ff, // [23] b'0000000011111111'
    0x00ff, // [24] b'0000000011111111'
    0x01ff, // [25] b'0000000111111111'
    0x01ff, // [26] b'0000000111111111'
    0x01ff, // [27] b'0000000111111111'
    0x03ff, // [28] b'0000001111111111'
    0x03ff, // [29] b'0000001111111111'
    0x03ff, // [30] b'0000001111111111'
    0x07ff, // [31] b'0000011111111111'
    0x07ff, // [32] b'0000011111111111'
  },

  {
    // 33 entries for LED pattern #2 - requires 11 LEDs
    0x003f, // [ 0] b'0000000000111111'
    0x003f, // [ 1] b'0000000000111111'
    0x003e, // [ 2] b'0000000000111110'
    0x003e, // [ 3] b'0000000000111110'
    0x003e, // [ 4] b'0000000000111110'
    0x003c, // [ 5] b'0000000000111100'
    0x003c, // [ 6] b'0000000000111100'
    0x003c, // [ 7] b'0000000000111100'
    0x0038, // [ 8] b'0000000000111000'
    0x0038, // [ 9] b'0000000000111000'
    0x0038, // [10] b'0000000000111000'
    0x0030, // [11] b'0000000000110000'
    0x0030, // [12] b'0000000000110000'
    0x0030, // [13] b'0000000000110000'
    0x0020, // [14] b'0000000000100000'
    0x0020, // [15] b'0000000000100000'
    0x0070, // [16] b'0000000001110000' // taken when mid value has been selected
    0x0020, // [17] b'0000000000100000'
    0x0020, // [18] b'0000000000100000'
    0x0060, // [19] b'0000000001100000'
    0x0060, // [20] b'0000000001100000'
    0x0060, // [21] b'0000000001100000'
    0x00e0, // [22] b'0000000011100000'
    0x00e0, // [23] b'0000000011100000'
    0x00e0, // [24] b'0000000011100000'
    0x01e0, // [25] b'0000000111100000'
    0x01e0, // [26] b'0000000111100000'
    0x01e0, // [27] b'0000000111100000'
    0x03e0, // [28] b'0000001111100000'
    0x03e0, // [29] b'0000001111100000'
    0x03e0, // [30] b'0000001111100000'
    0x07e0, // [31] b'0000011111100000'
    0x07e0, // [32] b'0000011111100000'
  },

  {
    // 33 entries for LED pattern #3 - requires 11 LEDs
    0x0001, // [ 0] b'0000000000000001'
    0x0001, // [ 1] b'0000000000000001'
    0x0001, // [ 2] b'0000000000000001'
    0x0002, // [ 3] b'0000000000000010'
    0x0002, // [ 4] b'0000000000000010'
    0x0002, // [ 5] b'0000000000000010'
    0x0004, // [ 6] b'0000000000000100'
    0x0004, // [ 7] b'0000000000000100'
    0x0004, // [ 8] b'0000000000000100'
    0x0008, // [ 9] b'0000000000001000'
    0x0008, // [10] b'0000000000001000'
    0x0008, // [11] b'0000000000001000'
    0x0010, // [12] b'0000000000010000'
    0x0010, // [13] b'0000000000010000'
    0x0010, // [14] b'0000000000010000'
    0x0020, // [15] b'0000000000100000'
    0x0070, // [16] b'0000000001110000' // taken when mid value has been selected
    0x0020, // [17] b'0000000000100000'
    0x0040, // [18] b'0000000001000000'
    0x0040, // [19] b'0000000001000000'
    0x0040, // [20] b'0000000001000000'
    0x0080, // [21] b'0000000010000000'
    0x0080, // [22] b'0000000010000000'
    0x0080, // [23] b'0000000010000000'
    0x0100, // [24] b'0000000100000000'
    0x0100, // [25] b'0000000100000000'
    0x0100, // [26] b'0000000100000000'
    0x0200, // [27] b'0000001000000000'
    0x0200, // [28] b'0000001000000000'
    0x0200, // [29] b'0000001000000000'
    0x0400, // [30] b'0000010000000000'
    0x0400, // [31] b'0000010000000000'
    0x0400, // [32] b'0000010000000000'
  },

  {
    // 33 entries for LED pattern #4 - requires 11 LEDs
    0x0020, // [ 0] b'0000000000100000'
    0x0020, // [ 1] b'0000000000100000'
    0x0020, // [ 2] b'0000000000100000'
    0x0020, // [ 3] b'0000000000100000'
    0x0070, // [ 4] b'0000000001110000'
    0x0070, // [ 5] b'0000000001110000'
    0x0070, // [ 6] b'0000000001110000'
    0x0070, // [ 7] b'0000000001110000'
    0x0070, // [ 8] b'0000000001110000'
    0x0070, // [ 9] b'0000000001110000'
    0x00f8, // [10] b'0000000011111000'
    0x00f8, // [11] b'0000000011111000'
    0x00f8, // [12] b'0000000011111000'
    0x00f8, // [13] b'0000000011111000'
    0x00f8, // [14] b'0000000011111000'
    0x00f8, // [15] b'0000000011111000'
    0x01fc, // [16] b'0000000111111100' // taken when mid value has been selected
    0x01fc, // [17] b'0000000111111100'
    0x01fc, // [18] b'0000000111111100'
    0x01fc, // [19] b'0000000111111100'
    0x01fc, // [20] b'0000000111111100'
    0x01fc, // [21] b'0000000111111100'
    0x01fc, // [22] b'0000000111111100'
    0x03fe, // [23] b'0000001111111110'
    0x03fe, // [24] b'0000001111111110'
    0x03fe, // [25] b'0000001111111110'
    0x03fe, // [26] b'0000001111111110'
    0x03fe, // [27] b'0000001111111110'
    0x03fe, // [28] b'0000001111111110'
    0x07ff, // [29] b'0000011111111111'
    0x07ff, // [30] b'0000011111111111'
    0x07ff, // [31] b'0000011111111111'
    0x07ff, // [32] b'0000011111111111'
  },

};


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
// This function initializes the matrix handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

#if MEASURE_PERFORMANCE
  MIOS32_BOARD_J5_PinInit(0, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#endif

  selected_row = 0;

  {
    int matrix, row;
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix) {
      for(row=0; row<MBNG_PATCH_NUM_MATRIX_ROWS_MAX; ++row) {
	button_row[matrix][row] = 0xffff;
	button_row_changed[matrix][row] = 0x0000;
      }
      button_debounce_ctr[matrix] = 0;
    }
  }

  debounce_reload = 10; // mS

  {
    int matrix, row, color;
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix) {
      for(row=0; row<MBNG_PATCH_NUM_MATRIX_ROWS_MAX; ++row) {
	for(color=0; color<MBNG_PATCH_NUM_MATRIX_COLORS_MAX; ++color) {
	  led_row[matrix][row][color] = 0x0000;
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a column.
// It should be called from the APP_SRIO_ServicePrepare()
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_PrepareCol(void)
{
#if MEASURE_PERFORMANCE
  MIOS32_BOARD_J5_PinSet(0, 1);
#endif

  // increment column, wrap at MBNG_PATCH_NUM_MATRIX_ROWS_MAX
  if( ++selected_row >= MBNG_PATCH_NUM_MATRIX_ROWS_MAX )
    selected_row = 0;

  {
    // DIN selection:
    // forward value to all shift registers which are assigned to the matrix function
    int matrix;
    mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {
      // select next DIN column (selected cathode line = 0, all others 1)

      u16 dout_value;
      if( m->num_rows <= 4 ) dout_value = selection_4rows[selected_row];
      else if( m->num_rows <= 8 ) dout_value = selection_8rows[selected_row];
      else dout_value = selection_16rows[selected_row];

      if( m->inverted ) {
	dout_value ^= 0xffff;
      }

      if( m->sr_dout_sel1 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_sel1-1, dout_value);
      }

      if( m->sr_dout_sel2 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_sel2-1, dout_value >> 8);
      }
    }
  }


  {
    // same for DOUTs
    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    u16 *pattern = (u16 *)&led_row[0][selected_row];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
      // select next DIN column (selected cathode line = 0, all others 1)

      u16 dout_value;
      if( m->num_rows <= 4 ) dout_value = selection_4rows[selected_row];
      else if( m->num_rows <= 8 ) dout_value = selection_8rows[selected_row];
      else dout_value = selection_16rows[selected_row];
      if( m->inverted ) { dout_value ^= 0xffff; }

      if( m->sr_dout_sel1 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_sel1-1, dout_value);
      }

      if( m->sr_dout_sel2 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_sel2-1, dout_value >> 8);
      }

#if MBNG_PATCH_NUM_MATRIX_COLORS_MAX != 3
# error "only prepared for 3 colors" // just to ensure
#endif
      dout_value = *pattern++;
      if( m->inverted ) { dout_value ^= 0xffff; }
      if( m->sr_dout_r1 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_r1-1, dout_value);
      }
      if( m->sr_dout_r2 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_r2-1, dout_value >> 8);
      }

      dout_value = *pattern++;
      if( m->inverted ) { dout_value ^= 0xffff; }
      if( m->sr_dout_g1 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_g1-1, dout_value);
      }
      if( m->sr_dout_g2 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_g2-1, dout_value >> 8);
      }

      dout_value = *pattern++;
      if( m->inverted ) { dout_value ^= 0xffff; }
      if( m->sr_dout_b1 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_b1-1, dout_value);
      }
      if( m->sr_dout_b2 ) {
	MIOS32_DOUT_SRSet(m->sr_dout_b2-1, dout_value >> 8);
      }
    }
  }

#if MEASURE_PERFORMANCE
  MIOS32_BOARD_J5_PinSet(0, 0);
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected row.
// It should be called from the APP_SRIO_ServiceFinish() hook
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_GetRow(void)
{
  int matrix;
  mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {
    // determine the scanned row (selected_row driver has already been incremented, decrement it again)
    u8 row = (selected_row-1) & (m->num_rows-1); // num_rows is either 4, 8 or 16

    // decrememt debounce counter if > 0
    if( button_debounce_ctr[matrix] )
      --button_debounce_ctr[matrix];

    // get DIN values (if SR is assigned to matrix)
    if( m->sr_din1 || m->sr_din2 ) {
      u16 sr_value = 0;

      if( m->sr_din1 ) {
	MIOS32_DIN_SRChangedGetAndClear(m->sr_din1-1, 0xff); // ensure that change won't be propagated to normal DIN handler
	sr_value = MIOS32_DIN_SRGet(m->sr_din1-1);
      } else {
	if( !m->inverted )
	  sr_value = 0x00ff;
      }

      if( m->sr_din2 ) {
	MIOS32_DIN_SRChangedGetAndClear(m->sr_din2-1, 0xff); // ensure that change won't be propagated to normal DIN handler
	sr_value |= (u16)MIOS32_DIN_SRGet(m->sr_din2-1) << 8;
      } else {
	if( !m->inverted )
	  sr_value |= 0xff00;
      }

      // determine pin changes
      u16 changed = sr_value ^ button_row[matrix][row];

      // ignore as long as debounce counter running
      if( button_debounce_ctr[matrix] )
	break;

      // add them to existing notifications
      button_row_changed[matrix][row] |= (changed << 0);

      // store new value
      button_row[matrix][row] = sr_value;

      // reload debounce counter if any pin has changed
      if( changed )
	button_debounce_ctr[matrix] = debounce_reload;
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from a task to check for button changes
// periodically. Events (change from 0->1 or from 1->0) will be notified 
// to MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_ButtonHandler(void)
{
  int matrix, row;
  mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {
    for(row=0; row<m->num_rows; ++row) {
    
      // check if there are pin changes - must be atomic!
      MIOS32_IRQ_Disable();
      u16 changed = button_row_changed[matrix][row];
      button_row_changed[matrix][row] = 0;
      MIOS32_IRQ_Enable();

      // any pin change at this SR?
      if( !changed )
	continue;

      // check all pins of the SR
      int pin;
      u16 mask = (1 << 0);
      for(pin=0; pin<m->num_rows; ++pin, mask <<= 1)
	if( changed & mask )
	  MBNG_MATRIX_NotifyToggle(matrix, row*m->num_rows+pin, (button_row[matrix][row] & mask) ? 1 : 0);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function will be called by MBNG_MATRIX_ButtonHandler whenever
// a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
{
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MIOS32_MIDI_SendDebugMessage("matrix=%d pin=%d pin_value=%d\n", matrix, pin, pin_value);
  }

  // TODO

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int button_matrix_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DIN_NotifyReceivedValue(%d, %d)\n", button_matrix_ix, value);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int led_matrix_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DOUT_NotifyReceivedValue(%d, %d)\n", led_matrix_ix, value);
  }

#if MBNG_PATCH_NUM_MATRIX_ROWS_MAX != 16
# error "not prepared for != 16 rows - id assignments have to be adapted!"
#endif

  int row = (led_matrix_ix-1) % mbng_patch_cfg.matrix_dout_group_size;
  int matrix = (led_matrix_ix-1) / mbng_patch_cfg.matrix_dout_group_size;

  if( matrix < MBNG_PATCH_NUM_MATRIX_DOUT &&
      row < MBNG_PATCH_NUM_MATRIX_ROWS_MAX ) {

    u8 color = 0;
    u16 *led_pattern = (u16 *)&led_row[matrix][row][color];

    int min = item->min;
    int max = item->max;
    if( min > max ) { // swap
      int swap = max;
      max = min;
      min = swap;
    }

    u8 pattern = 2; // TODO: take from item
    int range = max - min + 1;    
    u16 scaled = (32*value) / range;
    int pattern_ix;
    if( scaled <= 15 )
      pattern_ix = scaled;
    else if( value == (range/2) ) {
      pattern_ix = 16;
    } else {
      pattern_ix = scaled+1;
    }
    *led_pattern = ledring_pattern[pattern][pattern_ix];
#if 1
    DEBUG_MSG("matrix=%d row=%d value=%d range=%d scaled=%d ix=%d led_pattern=0x%04x\n", matrix, row, value, range, scaled, pattern_ix, *led_pattern);
#endif
  }

  return 0; // no error
}
