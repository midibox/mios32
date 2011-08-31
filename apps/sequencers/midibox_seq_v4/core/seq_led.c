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

#include <blm_x.h>

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

  if( pin < 196 )
    return BLM_X_LEDSet(pin-128, 0, value);

  return -1; // pin not available
}


/////////////////////////////////////////////////////////////////////////////
// Sets a DOUT SR (8 LEDs), differs between LEDs directly connected to DOUT SR
// (sr=0..15) and LEDS which are connected to a 8x8 matrix (sr=16..23)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_SRSet(u32 sr, u8 value)
{
  if( !sr )
    return -1; // SR not available

  if( sr < 16 )
    return MIOS32_DOUT_SRSet(sr, value);

  if( sr < 24 )
    return BLM_X_LEDSRSet(sr-16, 0, value);

  return -1; // SR not available
}

/////////////////////////////////////////////////////////////////////////////
// returns the value of a DOUT SR (8 LEDs), differs between LEDs directly connected to DOUT SR
// (sr=0..15) and LEDS which are connected to a 8x8 matrix (sr=16..23)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_SRGet(u32 sr)
{
  if( !sr )
    return 0; // SR not available... return 0

  if( sr < 16 )
    return MIOS32_DOUT_SRGet(sr);

  if( sr < 24 )
    return BLM_X_LEDSRGet(sr-16, 0);

  return 0; // SR not available... return 0
}
