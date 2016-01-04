// $Id$
//! \defgroup MBNG_DOUT
//! DOUT access functions for MIDIbox NG
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

#include "app.h"
#include "mbng_dout.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_matrix.h"


/////////////////////////////////////////////////////////////////////////////
//! local defines
/////////////////////////////////////////////////////////////////////////////

#define NUM_DIM_LEVELS 16

/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

static const u32 dim_pattern[NUM_DIM_LEVELS] = {
  0x00000000, // 0
  0x00000001, // 1
  0x00010001, // 2
  0x01010101, // 3
  0x01010103, // 4
  0x01030103, // 5
  0x03030303, // 6
  0x03130313, // 7
  0x13131313, // 8
  0x33333333, // 9
  0x33373337, // 10
  0x37373737, // 11
  0x77777777, // 12
  0x777f777f, // 13
  0x7f7f7f7f, // 14
  0xffffffff, // 15
};


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the DOUT handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int pin;
  for(pin=0; pin<8*MIOS32_SRIO_NUM_SR; ++pin)
    MIOS32_DOUT_PinSet(pin, 0x00);
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DOUT_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // check if any matrix emulates the LEDs
  u8 emulated = 0;
  {
    u16 hw_id_ix = hw_id & 0xfff;
    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
      if( m->led_emu_id_offset && (m->sr_dout_r1 || m->flags.max72xx_enabled) && hw_id_ix >= m->led_emu_id_offset ) {

	u8 row_size = m->sr_dout_r2 ? 16 : 8; // we assume that the same condition is valid for dout_g2 and dout_b2
	if( hw_id_ix < (m->led_emu_id_offset + (m->num_rows * row_size)) ) {
	  emulated = 1;
	  u16 tmp_hw_id = item->hw_id;
	  item->hw_id = MBNG_EVENT_CONTROLLER_LED_MATRIX + matrix + 1;
	  item->matrix_pin = hw_id_ix - m->led_emu_id_offset;
	  MBNG_MATRIX_DOUT_NotifyReceivedValue(item);
	  item->hw_id = tmp_hw_id;
	}
      }
    }
  }

  if( !emulated ) {
    u16 dout_value = item->value;

    s32 mapped_value;
    if( (mapped_value=MBNG_EVENT_MapValue(item->map, dout_value, 0, 0)) >= 0 ) {
      dout_value = mapped_value;
    } else {
      int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
      if( item->flags.radio_group ) {
	if( item->min <= item->max )
	  dout_value = item->value >= item->min && item->value <= item->max;
	else
	  dout_value = item->value >= item->max && item->value <= item->min;
      } else if( item->min == item->max ) {      
	dout_value = (item->value == item->min) ? (NUM_DIM_LEVELS-1) : 0;
      } else {
	if( !item->flags.dimmed || item->rgb.ALL ) {
	  if( item->min <= item->max )
	    dout_value = ((item->value - item->min) >= (range/2)) ? (NUM_DIM_LEVELS-1) : 0;
	  else
	    dout_value = ((item->value - item->max) >= (range/2)) ? (NUM_DIM_LEVELS-1) : 0;
	} else {
	  int steps = range / NUM_DIM_LEVELS;
	  dout_value = (item->min <= item->max) ? ((item->value - item->min) / steps) : ((item->value - item->max) / steps);
	}
      }
    }

    // set LED
    int pin = (hw_id & 0xfff) - 1;
    u8 inverted = item->custom_flags.DOUT.inverted;

    if( item->rgb.ALL ) {
      u8 level = (item->flags.colour == 1) ? item->rgb.g : (item->flags.colour == 2) ? item->rgb.b : item->rgb.r;
      u32 pattern = dout_value ? dim_pattern[level] : 0;
      int i;
      for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; ++i, pattern >>= 1) {
	u8 value = pattern & 1;
	if( inverted )
	  value ^= 1;
	MIOS32_DOUT_PagePinSet(i, pin, value);
      }
    } else if( !item->flags.dimmed ) {
      if( inverted )
	dout_value = dout_value ? 0 : 1;
      MIOS32_DOUT_PinSet(pin, dout_value);
    } else {
      u32 pattern = dim_pattern[(dout_value < NUM_DIM_LEVELS) ? dout_value : (NUM_DIM_LEVELS-1)];
      int i;
      for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; ++i, pattern >>= 1) {
	u8 value = pattern & 1;
	if( inverted )
	  value ^= 1;
	MIOS32_DOUT_PagePinSet(i, pin, value);
      }
    }
  }

  return 0; // no error
}

//! \}
