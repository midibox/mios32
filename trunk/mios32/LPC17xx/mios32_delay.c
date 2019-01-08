// $Id$
//! \defgroup MIOS32_DELAY
//!
//! Delay functions for MIOS32
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
#if !defined(MIOS32_DONT_USE_DELAY)

// RIT clocked at CCLK/4 MHz
#define RIT_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)


/////////////////////////////////////////////////////////////////////////////
//! Initializes the Timer used by MIOS32_DELAY functions<BR>
//! This function has to be executed before wait functions are used
//! (already done in main.c of the programming model)
//!
//! Currently the freerunning timer (RIT) is allocated by MIOS32_DELAY functions
//!
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DELAY_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // enable timer
  LPC_RIT->RICTRL = (1 << 3);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Waits for a specific number of uS<BR>
//! Example:<BR>
//! \code
//!   // wait for 500 uS
//!   MIOS32_DELAY_Wait_uS(500);
//! \endcode
//! \param[in] uS delay (1..65535 microseconds)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DELAY_Wait_uS(u16 uS)
{
  u32 uS_ticks = uS * (RIT_PERIPHERAL_FRQ / 1000000);
  u32 start    = LPC_RIT->RICOUNTER;

  // note that this event works on 32bit counter wrap-arounds
  while( (LPC_RIT->RICOUNTER - start) <= uS_ticks );

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_DELAY */
