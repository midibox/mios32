// $Id$
/*
 * DOUT functions for MIOS32
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_DOUT)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes DOUT driver
// IN: <mode>: currently only mode 0 supported
//             later we could provide operation modes
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_Init(u32 mode)
{
  u8 i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear DOUT part of SRIO chain
  // TODO: here we could provide an option to invert the default value
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    mios32_srio_dout[i] = 0;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns value from a DOUT Pin
// IN: pin number in <pin>
// OUT: 1 if pin is Vss, 0 if pin is 0V, -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinGet(u32 pin)
{
  // check if pin available
  if( pin/8 >= MIOS32_SRIO_NUM_SR )
    return -1;

  // NOTE: DOUT SR registers in reversed (!) order (since DMA doesn't provide a decrement address function)
  return (mios32_srio_dout[MIOS32_SRIO_NUM_SR - (pin>>3) - 1] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// set pin to 0 or Vss
// IN: pin number in <pin>, pin value in <value>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinSet(u32 pin, u32 value)
{
  // check if pin available
  if( pin/8 >= MIOS32_SRIO_NUM_SR )
    return -1;

  if( value )
    mios32_srio_dout[MIOS32_SRIO_NUM_SR - (pin>>3) - 1] |= (u8)(1 << (pin&7));
  else
    mios32_srio_dout[MIOS32_SRIO_NUM_SR - (pin>>3) - 1] &= ~(u8)(1 << (pin&7));

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns value of DOUT shift register
// IN: shift register number in <sr>
// OUT: 8bit value of shift register, -1 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= MIOS32_SRIO_NUM_SR )
    return -1;

  return mios32_srio_dout[MIOS32_SRIO_NUM_SR - sr - 1];
}

/////////////////////////////////////////////////////////////////////////////
// sets value of DOUT shift register
// IN: shift register number in <sr>, 8bit shift register value in <value>
// OUT: returns -1 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_SRSet(u32 sr, u8 value)
{
  // check if SR available
  if( sr >= MIOS32_SRIO_NUM_SR )
    return -1;

  mios32_srio_dout[MIOS32_SRIO_NUM_SR - sr - 1] = value;

  return 0;
}

#endif /* MIOS32_DONT_USE_DOUT */
