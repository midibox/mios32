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

static u32 led_states[MBNG_PATCH_NUM_DOUT/32];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DOUT handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_DOUT/32; ++i)
    led_states[i] = 0;

  int pin;
  for(pin=0; pin<8*MIOS32_SRIO_NUM_SR; ++pin)
    MIOS32_DOUT_PinSet(pin, 0x00);
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int dout_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DOUT_NotifyReceivedValue(%d, %d)\n", dout_subid, value);
  }

  int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
  u8 dout_value;
  if( item->flags.DOUT.radio_group ) {
    if( item->min <= item->max )
      dout_value = value >= item->min && value <= item->max;
    else
      dout_value = value >= item->max && value <= item->min;
  } else if( item->min == item->max ) {
    dout_value = value == item->min;
  } else
    dout_value = (item->min <= item->max) ? ((value - item->min) >= (range/2)) : ((value - item->max) >= (range/2));

  // set state
  if( dout_subid && dout_subid <= MBNG_PATCH_NUM_DOUT ) {
    int ix = (dout_subid-1) / 32;
    int mask = (1 << ((dout_subid-1) % 32));
    if( dout_value )
      led_states[ix] |= mask;
    else
      led_states[ix] &= ~mask;
  }

  // check if any matrix emulates the LEDs
  u8 emulated = 0;
  {
    int matrix;
    mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[0];
    for(matrix=0; matrix<MBNG_PATCH_NUM_MATRIX_DOUT; ++matrix, ++m) {
      if( m->led_emu_id_offset && m->sr_dout_r1 && dout_subid >= m->led_emu_id_offset ) {
	int num_pins = 8*m->num_rows;
	if( m->sr_dout_r2 )
	  num_pins *= 2;

	if( dout_subid < (m->led_emu_id_offset + num_pins) ) {
	  emulated = 1;

	  u8 color = 0; // TODO
	  MBNG_MATRIX_DOUT_PinSet(matrix, color, dout_subid - m->led_emu_id_offset, dout_value);
	}
      }
    }
  }

  if( !emulated ) {
    // set LED
    MIOS32_DOUT_PinSet(dout_subid-1, dout_value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the value of a given item ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_GetCurrentValueFromId(mbng_event_item_id_t id)
{
  int dout_subid = id & 0xfff;

  if( !dout_subid || dout_subid > MBNG_PATCH_NUM_DOUT )
    return -1; // item not mapped to hardware

  int ix = (dout_subid-1) / 32;
  int mask = (1 << ((dout_subid-1) % 32));
  return (led_states[ix] & mask) ? 1 : 0;
}
