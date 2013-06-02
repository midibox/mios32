// $Id$
//! \defgroup MIOS32_USB_COM
//!
//! USB COM layer for MIOS32
//! 
//! Not supported for STM32F4 (yet)
//!
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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
// it's disabled by default, since Windows doesn't allow to use USB MIDI and CDC in parallel!
#if defined(MIOS32_USE_USB_COM)


/////////////////////////////////////////////////////////////////////////////
//! Initializes USB COM layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_Init(u32 mode)
{
  return -1; // not supported
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] connected connection status (1 if connected)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_ChangeConnectionState(u8 connected)
{
  return -1; // not supported
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB COM interface
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_CheckAvailable(void)
{
  return 0;
}

//! \}

#endif /* MIOS32_USE_USB_COM */
