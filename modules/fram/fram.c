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
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function initialized the FRAM module
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Init(void)
{
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
s32 FRAM_Transfer(FRAM_transfer_t transfer_type, u8 device_addr, u32 mem_addr, u8 *buffer, u8 buffer_len){
  return -1;
  }
  

/////////////////////////////////////////////////////////////////////////////
// This function checks if the device <device_addr> is present
// IN: -
// OUT: returns 0 if device is present, < 0 on errors
/////////////////////////////////////////////////////////////////////////////

s32 FRAM_CheckDevice(u8 device_addr){
  return -1;
  }