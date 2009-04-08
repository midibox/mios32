// $Id$
//! \defgroup MIOS32_IIC_BS
//!
//! IIC BankStick layer for MIOS32
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
#if !defined(MIOS32_DONT_USE_IIC_BS)

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// bankstick sizes
static const s32 bs_size[8] = {
  MIOS32_IIC_BS0_SIZE,
  MIOS32_IIC_BS1_SIZE,
  MIOS32_IIC_BS2_SIZE,
  MIOS32_IIC_BS3_SIZE,
  MIOS32_IIC_BS4_SIZE,
  MIOS32_IIC_BS5_SIZE,
  MIOS32_IIC_BS6_SIZE,
  MIOS32_IIC_BS7_SIZE
};

#if MIOS32_IIC_BS_NUM
// available BankSticks
static u8 bs_available = 0;
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes BankSticks
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_IIC_BS_NUM == 0
  return -1; // BankSticks not explicitely enabled in mios32_config.h
#else

  // initialize IIC interface
  if( MIOS32_IIC_Init(0) < 0 )
    return -1; // initialisation of IIC Interface failed

  // scan for available BankSticks
  if( MIOS32_IIC_BS_ScanBankSticks() < 0 )
    return -2; // we don't expect that any other task accesses the IIC port yet!

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Scans all BankSticks to check the availablility by sending dummy requests
//! and checking the ACK response.
//!
//! Per module, this procedure takes at least ca. 25 uS, if no module is 
//! connected ca. 75 uS (3 retries), or even more if we have to wait for 
//! completion of the previous IIC transfer.
//! Therefore this function should only be rarely used (e.g. once per second),
//! and the state should be saved somewhere in the application
//! \return 0 if all BankSticks scanned
//! \return -2 if BankStick blocked by another task (retry the scan!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_ScanBankSticks(void)
{
#if MIOS32_IIC_BS_NUM == 0
  return 0; // BankSticks not explicitely enabled in mios32_config.h
#else
  // try to get the IIC peripheral
  if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  u8 bs;
  bs_available = 0;
  for(bs=0; bs<MIOS32_IIC_BS_NUM; ++bs) {
    if( bs_size[bs] ) {
      // try to address the BankStick 3 times
      u8 retries=3;
      s32 error = -1;

      while( error < 0 && retries-- ) {
	s32 error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_BS_ADDR_BASE + 2*bs, NULL, 0);
	if( !error )
	  error = MIOS32_IIC_TransferWait();
	if( !error )
	  bs_available |= (1 << bs);
      }
    }
  }

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  return 0; // no error during scan
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a BankStick
//! taken from the last results of \ref MIOS32_IIC_BS_ScanBankSticks
//! \param[in] bs BankStick number (0-7)
//! \return >0: BankStick available, returns the size in bytes (e.g. 32768)
//! \return 0: BankStick not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_CheckAvailable(u8 bs)
{
#if MIOS32_IIC_BS_NUM == 0
  return 0x00; // BankSticks not explicitely enabled in mios32_config.h
#else
  return (bs_available & (1 << bs)) ? bs_size[bs] : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Reads one or more bytes into a buffer
//! \param[in] bs BankStick number (0-7)
//! \param[in] address BankStick address (depends on size)
//! \param[out] buffer destination buffer
//! \param[in] len number of bytes which should be read (1..64)
//! \return 0 if operation was successful
//! \return -1 if error during IIC transfer
//! \return -2 if BankStick blocked by another task (retry it!)
//! \return -3 if BankStick wasn't available at last scan
//! \return -4 if invalid length
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_Read(u8 bs, u16 address, u8 *buffer, u8 len)
{
#if MIOS32_IIC_BS_NUM == 0
  return -3; // BankSticks not explicitely enabled in mios32_config.h
#else
  // length has to be >1 and <=64
  if( !len || len > 64 )
    return -4;

  // check if BankStick was available at last scan
  if( !(bs_available & (1 << bs)) )
    return -3;

  // try to get the IIC peripheral
  if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  // send IIC address and EEPROM address
  // to avoid issues with litte/big endian: copy address into temporary buffer
  u8 addr_buffer[2] = {(u8)(address>>8), (u8)address};
  s32 error = MIOS32_IIC_Transfer(IIC_Write_WithoutStop, MIOS32_IIC_BS_ADDR_BASE + 2*bs, addr_buffer, 2);
  if( !error )
    error = MIOS32_IIC_TransferWait();

  // now receive byte(s)
  if( !error )
    error = MIOS32_IIC_Transfer(IIC_Read, MIOS32_IIC_BS_ADDR_BASE + 2*bs, buffer, len);
  if( !error )
    error = MIOS32_IIC_TransferWait();

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  // return error status
  return error < 0 ? -1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Writes one or more bytes into the BankStick
//! \param[in] bs BankStick number (0-7)
//! \param[in] address BankStick address (depends on size)
//! \param[in] buffer source buffer
//! \param[in] len number of bytes which should be written (1..64)
//! \return 0 if operation was successful
//! \return -1 if error during IIC transfer
//! \return -2 if BankStick blocked by another task (retry it!)
//! \return -3 if BankStick wasn't available at last scan
//! \return -4 if invalid length
//! \note Use \ref MIOS32_IIC_BS_CheckWriteFinished to check when the write operation
//! has been finished - this can take up to 5 mS!
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_Write(u8 bs, u16 address, u8 *buffer, u8 len)
{
#if MIOS32_IIC_BS_NUM == 0
  return -3; // BankSticks not explicitely enabled in mios32_config.h
#else
  // length has to be >1 and <=64
  if( !len || len > 64 )
    return -4;

  // check if BankStick was available at last scan
  if( !(bs_available & (1 << bs)) )
    return -3;

  // try to get the IIC peripheral
  if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  // send IIC address, EEPROM address and data to EEPROM
  u8 eeprom_buffer[64+2];
  eeprom_buffer[0] = (u8)(address>>8);
  eeprom_buffer[1] = (u8)address;
  int i;
  for(i=0; i<len; ++i)
    eeprom_buffer[i+2] = buffer[i];

  s32 error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_BS_ADDR_BASE + 2*bs, eeprom_buffer, len+2);
  if( !error )
    error = MIOS32_IIC_TransferWait();

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  // return error status
  return error < 0 ? -1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Has to be used after MIOS32_IIC_BS_Write (during write operation) 
//! to poll the device state
//! \param[in] bs BankStick number (0-7)
//! \return 1 if BankStick was available before, and now doesn't respond: write operation is in progress
//! \return 0 if BankStick available, write operation finished
//! \return -1 if error during IIC transfer
//! \return -2 if BankStick blocked by another task (retry it!)
//! \return -3 if BankStick wasn't available at last scan
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_BS_CheckWriteFinished(u8 bs)
{
#if MIOS32_IIC_BS_NUM == 0
  return -3; // BankSticks not explicitely enabled in mios32_config.h
#else
  // check if BankStick was available at last scan
  if( !(bs_available & (1 << bs)) )
    return -3;

  // try to get the IIC peripheral
  if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  // only send the address, check for ACK/NAK flag
  s32 error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_BS_ADDR_BASE + 2*bs, NULL, 0);
  if( !error )
    error = MIOS32_IIC_TransferWait();

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  if( error == MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED )
    return 1; // got a NAK
  if( error >= 0 )
    return 0; // BankStick available and write operation finished

  return -1; // IIC error (for debugging: error status can be read with MIOS32_IIC_LastErrorGet())
#endif
}

//! \}

#endif /* MIOS32_DONT_USE_IIC_BS */
