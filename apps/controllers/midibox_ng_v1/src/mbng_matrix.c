// $Id$
//! \defgroup MBNG_MATRIX
//! DIN/DOUT Scan Matrix functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <max72xx.h>

#include "app.h"
#include "mbng_lcd.h"
#include "mbng_matrix.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_din.h"


/////////////////////////////////////////////////////////////////////////////
//! local definitions
/////////////////////////////////////////////////////////////////////////////

// same number of levels as for "common DOUTs" (see mbng_dout.c)
// however, the actual number of levels depends on the number of scanned rows:
// 4  rows: reduced to 8 dim levels
// 8  rows: reduced to 4 dim levels
// 16 rows: reduced to 2 dim levels
#define NUM_MATRIX_DIM_LEVELS 16
// but the level is always selected in the 0..15 range to avoid unnecessary calculations at the user side


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

// just to ensure
#if MBNG_PATCH_NUM_MATRIX_ROWS_MAX != 16
# error "not prepared for != 16 rows - many variable types have to be adapted!"
#endif

#if MIOS32_SRIO_NUM_DOUT_PAGES != 32
# error "not prepared for != 32 pages - many variable types have to be adapted!"
#endif

static u16 button_row[MBNG_PATCH_NUM_MATRIX_DIN][MBNG_PATCH_NUM_MATRIX_ROWS_MAX];
static u16 button_row_changed[MBNG_PATCH_NUM_MATRIX_DIN][MBNG_PATCH_NUM_MATRIX_ROWS_MAX];
static u16 button_debounce_ctr[MBNG_PATCH_NUM_MATRIX_DIN];

static u8 debounce_reload;

static u8 max72xx_init_req; // requests MAX72xx chain initialisation
#if MAX72XX_NUM_DEVICES_PER_CHAIN > 16
# error "update the max72xx_update_chain_req variable type, it's only u16!"
#endif
static u16 max72xx_update_digit_req; // requests MAX72xx digit update (16 flags for 16 digits)


// pre-calculated selection patterns, since we need them very often
const u16 selection_4rows[MBNG_PATCH_NUM_MATRIX_ROWS_MAX] = {
  0xeeee, 0xdddd, 0xbbbb, 0x7777, // duplicate selection lines on all nibbles
  0xeeee, 0xdddd, 0xbbbb, 0x7777, // so that the remaining DOUT pins can be used as well (MBSEQ V3 BLM matrix)
  0xeeee, 0xdddd, 0xbbbb, 0x7777,
  0xeeee, 0xdddd, 0xbbbb, 0x7777,
};

const u16 selection_8rows[MBNG_PATCH_NUM_MATRIX_ROWS_MAX] = {
  0xfefe, 0xfdfd, 0xfbfb, 0xf7f7, // duplicate selection lines at second byte
  0xefef, 0xdfdf, 0xbfbf, 0x7f7f, // for the case that the second DOUT is used as well (for whatever reason)
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


// the LED digit patterns
static const u8 digit_patterns[64] = {
  //    a
  //   ---
  //  !   !
  // f! g !b
  //   ---
  //  !   !
  // e!   !c
  //   ---
  //    d   h
  // 0 = on, 1 = off
  // NOTE: the dod (h) will be set automatically by the driver above when bit 6 is 1

  // NOTE2: due to legacy reasons, the MBNG_MATRIX_DOUT_PatternSet_Digit function will invert and mirror the pattern!

              //    habcdefg     habcdefg
  0xff, 0x88, //  b'11111111', b'10001000'
  0x70, 0xb1, //  b'11100000', b'10110001'
  0xc2, 0xb0, //  b'11000010', b'10110000'
  0xb8, 0xa1, //  b'10111000', b'10100001'
  0xe8, 0xcf, //  b'11101000', b'11001111'
  0xc3, 0xf8, //  b'11000011', b'11111000'
  0xf1, 0x89, //  b'11110001', b'10001001'
  0xea, 0xe2, //  b'11101010', b'11100010'

  0x98, 0x8c, //  b'10011000', b'10001100'
  0xfa, 0x92, //  b'11111010', b'10010010'
  0xf0, 0xe3, //  b'11110000', b'11100011'
  0xd8, 0xc1, //  b'11011000', b'11000001'
  0xc8, 0xc4, //  b'11001000', b'11000100'
  0x92, 0xb1, //  b'10010010', b'10110001'
  0xec, 0x87, //  b'11101100', b'10000111'
  0x9d, 0xf7, //  b'10011101', b'11110111'

  0xff, 0xff, //  b'11111111', b'11111111'
  0xdd, 0x9c, //  b'11011101', b'10011100'
  0xa4, 0x98, //  b'10100100', b'10011000'
  0x82, 0xfd, //  b'10000010', b'11111101'
  0xb1, 0x87, //  b'10110001', b'10000111'
  0x9c, 0xce, //  b'10011100', b'11001110'
  0xfb, 0xfe, //  b'11111011', b'11111110'
  0xf7, 0xba, //  b'11110111', b'11011010'

  0x81, 0xcf, //  b'10000001', b'11001111'
  0x92, 0x86, //  b'10010010', b'10000110'
  0xcc, 0xa4, //  b'11001100', b'10100100'
  0xa0, 0x8f, //  b'10100000', b'10001111'
  0x80, 0x84, //  b'10000000', b'10000100'
  0xff, 0xbb, //  b'11111111', b'10111011'
  0xce, 0xf6, //  b'11001110', b'11110110'
  0xf8, 0x9a, //  b'11111000', b'10011010'
};


static const u16 lc_meter_pattern_preload[17] = {
  // 17 entries for LED pattern #1 - requires 11 LEDs
  0x0000, // [ 0] b'0000000000000000'
  0x0001, // [ 1] b'0000000000000001'
  0x0003, // [ 2] b'0000000000000011'
  0x0007, // [ 3] b'0000000000000111'
  0x0007, // [ 4] b'0000000000000111'
  0x000f, // [ 5] b'0000000000001111'
  0x001f, // [ 6] b'0000000000011111'
  0x003f, // [ 7] b'0000000000111111'
  0x007f, // [ 8] b'0000000001111111'
  0x00ff, // [ 9] b'0000000011111111'
  0x00ff, // [10] b'0000000011111111'
  0x01ff, // [11] b'0000000111111111'
  0x01ff, // [12] b'0000000111111111'
  0x03ff, // [13] b'0000001111111111'
  0x03ff, // [14] b'0000001111111111'
  0x07ff, // [15] b'0000011111111111'
  0x0800, // [16] b'0000100000000000' // specifies the position of the overload LED
};

static u16 lc_meter_pattern[17];


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the matrix handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  max72xx_init_req = 0;
  max72xx_update_digit_req = 0;

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
    int page, sr;
    for(page=0; page<MIOS32_SRIO_NUM_DOUT_PAGES; ++page)
      for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
	MIOS32_DOUT_PageSRSet(page, sr, 0x00);
  }

  // initial LED patterns
  memcpy((u16 *)dout_matrix_pattern, (u16 *)dout_matrix_pattern_preload, 2*MBNG_PATCH_NUM_MATRIX_DOUT_PATTERNS*MBNG_MATRIX_DOUT_NUM_PATTERN_POS);
  memcpy((u16 *)lc_meter_pattern, (u16 *)lc_meter_pattern_preload, 2*17);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Call this function whenever the LED_MATRIX configuration has been changed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_LedMatrixChanged(u8 matrix)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[matrix];

  if( !m->num_rows )
    return -2; // no rows

  // special handling for MAX72xx
  if( m->flags.max72xx_enabled ) {
    // request initialisation
    max72xx_init_req = 1;
    return 0;
  }

  int row;
  for(row=0; row<MBNG_PATCH_NUM_MATRIX_ROWS_MAX; ++row) {
    // update selection pattern based on the matrix parameters
    u16 dout_value;
    if( m->num_rows <= 4 ) dout_value = selection_4rows[row];
    else if( m->num_rows <= 8 ) dout_value = selection_8rows[row];
    else dout_value = selection_16rows[row];

    if( m->flags.inverted_sel ) {
      dout_value ^= 0xffff;
    }

    if( m->sr_dout_sel1 ) {
      MIOS32_DOUT_PageSRSet(row+ 0, m->sr_dout_sel1-1, dout_value);
      MIOS32_DOUT_PageSRSet(row+16, m->sr_dout_sel1-1, dout_value);
    }

    if( m->sr_dout_sel2 ) {
      MIOS32_DOUT_PageSRSet(row+ 0, m->sr_dout_sel2-1, dout_value >> 8);
      MIOS32_DOUT_PageSRSet(row+16, m->sr_dout_sel2-1, dout_value >> 8);
    }
  }

  u8 sr_init_value = m->flags.inverted_row ? 0xff : 0x00;
  {
    int page;
    for(page=0; page<MIOS32_SRIO_NUM_DOUT_PAGES; ++page) {
      if( m->sr_dout_r1 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_r1 - 1, sr_init_value);
      if( m->sr_dout_r2 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_r2 - 1, sr_init_value);

      if( m->sr_dout_g1 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_g1 - 1, sr_init_value);
      if( m->sr_dout_g2 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_g2 - 1, sr_init_value);

      if( m->sr_dout_b1 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_b1 - 1, sr_init_value);
      if( m->sr_dout_b2 ) MIOS32_DOUT_PageSRSet(page, m->sr_dout_b2 - 1, sr_init_value);
    }
  }
    
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Call this function whenever the BUTTON_MATRIX configuration has been changed
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_ButtonMatrixChanged(u8 matrix)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DIN )
    return -1; // invalid matrix

  mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[matrix];

  if( !m->num_rows )
    return -2; // no rows

  int row;
  for(row=0; row<MBNG_PATCH_NUM_MATRIX_ROWS_MAX; ++row) {
    // update selection pattern based on the matrix parameters
    u16 dout_value;
    if( m->num_rows <= 4 ) dout_value = selection_4rows[row];
    else if( m->num_rows <= 8 ) dout_value = selection_8rows[row];
    else dout_value = selection_16rows[row];

    if( m->flags.inverted_sel ) {
      dout_value ^= 0xffff;
    }

    if( m->sr_dout_sel1 ) {
      MIOS32_DOUT_PageSRSet(row+ 0, m->sr_dout_sel1-1, dout_value);
      MIOS32_DOUT_PageSRSet(row+16, m->sr_dout_sel1-1, dout_value);
    }

    if( m->sr_dout_sel2 ) {
      MIOS32_DOUT_PageSRSet(row+ 0, m->sr_dout_sel2-1, dout_value >> 8);
      MIOS32_DOUT_PageSRSet(row+16, m->sr_dout_sel2-1, dout_value >> 8);
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Set/Get pattern
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


s32 MBNG_MATRIX_LcMeterPatternSet(u8 pos, u16 pattern)
{
  if( pos >= 17 )
    return -2;

  lc_meter_pattern[pos] = pattern;

  return 0; // no error
}

u16 MBNG_MATRIX_LcMeterPatternGet(u8 pos)
{
  if( pos >= 17 )
    return 0x0000;
  
  return lc_meter_pattern[pos];
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a pin on the given DOUT matrix
//! The level ranges from 0..NUM_MATRIX_DIM_LEVELS-1
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PinSet(u8 matrix, u8 color, u16 pin, u8 level)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[matrix];

  // special handling for MAX72xx
  if( m->flags.max72xx_enabled ) {
    u16 num_pins = 64*MAX72XX_NUM_DEVICES_PER_CHAIN;
    if( pin >= num_pins )
      return -1; // invalid pin

    u8 chain = 0; // only a single chain is supported
    u8 device = pin / 64;
    u8 digit = ((pin % 64) / 8);
    u8 digit_mask = m->flags.mirrored_row ? (1 << (7-(pin % 8))) : (1 << (pin % 8));

    if( m->flags.inverted_row )
      level = level ? 0 : 1;

    // operation should be atomic
    MIOS32_IRQ_Disable();
    u8 value = MAX72XX_DigitGet(chain, device, digit);
    if( level )
      value |= digit_mask;
    else
      value &= ~digit_mask;
    MAX72XX_DigitSet(chain, device, digit, value);

    max72xx_update_digit_req |= (1 << digit);
    MIOS32_IRQ_Enable();

    return 0;
  }

  u8 row_size;
  switch( color ) {
  case 1:  row_size = m->sr_dout_g2 ? 16 : 8; break;
  case 2:  row_size = m->sr_dout_b2 ? 16 : 8; break;
  default: row_size = m->sr_dout_r2 ? 16 : 8; break;
  }

  if( pin >= row_size*m->num_rows )
    return -2; // pin is out-of-range

  int row    = pin / row_size;
  int column = pin % row_size;

  u8 sr;
  switch( color ) {
  case 1:  sr = (column >= 8) ? m->sr_dout_g2 : m->sr_dout_g1; break;
  case 2:  sr = (column >= 8) ? m->sr_dout_b2 : m->sr_dout_b1; break;
  default: sr = (column >= 8) ? m->sr_dout_r2 : m->sr_dout_r1; break;
  }

#if MIOS32_SRIO_NUM_DOUT_PAGES != 32
# error "Please adapt this code according to the different MIOS32_SRIO_NUM_DOUT_PAGES"
#endif

#if NUM_MATRIX_DIM_LEVELS != (MIOS32_SRIO_NUM_DOUT_PAGES/2)
# error "Please adapt this code according to the different NUM_MATRIX_DIM_LEVELS"
#endif
  if( sr && m->num_rows ) {
    u32 pin_offset = 8*(sr-1) + column;
    int i;
    for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; i+=m->num_rows) {
      int dout_value = level ? (i <= 2*level) : 0;
      if( m->flags.inverted_row )
	dout_value = dout_value ? 0 : 1;

      MIOS32_DOUT_PagePinSet(row + i, pin_offset, dout_value);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This help function transfers the pattern to DOUT registers
/////////////////////////////////////////////////////////////////////////////
static s32 Hlp_DOUT_PatternTransfer(u8 matrix, u8 color, u16 row, u16 matrix_pattern, u8 level)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DOUT )
    return -1; // invalid matrix

  mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[matrix];

  // special handling for MAX72xx
  if( m->flags.max72xx_enabled ) {
    u16 num_rows = 8*MAX72XX_NUM_DEVICES_PER_CHAIN;
    if( row >= num_rows )
      return -1; // invalid pin

    u8 chain = 0; // only a single chain is supported
    u8 device = row / 8;
    u8 digit = row % 8;

    u8 value = level ? matrix_pattern : 0x00; // only 8bit used!
    if( m->flags.mirrored_row ) {
      value = mios32_dout_reverse_tab[value];
    }

    if( m->flags.inverted_row )
      level = level ? 0 : 1;

    // operation should be atomic
    MIOS32_IRQ_Disable();
    MAX72XX_DigitSet(chain, device, digit, value);
    max72xx_update_digit_req |= (1 << digit);
    MIOS32_IRQ_Enable();

    return 0;
  }

  if( row >= m->num_rows )
    return -2; // ignore

  if( m->flags.mirrored_row ) {
    // mirror the two bytes (don't swap the bytes - should we provide an option for this as well?)
    matrix_pattern = ((u16)mios32_dout_reverse_tab[matrix_pattern >> 8] << 8) | (u16)mios32_dout_reverse_tab[matrix_pattern & 0xff];
  }

  u8 sr1, sr2;
  switch( color ) {
  case 1:  sr1 = m->sr_dout_g1; sr2 = m->sr_dout_g2; break;
  case 2:  sr1 = m->sr_dout_b1; sr2 = m->sr_dout_b2; break;
  default: sr1 = m->sr_dout_r1; sr2 = m->sr_dout_r2; break;
  }

  if( m->flags.inverted_row )
    matrix_pattern ^= 0xffff;

  u8 sr1_value = matrix_pattern;
  u8 sr2_value = matrix_pattern >> 8;

  if( m->num_rows ) {
    if( sr1 ) {
      u8 sr_offset = sr1-1;
      int i;
      for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; i+=m->num_rows) {
	MIOS32_DOUT_PageSRSet(row + i, sr_offset, (level && (i <= 2*level)) ? sr1_value : 0);
      }
    }

    if( sr2 ) {
      u8 sr_offset = sr2-1;
      int i;
      for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; i+=m->num_rows) {
	MIOS32_DOUT_PageSRSet(row + i, sr_offset, (level && (i <= 2*level)) ? sr2_value : 0);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a pattern DOUT matrix row
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet(u8 matrix, u8 color, u16 row, u16 value, u16 range, u8 pattern, u8 level)
{
  u32 scaled = ((MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)*value) / range;
  int pattern_ix;
  if( scaled <= 7 )
    pattern_ix = scaled;
  else if( value == (range/2) ) {
    pattern_ix = (MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)/2;
  } else {
    pattern_ix = scaled+1;
  }

  u16 matrix_pattern = dout_matrix_pattern[pattern][pattern_ix];
  Hlp_DOUT_PatternTransfer(matrix, color, row, matrix_pattern, level);

#if 0
  DEBUG_MSG("matrix=%d row=%d value=%d range=%d scaled=%d ix=%d led_row=0x%04x\n", matrix, row, value, range, scaled, pattern_ix, *led_row_ptr);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a pattern DOUT matrix row for the LC protocol
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet_LC(u8 matrix, u8 color, u16 row, u16 value, u8 level)
{
  u8 pattern = (value >> 4) & 0x3;
  u8 pattern_ix = value & 0x0f;
  if( pattern_ix >= ((MBNG_MATRIX_DOUT_NUM_PATTERN_POS-1)/2) )
    ++pattern_ix; // 'M' entry not supported...
  u8 center_led = value & 0x40;

  u16 matrix_pattern = dout_matrix_pattern[pattern][pattern_ix] | (center_led ? (1 << 11) : 0);
  Hlp_DOUT_PatternTransfer(matrix, color, row, matrix_pattern, level);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a pattern DOUT matrix row for a Meter of the LC protocol
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet_LCMeter(u8 matrix, u8 color, u16 row, u8 meter_value, u8 level)
{
  u8 pattern_ix = (meter_value >> 3) & 0x0f;
  u8 overload = meter_value & 0x80;

  u16 matrix_pattern = lc_meter_pattern[pattern_ix] | (overload ? lc_meter_pattern[16] : 0);
  Hlp_DOUT_PatternTransfer(matrix, color, row, matrix_pattern, level);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a pattern DOUT matrix row for a ASCII Digit
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_PatternSet_Digit(u8 matrix, u8 color, u16 row, u8 value, u8 level, u8 dot)
{
  u8 matrix_pattern = ~digit_patterns[value % 64] | (dot ? 0x80 : 00);

  // mirror pattern (because it's wrongly defined)
  matrix_pattern = mios32_dout_reverse_tab[matrix_pattern];

  Hlp_DOUT_PatternTransfer(matrix, color, row, matrix_pattern, level);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function gets the DIN values of the selected row.
//! It should be called from the APP_SRIO_ServiceFinish() hook
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_GetRow(void)
{
  u8 selected_row = MIOS32_SRIO_DoutPageGet() % MBNG_PATCH_NUM_MATRIX_ROWS_MAX;

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
	u8 sr_value8 = MIOS32_DIN_SRGet(m->sr_din1-1);
	if( m->flags.mirrored_row ) {
	  sr_value8 = mios32_dout_reverse_tab[sr_value8];
	}
	sr_value = (u16)sr_value8;
	if( m->flags.inverted_row )
	  sr_value ^= 0x00ff;
      } else {
	if( !m->flags.inverted_row )
	  sr_value = 0x00ff;
      }

      if( m->sr_din2 ) {
	MIOS32_DIN_SRChangedGetAndClear(m->sr_din2-1, 0xff); // ensure that change won't be propagated to normal DIN handler
	u8 sr_value8 = MIOS32_DIN_SRGet(m->sr_din2-1);
	if( m->flags.mirrored_row ) {
	  sr_value8 = mios32_dout_reverse_tab[sr_value8];
	}
	sr_value |= (u16)sr_value8 << 8;
	if( m->flags.inverted_row )
	  sr_value ^= 0xff00;
      } else {
	if( !m->flags.inverted_row )
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
//! This function should be called from a task to check for button changes
//! periodically. Events (change from 0->1 or from 1->0) will be notified 
//! to MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_ButtonHandler(void)
{
  int matrix, row;
  mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DIN; ++matrix, ++m) {

    u8 row_size = m->sr_din2 ? 16 : 8;
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
      for(pin=0; pin<row_size; ++pin, mask <<= 1)
	if( changed & mask )
	  MBNG_MATRIX_NotifyToggle(matrix, (row * row_size) + pin, (button_row[matrix][row] & mask) ? 1 : 0);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is periodically called from app.c to update the
//! MAX72xx chain
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_MAX72xx_Update(void)
{
  int matrix;
  mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
  for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
    if( !m->flags.max72xx_enabled )
      continue;

    u8 chain = 0; // only a single chain is supported

    // MAX72xx found (we only support a single MAX72xx - ignore the remaining matrix configurations)
    if( max72xx_init_req ) {
      max72xx_init_req = 0;
      MAX72XX_Init(0);

      if( m->flags.inverted_row ) {
	int digit;
	for(digit=0; digit<MAX72XX_NUM_DEVICES_PER_CHAIN; ++digit) {
	  int i;
	  for(i=0; i<8; ++i) {
	    MAX72XX_DigitSet(chain, digit, 0, 0xff);
	  }
	}
      }

      // update all digits
      max72xx_update_digit_req = ~0;
    }

    if( max72xx_update_digit_req ) {
      // should be atomic
      MIOS32_IRQ_Disable();
      u16 update_req = max72xx_update_digit_req;
      max72xx_update_digit_req = 0;
      MIOS32_IRQ_Enable();

      int digit;
      for(digit=0; digit<MAX72XX_NUM_DEVICES_PER_CHAIN; ++digit, update_req >>= 1) {
	if( update_req & 1 ) {
	  MAX72XX_UpdateDigit(chain, digit);
	}
      }
    }

    return 0;
  }

  return -1; // no MAX72xx assigned
}


/////////////////////////////////////////////////////////////////////////////
//! This function will be called by MBNG_MATRIX_ButtonHandler whenever
//! a button has been toggled
//! pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_MATRIX_NotifyToggle(u8 matrix, u32 pin, u32 pin_value)
{
  if( matrix >= MBNG_PATCH_NUM_MATRIX_DIN )
    return -1; // invalid matrix

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MIOS32_MIDI_SendDebugMessage("MBNG_MATRIX_NotifyToggle(%d, %d, %d)\n", matrix, pin+1, pin_value);
  }

  // "normal" buttons emulated?
  if( mbng_patch_matrix_din[matrix].button_emu_id_offset ) // -> DIN handler
    return MBNG_DIN_NotifyToggle(pin + mbng_patch_matrix_din[matrix].button_emu_id_offset - 1, pin_value);

  u16 hw_id = MBNG_EVENT_CONTROLLER_BUTTON_MATRIX + matrix + 1;

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to BUTTON_MATRIX hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    // EXTRA for matrix: store pin number for MBNG_EVENT_Item* functions
    item.matrix_pin = pin;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    // button depressed?
    u8 depressed = pin_value ? 1 : 0;
    item.value = depressed ? item.min : item.max;

    if( MBNG_EVENT_NotifySendValue(&item) == 2 )
      break; // stop has been requested
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DIN_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DIN_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MATRIX_DOUT_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MATRIX_DOUT_NotifyReceivedValue(%d, %d, %d)\n", hw_id & 0xfff, item->matrix_pin+1, item->value);
  }

#if MBNG_PATCH_NUM_MATRIX_ROWS_MAX != 16
# error "not prepared for != 16 rows - id assignments have to be adapted!"
#endif

  u16 hw_id_ix = hw_id & 0xfff;
  if( !item->flags.led_matrix_pattern ) {
    if( hw_id_ix && hw_id_ix < MBNG_PATCH_NUM_MATRIX_DOUT ) {
      u8 matrix = hw_id_ix - 1;

      u8 dout_value;
      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
      if( map_len > 0 ) {
	int map_ix = item->value;
	if( map_ix >= map_len )
	  map_ix = map_len - 1;
	dout_value = map_values[map_ix];
      } else {
	if( item->min == item->max ) {      
	  dout_value = (item->value == item->min) ? (NUM_MATRIX_DIM_LEVELS-1) : 0;
	} else {
	  int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
	  if( !item->flags.dimmed || item->rgb.ALL ) {
	    if( item->min <= item->max )
	      dout_value = ((item->value - item->min) >= (range/2)) ? (NUM_MATRIX_DIM_LEVELS-1) : 0;
	    else
	      dout_value = ((item->value - item->max) >= (range/2)) ? (NUM_MATRIX_DIM_LEVELS-1) : 0;
	  } else {
	    int steps = range / NUM_MATRIX_DIM_LEVELS;
	    dout_value = (item->min <= item->max) ? ((item->value - item->min) / steps) : ((item->value - item->max) / steps);
	  }
	}
      }

      if( item->rgb.ALL ) {
	MBNG_MATRIX_DOUT_PinSet(matrix, 0, item->matrix_pin, dout_value ? item->rgb.r : 0);
	MBNG_MATRIX_DOUT_PinSet(matrix, 1, item->matrix_pin, dout_value ? item->rgb.g : 0);
	MBNG_MATRIX_DOUT_PinSet(matrix, 2, item->matrix_pin, dout_value ? item->rgb.b : 0);
      } else {
	MBNG_MATRIX_DOUT_PinSet(matrix, item->flags.colour, item->matrix_pin, dout_value);
      }
    }
  } else {
    int matrix = (hw_id_ix-1) / MBNG_PATCH_NUM_MATRIX_ROWS_MAX;
    int row = (hw_id_ix-1) % MBNG_PATCH_NUM_MATRIX_ROWS_MAX;
    // this is actually a dirty solution: without LED patterns we address the matrix directly with 1..8,
    // and here we multiply by 16 since we've connected 16 LED Rings... :-/

    if( item->flags.led_matrix_pattern == MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO ) {
      if( item->rgb.ALL ) {
	MBNG_MATRIX_DOUT_PatternSet_LC(matrix, item->flags.colour, row, item->value, item->rgb.r);
	MBNG_MATRIX_DOUT_PatternSet_LC(matrix, item->flags.colour, row, item->value, item->rgb.g);
	MBNG_MATRIX_DOUT_PatternSet_LC(matrix, item->flags.colour, row, item->value, item->rgb.b);
      } else {
	MBNG_MATRIX_DOUT_PatternSet_LC(matrix, item->flags.colour, row, item->value, NUM_MATRIX_DIM_LEVELS-1);
      }

    } else if( item->flags.led_matrix_pattern == MBNG_EVENT_LED_MATRIX_PATTERN_LC_DIGIT ) {
      int value = item->value & 0x3f;
      u8 dot = (item->value & 0x40) ? 1 : 0;

      if( item->rgb.ALL ) {
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.r, dot);
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.g, dot);
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.b, dot);
      } else {
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, NUM_MATRIX_DIM_LEVELS-1, dot);
      }

    } else if( item->flags.led_matrix_pattern >= MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT1 &&
	       item->flags.led_matrix_pattern <= MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT5 ) {
      int value = item->value;
      switch( item->flags.led_matrix_pattern ) {
      //case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT1: // nothing to do
      case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT2: value = value / 10; break;
      case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT3: value = value / 100; break;
      case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT4: value = value / 1000; break;
      case MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT5: value = value / 10000; break;
      }
      value %= 10;
      value += 0x30; // numbers
      u8 dot = 0;

      if( item->rgb.ALL ) {
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.r, dot);
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.g, dot);
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, item->rgb.b, dot);
      } else {
	MBNG_MATRIX_DOUT_PatternSet_Digit(matrix, item->flags.colour, row, value, NUM_MATRIX_DIM_LEVELS-1, dot);
      }

    } else if( item->flags.led_matrix_pattern >= MBNG_EVENT_LED_MATRIX_PATTERN_1 &&
	       item->flags.led_matrix_pattern <= MBNG_EVENT_LED_MATRIX_PATTERN_4 ) {
      u8 pattern = item->flags.led_matrix_pattern - MBNG_EVENT_LED_MATRIX_PATTERN_1;

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

      if( item->rgb.ALL ) {
	MBNG_MATRIX_DOUT_PatternSet(matrix, 0, row, saturated_value, range, pattern, item->rgb.r);
	MBNG_MATRIX_DOUT_PatternSet(matrix, 1, row, saturated_value, range, pattern, item->rgb.g);
	MBNG_MATRIX_DOUT_PatternSet(matrix, 2, row, saturated_value, range, pattern, item->rgb.b);
      } else {
	MBNG_MATRIX_DOUT_PatternSet(matrix, item->flags.colour, row, saturated_value, range, pattern, NUM_MATRIX_DIM_LEVELS-1);
      }
    }
  }

  return 0; // no error
}


//! \}
