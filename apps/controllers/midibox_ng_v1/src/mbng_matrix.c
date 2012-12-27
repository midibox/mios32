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
#include <string.h>

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_matrix.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_din.h"


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
static const u16 dout_matrix_pattern_preload[MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS][MBNG_MATRIX_DOUT_NUM_PATTERN_POS] = {

  {
    // 17 entries for LED pattern #1 - requires 11 LEDs
    0x0001, // [ 0] b'0000000000000001'
    0x0003, // [ 1] b'0000000000000011'
    0x0003, // [ 2] b'0000000000000011'
    0x0007, // [ 3] b'0000000000000111'
    0x0007, // [ 4] b'0000000000000111'
    0x000f, // [ 5] b'0000000000001111'
    0x000f, // [ 6] b'0000000000001111'
    0x001f, // [ 7] b'0000000000011111'
    0x083f, // [ 8] b'0000100000111111' // taken when mid value has been selected
    0x007f, // [ 9] b'0000000001111111'
    0x00ff, // [10] b'0000000011111111'
    0x00ff, // [11] b'0000000011111111'
    0x01ff, // [12] b'0000000111111111'
    0x01ff, // [13] b'0000000111111111'
    0x03ff, // [14] b'0000001111111111'
    0x03ff, // [15] b'0000001111111111'
    0x07ff, // [16] b'0000011111111111'
  },

  {
    // 17 entries for LED pattern #2 - requires 11 LEDs
    0x003f, // [ 0] b'0000000000111111'
    0x003e, // [ 1] b'0000000000111110'
    0x003e, // [ 2] b'0000000000111110'
    0x003c, // [ 3] b'0000000000111100'
    0x0038, // [ 4] b'0000000000111000'
    0x0038, // [ 5] b'0000000000111000'
    0x0030, // [ 6] b'0000000000110000'
    0x0020, // [ 7] b'0000000000100000'
    0x0870, // [ 8] b'0000100001110000' // taken when mid value has been selected
    0x0020, // [ 9] b'0000000000100000'
    0x0060, // [10] b'0000000001100000'
    0x0060, // [11] b'0000000001100000'
    0x00e0, // [12] b'0000000011100000'
    0x01e0, // [13] b'0000000111100000'
    0x01e0, // [14] b'0000000111100000'
    0x03e0, // [15] b'0000001111100000'
    0x07e0, // [16] b'0000011111100000'
  },

  {
    // 17 entries for LED pattern #3 - requires 11 LEDs
    0x0001, // [ 0] b'0000000000000001'
    0x0002, // [ 1] b'0000000000000010'
    0x0002, // [ 2] b'0000000000000010'
    0x0004, // [ 3] b'0000000000000100'
    0x0004, // [ 4] b'0000000000000100'
    0x0008, // [ 5] b'0000000000001000'
    0x0010, // [ 6] b'0000000000010000'
    0x0020, // [ 7] b'0000000000100000'
    0x0870, // [ 8] b'0000100001110000' // taken when mid value has been selected
    0x0020, // [ 9] b'0000000000100000'
    0x0040, // [10] b'0000000001000000'
    0x0080, // [11] b'0000000010000000'
    0x0080, // [12] b'0000000010000000'
    0x0100, // [13] b'0000000100000000'
    0x0200, // [14] b'0000001000000000'
    0x0200, // [15] b'0000001000000000'
    0x0400, // [16] b'0000010000000000'
  },

  {
    // 17 entries for LED pattern #4 - requires 11 LEDs
    0x0020, // [ 0] b'0000000000100000'
    0x0020, // [ 1] b'0000000000100000'
    0x0070, // [ 2] b'0000000001110000'
    0x0070, // [ 3] b'0000000001110000'
    0x0070, // [ 4] b'0000000001110000'
    0x00f8, // [ 5] b'0000000011111000'
    0x00f8, // [ 6] b'0000000011111000'
    0x00f8, // [ 7] b'0000000011111000'
    0x09fc, // [ 8] b'0000100111111100' // taken when mid value has been selected
    0x01fc, // [ 9] b'0000000111111100'
    0x01fc, // [10] b'0000000111111100'
    0x01fc, // [11] b'0000000111111100'
    0x03fe, // [12] b'0000001111111110'
    0x03fe, // [13] b'0000001111111110'
    0x03fe, // [14] b'0000001111111110'
    0x07ff, // [15] b'0000011111111111'
    0x07ff, // [16] b'0000011111111111'
  },

};


// stored in RAM so that it can be redefined in the .NGC file
static u16 dout_matrix_pattern[MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS][MBNG_MATRIX_DOUT_NUM_PATTERN_POS];


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

  // initial LED patterns
  memcpy((u16 *)dout_matrix_pattern, (u16 *)dout_matrix_pattern_preload, 2*MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS*MBNG_MATRIX_DOUT_NUM_PATTERN_POS);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Set/Get pattern
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_PatternSet(u8 num, u8 pos, u16 pattern)
{
  if( num >= MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS )
    return -1;

  if( pos >= MBNG_MATRIX_DOUT_NUM_PATTERN_POS )
    return -2;

  dout_matrix_pattern[num][pos] = pattern;

  return 0; // no error
}

u16 MBNG_MATRIX_PatternGet(u8 num, u8 pos)
{
  if( num >= MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS )
    return 0x0000;

  if( pos >= MBNG_MATRIX_DOUT_NUM_PATTERN_POS )
    return 0x0000;

  return dout_matrix_pattern[num][pos];
}


/////////////////////////////////////////////////////////////////////////////
// This function sets the a pin on the given DOUT matrix
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PinSet(u8 matrix, u8 color, u16 pin, u8 value)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  int num_rows = mbng_patch_matrix_dout[matrix].num_rows;
  int row    = pin / num_rows;
  int column = pin % num_rows;
  u16 *led_row_ptr = (u16 *)&led_row[matrix][row][color];

  if( value )
    *led_row_ptr |= (1 << column);
  else
    *led_row_ptr &= ~(1 << column);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sets a pattern DOUT matrix row
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet(u8 matrix, u8 color, u16 row, u16 value, u16 range, u8 pattern)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  u32 scaled = ((MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)*value) / range;
  int pattern_ix;
  if( scaled <= 7 )
    pattern_ix = scaled;
  else if( value == (range/2) ) {
    pattern_ix = (MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)/2;
  } else {
    pattern_ix = scaled+1;
  }

  u16 *led_row_ptr = (u16 *)&led_row[matrix][row][color];
  *led_row_ptr = dout_matrix_pattern[pattern][pattern_ix];
#if 0
  DEBUG_MSG("matrix=%d row=%d value=%d range=%d scaled=%d ix=%d led_row=0x%04x\n", matrix, row, value, range, scaled, pattern_ix, *led_row_ptr);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sets a pattern DOUT matrix row for the LC protocol
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet_LC(u8 matrix, u8 color, u16 row, u16 value)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  u8 pattern = (value >> 4) & 0x3;
  u8 pattern_ix = value & 0x0f;
  if( pattern_ix >= ((MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)/2) )
    ++pattern_ix; // 'M' entry not supported...
  u8 center_led = value & 0x40;

  u16 *led_row_ptr = (u16 *)&led_row[matrix][row][color];
  *led_row_ptr = dout_matrix_pattern[pattern][pattern_ix] | (center_led ? (1 << 11) : 0);

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
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DIN )
    return -1; // invalid matrix
    
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MIOS32_MIDI_SendDebugMessage("MBNG_MATRIX_NotifyToggle(%d, %d, %d)\n", matrix, pin, pin_value);
  }

  // "normal" buttons emulated?
  if( mbng_patch_matrix_din[matrix].button_emu_id_offset ) // -> DIN handler
    return MBNG_DIN_NotifyToggle(pin + mbng_patch_matrix_din[matrix].button_emu_id_offset - 1, pin_value);

  // get ID
  mbng_event_item_id_t matrix_id = MBNG_EVENT_CONTROLLER_BUTTON_MATRIX + matrix + 1;
  MBNG_PATCH_BankCtrlIdGet(matrix, &matrix_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(matrix_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to BUTTON_MATRIX id=%d\n", matrix_id & 0xfff);
    }
    return -2; // no event assigned
  }

  // EXTRA for matrix: store pin number for MBNG_EVENT_Item* functions
  item.matrix_pin = pin;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // button depressed?
  u8 depressed = pin_value ? 1 : 0;
  item.value = depressed ? item.min : item.max;

  // send MIDI event
  MBNG_EVENT_NotifySendValue(&item);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DIN_NotifyReceivedValue(mbng_event_item_t *item)
{
  int button_matrix_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DIN_NotifyReceivedValue(%d, %d)\n", button_matrix_ix, item->value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_NotifyReceivedValue(mbng_event_item_t *item)
{
  int led_matrix_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DOUT_NotifyReceivedValue(%d, %d)\n", led_matrix_ix, item->value);
  }

#if MBNG_PATCH_NUM_MATRIX_ROWS_MAX != 16
# error "not prepared for != 16 rows - id assignments have to be adapted!"
#endif

  if( !item->flags.LED_MATRIX.led_matrix_pattern ) {
    if( led_matrix_ix ) {
      // no LED pattern: set bit directly depending on item->matrix_pin, which could have been
      // set by a EVENT_BUTTON_MATRIX
      int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
      u8 dout_value = (item->min <= item->max) ? ((item->value - item->min) >= (range/2)) : ((item->value - item->max) >= (range/2));

      u8 color = 0; // TODO...
      MBNG_MATRIX_DOUT_PinSet(led_matrix_ix-1, color, item->matrix_pin, dout_value);
    }
  } else {
    int matrix = (led_matrix_ix-1) / MBNG_PATCH_NUM_MATRIX_ROWS_MAX;
    int row = (led_matrix_ix-1) % MBNG_PATCH_NUM_MATRIX_ROWS_MAX;
    u8 color = 0; // TODO...
    // this is actually a dirty solution: without LED patterns we address the matrix directly with 1..8,
    // and here we multiply by 16 since we've connected 16 LED Rings... :-/

    if( item->flags.LED_MATRIX.led_matrix_pattern >= MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO ) {
      MBNG_MATRIX_DOUT_PatternSet_LC(matrix, color, row, item->value);

    } else if( item->flags.LED_MATRIX.led_matrix_pattern >= MBNG_EVENT_LED_MATRIX_PATTERN_1 &&
	       item->flags.LED_MATRIX.led_matrix_pattern <= MBNG_EVENT_LED_MATRIX_PATTERN_4 ) {
      u8 pattern = item->flags.LED_MATRIX.led_matrix_pattern - MBNG_EVENT_LED_MATRIX_PATTERN_1;

      int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);

      int saturated_value;
      if( item->min <= item->max ) {
	saturated_value = item->value - item->min;
      } else {
	// reversed range
	saturated_value = item->min - item->value;
      }
      if( saturated_value < 0 )
	saturated_value = 0;
      if( saturated_value > range )
	saturated_value = range - 1;

      MBNG_MATRIX_DOUT_PatternSet(matrix, color, row, saturated_value, range, pattern);
    }
  }

  return 0; // no error
}
