// $Id$
//! \defgroup MIOS32_USB
//!
//! USB driver for MIOS32
//! 
//! Based on driver included in STM32 USB library
//! Some code copied and modified from Virtual_COM_Port demo
//! 
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
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
#if !defined(MIOS32_DONT_USE_USB)

#include <usbd_usr.h>
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
//! Initializes USB interface
//! \param[in] mode
//!   <UL>
//!     <LI>if 0, USB peripheral won't be initialized if this has already been done before
//!     <LI>if 1, USB peripheral re-initialisation will be forced
//!     <LI>if 2, USB peripheral re-initialisation will be forced, STM32 driver hooks won't be overwritten.<BR>
//!         This mode can be used for a local USB driver which installs it's own hooks during runtime.<BR>
//!         The application can switch back to MIOS32 drivers (e.g. MIOS32_USB_MIDI) by calling MIOS32_USB_Init(1)
//!   </UL>
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Init(u32 mode)
{
  return -1; // TODO
}


/////////////////////////////////////////////////////////////////////////////
//! Allows to query, if the USB interface has already been initialized.<BR>
//! This function is used by the bootloader to avoid a reconnection, it isn't
//! relevant for typical applications!
//! \return 1 if USB already initialized, 0 if not initialized
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_IsInitialized(void)
{
  return 0; // TODO
}


/////////////////////////////////////////////////////////////////////////////
//! \returns != 0 if a single USB port has been forced in the bootloader
//! config section
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_ForceSingleUSB(void)
{
  u8 *single_usb_confirm = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM;
  u8 *single_usb = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB;
  if( *single_usb_confirm == 0x42 && *single_usb < 0x80 )
    return *single_usb;

  return 0;
}

//! \}

#endif /* MIOS32_DONT_USE_USB */
