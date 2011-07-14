// $Id$
//! \defgroup EEPROM
//!
//! The LPC17 variant uses the on-board EEPROM of the LPCXPRESSO module
//!
//! The 24LC64 allows to store up to 8192 bytes.
//! 
//! Since this driver works halfword-wise to keep it compatible to the STM32
//! variant, this means that EEPROM_EMULATED_SIZE can be up to 4096
//!
//! Optionally an external EEPROM can be connected to one of the two IIC ports
//! of the MBHP_CORE_LPC17 by setting following defines in mios32_config.h
//! 
//! // set this either to 0 (for first IIC port) or 2 (for second IIC port)
//! // IIC1 selects the on-board EEPROM
//! #define EEPROM_IIC_DEVICE 1
//! // Set a device number if the 3 address pins are not tied to ground
//! #define EEPROM_IIC_CS     0
//!
//! Usage:
//! <UL>
//!   <LI>EEPROM_Init(u32 mode) should be called after startup to connect to the IIC EEPROM
//!   <LI>EEPROM_Read(u16 address) reads a 16bit value from EEPROM<BR>
//!       Returns <0 if address hasn't been programmed yet (it's up to the
//!       application, how to handle this, e.g. value could be zeroed)\n
//!       Extra LPC17: returns -1 if EEPROM not available
//!   <LI>EEPROM_Write(u16 address, u16 value): programs the 16bit value
//! </UL>
//!
//! Configuration: optionally EEPROM_EMULATED_SIZE can be overruled in mios32_config.h
//! to change the number of virtual addresses.
//!
//! By default, 128 addresses are available:<BR>
//! \code
//! #define EEPROM_EMULATED_SIZE 128  // -> 128 half words = 256 bytes
//! \endcode
//!
//! Example application:<BR>
//!   $MIOS32_PATH/apps/tutorials/025_sysex_and_eeprom (see patch.c)
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

#include <mios32.h>
#include "eeprom.h"

// should be set to 1 for outputting errors about which users should be aware of..
#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

//! Connects to the (on-board) EEPROM
//! \param[in] mode not used yet, only mode 0 is supported!
//! \return < 0 on error
s32 EEPROM_Init(u32 mode)
{
  s32 status;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // initialize IIC interface
  if( (status=MIOS32_IIC_Init(0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Init] ERROR: MIOS32_IIC_Init failed with status %d\n", status);
#endif
    return -1; // initialisation of IIC Interface failed
  }

  // try to get the IIC peripheral
  if( (status=MIOS32_IIC_TransferBegin(EEPROM_IIC_DEVICE, IIC_Non_Blocking)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Init] ERROR: MIOS32_IIC_TransferBegin failed with status %d\n", status);
#endif
    return -2; // normaly retry, here handled as error: shouldn't be allocated by another task
  }

  // try to address the BankStick 3 times
  u8 retries=3;
  u8 bs_available = 0;
  u8 cs = EEPROM_IIC_CS;
  s32 error = -1;
  while( error < 0 && retries-- ) {
    error = MIOS32_IIC_Transfer(EEPROM_IIC_DEVICE, IIC_Write, 0xa0 + 2*cs, NULL, 0);
    if( error < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[EEPROM_Init] ERROR: MIOS32_IIC_Transfer failed with status %d\n", error);
#endif
    } else {
      error = MIOS32_IIC_TransferWait(EEPROM_IIC_DEVICE);
      if( error < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[EEPROM_Init] ERROR: MIOS32_IIC_TransferWait failed with status %d\n", error);
#endif
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[EEPROM_Init] found device\n");
#endif
	bs_available = 1;
      }
    }
  }

  // release IIC peripheral
  MIOS32_IIC_TransferFinished(EEPROM_IIC_DEVICE);

  if( !bs_available ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Init] failed to access device!\n");
#endif
    return -3; // error during access
  }

  return 0; // no error
}

//! Returns the 16bit word on the given halfword address
//! \param[in] address the address which should be read
//! \return >= 0 if value could be successfully read (16bit value)
//! \return -1 if EEPROM not available
s32 EEPROM_Read(u16 address)
{
  s32 error;

  // try to get the IIC peripheral
  if( (error=MIOS32_IIC_TransferBegin(EEPROM_IIC_DEVICE, IIC_Non_Blocking)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Read] ERROR: MIOS32_IIC_TransferBegin failed with status %d\n", error);
#endif
    return -2; // normaly retry, here handled as error: shouldn't be allocated by another task
  }

  // send IIC address and EEPROM address
  u8 cs = EEPROM_IIC_CS;
  u8 eeprom_buffer[2];
  eeprom_buffer[0] = (u8)(address >> 7);
  eeprom_buffer[1] = (u8)(address << 1);
  error = MIOS32_IIC_Transfer(EEPROM_IIC_DEVICE, IIC_Write_WithoutStop, 0xa1 + 2*cs, eeprom_buffer, 2);
  if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Read] ERROR: MIOS32_IIC_Transfer address failed with status %d\n", error);
#endif
  } else {
    error = MIOS32_IIC_TransferWait(EEPROM_IIC_DEVICE);
    if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[EEPROM_Read] ERROR: MIOS32_IIC_TransferWait address failed with status %d\n", error);
#endif
    }
  }

  // now receive byte(s)
  if( !error ) {
    error = MIOS32_IIC_Transfer(EEPROM_IIC_DEVICE, IIC_Read, MIOS32_IIC_BS_ADDR_BASE + 2*cs, eeprom_buffer, 2);
    if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[EEPROM_Read] ERROR: MIOS32_IIC_Transfer read value failed with status %d\n", error);
#endif
    } else {
      error = MIOS32_IIC_TransferWait(EEPROM_IIC_DEVICE);
      if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[EEPROM_Read] ERROR: MIOS32_IIC_TransferWait read value failed with status %d\n", error);
#endif
      }
    }
  }

  // release IIC peripheral
  MIOS32_IIC_TransferFinished(EEPROM_IIC_DEVICE);

  if( error )
    return -1; // read failed...

  return (u16)eeprom_buffer[0] | ((u16)eeprom_buffer[1] << 8);
}

//! Writes into EEPROM at given halfword address
//! \param[in] address the address which should be written
//! \param[in] value the 16bit value which should be written
//! \return 0 on success, < 0 on errors
s32 EEPROM_Write(u16 address, u16 value)
{
  s32 error;

  // first we should check if the value has already been written
  // this saves some time and enlarges the EEPROM livecycle
  if( EEPROM_Read(address) == value )
    return 0; // fine!

  // try to get the IIC peripheral
  if( (error=MIOS32_IIC_TransferBegin(EEPROM_IIC_DEVICE, IIC_Non_Blocking)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Write] ERROR: MIOS32_IIC_TransferBegin failed with status %d\n", error);
#endif
    return -2; // normaly retry, here handled as error: shouldn't be allocated by another task
  }

  // send IIC address, EEPROM address and data to EEPROM
  u8 cs = EEPROM_IIC_CS;
  u8 eeprom_buffer[4];
  eeprom_buffer[0] = (u8)(address >> 7);
  eeprom_buffer[1] = (u8)(address << 1);
  eeprom_buffer[2] = (u8)(value);
  eeprom_buffer[3] = (u8)(value >> 8);

  error = MIOS32_IIC_Transfer(EEPROM_IIC_DEVICE, IIC_Write, 0xa0 + 2*cs, eeprom_buffer, 4);
  if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Write] ERROR: MIOS32_IIC_Transfer failed with status %d\n", error);
#endif
  } else {
    error = MIOS32_IIC_TransferWait(EEPROM_IIC_DEVICE);
    if( error ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[EEPROM_Write] ERROR: MIOS32_IIC_TransferWait failed with status %d\n", error);
#endif
    }
  }

  // wait until write operation finished  
  u32 retry_ctr = 0;
  u8 available = 0;
  while( !available && (++retry_ctr < 1000) ) { // enough? Normaly it takes ca. 130 polls till EEPROM is ready
    // only send the address, check for ACK/NAK flag
    error = MIOS32_IIC_Transfer(EEPROM_IIC_DEVICE, IIC_Write, 0xa0 + 2*cs, NULL, 0);
    if( !error )
      error = MIOS32_IIC_TransferWait(EEPROM_IIC_DEVICE);

    // release IIC peripheral
    MIOS32_IIC_TransferFinished(EEPROM_IIC_DEVICE);

    if( error == MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED ) {
      // NAK - retry...
    } else if( error >= 0 ) {
      // EEPROM available again
      available = 1;
    }
  }

  if( !available ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[EEPROM_Write] ERROR: device not available anymore after write operation!\n");
#endif
    return -3; // write failed
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[EEPROM_Write] successfull write (available again after %d polls)\n", retry_ctr);
#endif

  // release IIC peripheral
  MIOS32_IIC_TransferFinished(EEPROM_IIC_DEVICE);

  return 0; // no error
}
