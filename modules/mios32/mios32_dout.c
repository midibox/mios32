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
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode
  
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
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( pin/8 >= MIOS32_SRIO_NUM_MAX )
    return -1;

  return (mios32_srio_dout[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// set pin to 0 or Vss
// IN: pin number in <pin>, pin value in <value>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinSet(u32 pin, u32 value)
{
  // check if pin available
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( pin/8 >= MIOS32_SRIO_NUM_MAX )
    return -1;

  if( value )
    mios32_srio_dout[pin >> 3] |= (u8)(1 << (pin&7));
  else
    mios32_srio_dout[pin >> 3] &= ~(u8)(1 << (pin&7));

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
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( sr >= MIOS32_SRIO_NUM_MAX )
    return -1;

  return mios32_srio_dout[sr];
}

/////////////////////////////////////////////////////////////////////////////
// sets value of DOUT shift register
// IN: shift register number in <sr>, 8bit shift register value in <value>
// OUT: returns -1 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_SRSet(u32 sr, u8 value)
{
  // check if SR available
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( sr >= MIOS32_SRIO_NUM_MAX )
    return -1;

  mios32_srio_dout[sr] = value;

  return 0;
}
