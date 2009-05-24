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

// FRAM slave ID (b1010)
#define FRAM_SLAVEID_MASK 0xA0


/////////////////////////////////////////////////////////////////////////////
// This function initialized the FRAM module (IIC & GPIO pins)
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Init(void){
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
  s32 res;
  u8 ext_buf[4];
  MIOS32_IIC_TransferBegin(FRAM_IIC_PORT,IIC_Blocking);
#if FRAM_MULTIPLEX_ENABLE==1
  // Set multiplex pins
  GPIO_WriteBit(FRAM_MULTIPLEX_PORT_MSB, FRAM_MULTIPLEX_PIN_MSB, (device_addr & 0x10) ? Bit_SET : Bit_RESET);
  GPIO_WriteBit(FRAM_MULTIPLEX_PORT_LSB, FRAM_MULTIPLEX_PIN_LSB, (device_addr & 0x08) ? Bit_SET : Bit_RESET);
#endif
  device_addr = ( (device_addr << 1) & 0x0F ) | FRAM_SLAVEID_MASK;
  // swap mem-address top big-endian, write to extension buffer
  ext_buf[1] = (u8)(mem_addr);
  ext_buf[0] = (u8)(mem_addr >> 8);
  // switch by transfer-type 
  switch(transfer_type){
    case FRAM_Write:
      // move two first bytes of transfer-buffer to extension buffer; move mem-address to two first bytes of buffer
      if( buffer_len > 0 ){
        ext_buf[2] = buffer[0];
        if ( buffer_len > 1 ){
          ext_buf[3] = buffer[1];
          if( buffer_len > 2){
            // more than two bytes, original buffer still needed, 
            // increment memory address by two and assign to begin of buffer
            mem_addr += 2;
            buffer[1] = (u8)(mem_addr);
            buffer[0] = (u8)(mem_addr >> 8);
            }
          }
        }
      // transmit extension buffer with mem-address and first two data bytes
      MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write_WithoutStop, device_addr, ext_buf, (buffer_len >= 2) ? 4 : (2 + buffer_len) );
      res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
      // transmit buffer with mem-address and (buffer_len - 2) data bytes
      if( res == 0 && (buffer_len > 2) ){
        MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write, device_addr, buffer, buffer_len);
        res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
        }
      break;
    case FRAM_Read:
      MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write_WithoutStop, device_addr, ext_buf, 2);
      res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
      if( res == 0 ){
       res = MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Read, device_addr, buffer, buffer_len);
       res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
       }
      break;
    default:
      res = -255;// unsupported transfer-type
    }
  MIOS32_IIC_TransferFinished(FRAM_IIC_PORT);
  return res;
  }
  

/////////////////////////////////////////////////////////////////////////////
// This function checks if the device <device_addr> is present
// IN: -
// OUT: returns 0 if device is present, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_CheckDevice(u8 device_addr){
  s32 res;
  MIOS32_IIC_TransferBegin(FRAM_IIC_PORT,IIC_Blocking);
  // Read transfer 0 bytes mem-address 0
  FRAM_Transfer(FRAM_Read,device_addr,0,NULL,0);
  res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
  MIOS32_IIC_TransferFinished(FRAM_IIC_PORT);
  return res;
  }