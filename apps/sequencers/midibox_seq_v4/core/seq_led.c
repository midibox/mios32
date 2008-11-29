// $Id$
/*
 * LED Handlers
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

#if DEFAULT_SRM_ENABLED
#include <blm8x8.h>
#endif

#include "seq_led.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets a LED, differs between LEDs directly connected to DOUT pins 
// (pin = 0..127) and LEDS which are connected to a 8x8 matrix (pin = 128..195)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_PinSet(u32 pin, u32 value)
{
  if( pin < 128 )
    return MIOS32_DOUT_PinSet(pin, value);

#if DEFAULT_SRM_ENABLED
  if( pin < 196 )
    BLM8X8_DOUT_PinSet(pin-128, value);
#endif

  return -1; // pin not available
}


/////////////////////////////////////////////////////////////////////////////
// Sets a DOUT SR (8 LEDs), differs between LEDs directly connected to DOUT SR
// (sr=0..15) and LEDS which are connected to a 8x8 matrix (sr=16..23)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_SRSet(u32 sr, u8 value)
{
  if( sr < 16 )
    return MIOS32_DOUT_SRSet(sr, value);

#if DEFAULT_SRM_ENABLED
  if( sr < 24 )
    BLM8X8_DOUT_SRSet(sr-16, value);
#endif

  return -1; // SR not available
}
