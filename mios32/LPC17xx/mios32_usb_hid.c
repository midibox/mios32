// $Id:
//! \defgroup MIOS32_USB_HID
//!
//! USB HID layer for MIOS32
//! 
//! Only supported for STM32F4 
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB_HID)


/////////////////////////////////////////////////////////////////////////////
//! Initializes USB COM layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HS_Init(u32 mode)
{
  return -1; // not supported
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] connected connection status (1 if connected)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HS_ChangeConnectionState(u8 connected)
{
  return -1; // not supported
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB COM interface
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HS_CheckAvailable(void)
{
  return -1; // not supported
}


s32 MIOS32_USB_HS_Process(void){
  return -1; // not supported
}



//! \}

#endif /* MIOS32_DONT_USE_USB_HID */
