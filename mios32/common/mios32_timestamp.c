// $Id$
//! \defgroup MIOS32_TIMESTAMP
//!
//! mS accurate Timestamp for MIOS32
//! \{
/* ==========================================================================
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
#if !defined(MIOS32_DONT_USE_TIMESTAMP)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 timestamp;


/////////////////////////////////////////////////////////////////////////////
//! Resets the timestamp
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMESTAMP_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  timestamp = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Increments the timestamp, typically based on the FreeRTOS clock (1 mS)
//!
//! \note this function is called from vApplicationTickHook in main.c
//! Don't call it from your application.
//!
//! \return number of SRs
/////////////////////////////////////////////////////////////////////////////
u8 MIOS32_TIMESTAMP_Inc(void)
{
  ++timestamp;
}


/////////////////////////////////////////////////////////////////////////////
//! Use this function to get the current timestamp
//!
//! \return the current timestamp
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMESTAMP_Get(void)
{
  return timestamp;
}


/////////////////////////////////////////////////////////////////////////////
//! Use this function to get the delay which has passed between a given and
//! and current timestamp.
//!
//! Usage Example:
//! \code
//!   u32 captured_timestamp = MIOS32_TIMESTAMP_GetDelay();
//!   // ...
//!   // ... do something ...
//!   // ...
//!   u32 delay_in_ms = MIOS32_TIMESTAMP_GetDelay(captured_timestamp);
//!   MIOS32_MIDI_SendDebugMessage("Delay: %d mS\n", delay_in_ms);
//! \endcode
//! \return the delay between the given and the current timestamp
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMESTAMP_GetDelay(u32 captured_timestamp)
{
  // will automatically roll over:
  // e.g. 0x00000010 - 0xfffffff0 = 0x00000020
  return timestamp - captured_timestamp;
}

//! \}

#endif /* MIOS32_DONT_USE_TIMESTAMP */

