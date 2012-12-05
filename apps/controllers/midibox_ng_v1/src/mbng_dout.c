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
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DOUT handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

#if 0
  // set all DOUT pins depending on polarity
  int pin;
  u8 pin_value = mbng_patch_cfg.flags.INVERSE_DOUT;
  for(pin=0; pin<MBNG_PATCH_NUM_DOUT; ++pin)
    MIOS32_DOUT_PinSet(pin, pin_value);
#endif
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sets a DOUT pin with configured polarity (and mode)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DOUT_PinSet(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_DOUT )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MBNG_DOUT_PinSet(%d, %d)\n", pin, pin_value);
#endif

#if 0
  // DOUT assigned to program change?
  if( mbng_patch_cfg.flags.ALT_PROGCHNG &&
      ((mbng_patch_dout[pin].evnt0 >> 4) | 0x8) == ProgramChange ) {

    if( !pin_value )
      return 0; // ignore

    // set pin
    if( mbng_patch_cfg.flags.INVERSE_DOUT )
      pin_value = pin_value ? 0 : 1;

    MIOS32_DOUT_PinSet(pin, pin_value);

    // search for DOUTs assigned to program change and same channel
    int i;
    mbng_patch_dout_entry_t *dout_cfg = (mbng_patch_dout_entry_t *)&mbng_patch_dout[0];
    u8 search_event = mbng_patch_dout[pin].evnt0;
    for(i=0; i<MBNG_PATCH_NUM_DOUT; ++i, ++dout_cfg) {
      if( i != pin && dout_cfg->evnt0 == search_event )
	MIOS32_DOUT_PinSet(i, mbng_patch_cfg.flags.INVERSE_DOUT ? 1 : 0);
    }
  } else {
    if( mbng_patch_cfg.flags.INVERSE_DOUT )
      pin_value = pin_value ? 0 : 1;

    MIOS32_DOUT_PinSet(pin, pin_value);
  }
#endif


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
