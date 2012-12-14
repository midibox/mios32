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


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DOUT handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // set all DOUT pins depending on polarity
  int pin;
  u8 pin_value = 0; // mbng_patch_cfg.flags.INVERSE_DOUT; // TODO
  for(pin=0; pin<8*MIOS32_SRIO_NUM_SR; ++pin)
    MIOS32_DOUT_PinSet(pin, pin_value);
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int led_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DOUT_NotifyReceivedValue(%d, %d)\n", led_ix, value);
  }

  // print label
  MBNG_LCD_PrintItemLabel(item, value);

  int range = item->max - item->min + 1;
  if( range < 0 ) range *= -1;
  u8 led_value = value >= (range/2);

  // set LED
  MIOS32_DOUT_PinSet(led_ix-1, led_value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called on patch configuration changes to invert DOUT pins if
// required
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_UpdatePolarity(u8 old_polarity, u8 new_polarity)
{
  if( old_polarity != new_polarity ) {
    int pin;
    for(pin=0; pin<MBNG_PATCH_NUM_DOUT; ++pin)
      MIOS32_DOUT_PinSet(pin, MIOS32_DOUT_PinGet(pin) ? 0 : 1);
  }

  return 0; // no error
}
