// $Id$
/*
 * DOUT access functions for MIDIbox NG
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
#include "mbng_dout.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"
#include "mbng_matrix.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DOUT handler
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
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DOUT_NotifyReceivedValue(%d, %d)\n", hw_id, item->value);
  }

  int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
  u8 dout_value;
  if( item->flags.DOUT.radio_group ) {
    if( item->min <= item->max )
      dout_value = item->value >= item->min && item->value <= item->max;
    else
      dout_value = item->value >= item->max && item->value <= item->min;
  } else if( item->min == item->max ) {
    dout_value = item->value == item->min;
  } else
    dout_value = (item->min <= item->max) ? ((item->value - item->min) >= (range/2)) : ((item->value - item->max) >= (range/2));

  // check if any matrix emulates the LEDs
  u8 emulated = 0;
  {
    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
      if( m->led_emu_id_offset && m->sr_dout_r1 && hw_id >= m->led_emu_id_offset ) {

	u8 row_size = m->sr_dout_r2 ? 16 : 8; // we assume that the same condition is valid for dout_g2 and dout_b2
	if( hw_id < (m->num_rows * (m->led_emu_id_offset + row_size)) ) {
	  emulated = 1;
	  u16 tmp_hw_id = item->hw_id;
	  item->hw_id = matrix + 1;
	  item->matrix_pin = hw_id - m->led_emu_id_offset;
	  MBNG_MATRIX_DOUT_NotifyReceivedValue(item);
	  item->hw_id = tmp_hw_id;
	}
      }
    }
  }

  if( !emulated ) {
    // set LED
    MIOS32_DOUT_PinSet(hw_id - 1, dout_value);
  }

  return 0; // no error
}
