// $Id$
//! \defgroup MIOS32_IIC
//!
//! IIC driver for MIOS32
//!
//! Not adapted to LPC17xx yet
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 *  IRQ handlers reworked by Matthias Mächler (juli 2009)
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_IIC)


/////////////////////////////////////////////////////////////////////////////
//! Initializes IIC driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  return -1; // not supported yet
}

/////////////////////////////////////////////////////////////////////////////
//! Semaphore handling: requests the IIC interface
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \param[in] semaphore_type is either IIC_Blocking or IIC_Non_Blocking
//! \return Non_Blocking: returns -1 to request a retry
//! \return 0 if IIC interface free
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferBegin(u8 iic_port, mios32_iic_semaphore_t semaphore_type)
{
  return -1; // not supported yet
}

/////////////////////////////////////////////////////////////////////////////
//! Semaphore handling: releases the IIC interface for other tasks
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferFinished(u8 iic_port)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the last transfer error<BR>
//! Will be updated by MIOS32_IIC_TransferCheck(), so that the error status
//! doesn't get lost (the check function will return 0 when called again)<BR>
//! Will be cleared when a new transfer has been started successfully
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return last error status
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_LastErrorGet(u8 iic_port)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Checks if transfer is finished
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return 0 if no ongoing transfer
//! \return 1 if ongoing transfer
//! \return < 0 if error during transfer
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferCheck(u8 iic_port)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Waits until transfer is finished
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return 0 if no ongoing transfer
//! \return < 0 if error during transfer
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferWait(u8 iic_port)
{
  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Starts a new transfer. If this function is called during an ongoing
//! transfer, we wait until it has been finished and setup the new transfer
//! \param[in] transfer type:<BR>
//! <UL>
//!   <LI>IIC_Read: a common Read transfer
//!   <LI>IIC_Write: a common Write transfer
//!   <LI>IIC_Read_AbortIfFirstByteIs0: used to poll MBHP_IIC_MIDI: aborts transfer
//!         if the first received byte is 0
//!   <LI>IIC_Write_WithoutStop: don't send stop condition after transfer to allow
//!         a restart condition (e.g. used to access EEPROMs)
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \param[in] address of IIC device (bit 0 always cleared)
//! \param[in] *buffer pointer to transmit/receive buffer
//! \param[in] len number of bytes which should be transmitted/received
//! \return 0 no error
//! \return < 0 on errors, if MIOS32_IIC_ERROR_PREV_OFFSET is added, the previous
//!      transfer got an error (the previous task didn't use \ref MIOS32_IIC_TransferWait
//!      to poll the transfer state)
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Transfer(u8 iic_port, mios32_iic_transfer_t transfer, u8 address, u8 *buffer, u16 len)
{
  return -1; // not supported yet
}

//! \}

#endif /* MIOS32_DONT_USE_IIC */
