/*
 * FRAM device driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "fram.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define FRAM_SLAVEID_MASK 0xA0

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function initialized the FRAM module
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Init(void)
{
  // initialize multiplex port
#if FRAM_MULTIPLEX_ENABLE==1
  // prepare structure
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
  // configure pins
  GPIO_InitStructure.GPIO_Pin = FRAM_MULTIPLEX_PIN_MSB;
  GPIO_Init(FRAM_MULTIPLEX_PORT_MSB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = FRAM_MULTIPLEX_PIN_LSB;
  GPIO_Init(FRAM_MULTIPLEX_PORT_LSB, &GPIO_InitStructure);
#endif
  // initialize IIC device
  return MIOS32_IIC_Init(0);
}

/////////////////////////////////////////////////////////////////////////////
// This function starts a FRAM transfer
// IN: transfer_type (FRAM_Read_Blocking / FRAM_Read_Non_Blocking / 
//       FRAM_Write_Blocking / FRAM_Write_Non_Blocking)
//     device_addr: FRAM device number (address selector pin's)
//     mem_addr: memory-address in FRAM memory
//     buffer: pointer to r/w buffer
//     buffer_len: number of bytes to read/write
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Transfer(FRAM_transfer_t transfer_type, u8 device_addr, u16 mem_addr, u8 *buffer, u8 buffer_len){
  MIOS32_IIC_TransferBegin(FRAM_IIC_PORT,IIC_Blocking);
  // Write: single write transfer
  // Read: write transfer for mem_address (writewithoutstop), read transfer.
  MIOS32_IIC_TransferFinished(FRAM_IIC_PORT);
  return -1;
  }
  

/////////////////////////////////////////////////////////////////////////////
// This function checks if the device <device_addr> is present
// IN: -
// OUT: returns 0 if device is present, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_CheckDevice(u8 device_addr){
  MIOS32_IIC_TransferBegin(FRAM_IIC_PORT,IIC_Blocking);
  // Read transfer 0 bytes mem-address 0
  MIOS32_IIC_TransferFinished(FRAM_IIC_PORT);
  return -1;
  }