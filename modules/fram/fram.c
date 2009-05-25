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
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// 0 no ongoing transfer; 1 ongoing transfer; < 0 error_code (last transfer)
volatile static s32 transfer_status;


/////////////////////////////////////////////////////////////////////////////
// Initializes the FRAM module (IIC & GPIO pins)
// IN: -
// OUT: 0: success; -1 error (IIC initialization failed)
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
  // initialize variables
  transfer_status = 0;
  // initialize IIC device
  return MIOS32_IIC_Init(0);
  }


/////////////////////////////////////////////////////////////////////////////
// Checks if the device <device_addr> is present (blocking)
// IN: -
// OUT: returns 0 if device is present; < 0 on errors 
//      (see README for error codes)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_CheckAvailable(u8 device_addr){
  return FRAM_Read(device_addr, 0, NULL, 0);
  }
  
  
/////////////////////////////////////////////////////////////////////////////
// Reads data from an FRAM device (blocking mode).
// IN: device_addr: FRAM device address (0 - 8)
//     mem_addr: start-address on FRAM device memory
//     buffer: pointer to data buffer
//     buffer_len: count bytes to read
// OUT: 0 on success, < 0 on errors
//      (see README for error codes)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Read(u8 device_addr, u16 mem_addr, u8 *buffer, u8 buffer_len){
  u32 res;
  if( FRAM_SemaphoreEnter(0) != 0 )
    return FRAM_ERROR_DEVICE_BLOCKED;
  FRAM_Transfer(FRAM_ReadTransfer, device_addr, mem_addr, buffer, buffer_len);
  res = FRAM_TransferWaitCheck(1);
  FRAM_SemaphoreLeave();
  return res;
  }

/////////////////////////////////////////////////////////////////////////////
// Writes data to an FRAM device (blocking mode).
// IN: device_addr: FRAM device address (0 - 8)
//     mem_addr: start-address on FRAM device memory
//     buffer: pointer to data buffer
//     buffer_len: count bytes to write
// OUT: 0 on success, < 0 on errors
//      (see README for error codes)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Write(u8 device_addr, u16 mem_addr, u8 *buffer, u8 buffer_len){
  u32 res;
  if( FRAM_SemaphoreEnter(0) != 0 )
    return FRAM_ERROR_DEVICE_BLOCKED;
  FRAM_Transfer(FRAM_WriteTransfer, device_addr, mem_addr, buffer, buffer_len);
  res = FRAM_TransferWaitCheck(1);
  FRAM_SemaphoreLeave();
  return res;
  }
  
  
/////////////////////////////////////////////////////////////////////////////
// Checks the transfer status or waits for status == 0 (no ongoing transfer)
// IN: blocking = 1: waits for the ongoing transfer to finish; 
//                0: returns the current transfer status
// OUT: transfer status (1: ongoing; 0: no transfer; < 0: error during last
//      transfer. clears the transfer-status on errors.
//      (see README for error codes)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_TransferWaitCheck(u8 blocking){
  s32 res;
  if(transfer_status == 1){
    // ongoing transfer: poll IIC transfer status
    transfer_status = blocking ? 
      MIOS32_IIC_TransferWait(FRAM_IIC_PORT) : 
      MIOS32_IIC_TransferCheck(FRAM_IIC_PORT);
    }
  res = transfer_status;
  // clear error status
  if(transfer_status < 0)
    transfer_status = 0;
  // return last transfer status
  return res;
  }
  
  
/////////////////////////////////////////////////////////////////////////////
// Enters / sets the FRAM (IIC) semaphore
// IN: blocking = 1: waits for the semaphore to be left by other task
//                0: returns -1 if semaphore is currently set, doesn't wait
// OUT: returns 0 if semaphore was entered, -1 if semaphore is in use (blocking)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_SemaphoreEnter(u8 blocking){
  return MIOS32_IIC_TransferBegin(FRAM_IIC_PORT, blocking ? IIC_Blocking : IIC_Non_Blocking );
  }


/////////////////////////////////////////////////////////////////////////////
// Leaves / releases the FRAM (IIC) semaphore
// IN: -
// OUT: -
/////////////////////////////////////////////////////////////////////////////
void FRAM_SemaphoreLeave(void){
  MIOS32_IIC_TransferFinished(FRAM_IIC_PORT);
  }


/////////////////////////////////////////////////////////////////////////////
// Starts a FRAM transfer. The function is a (almost) non-blocking low-level 
// function. It just blocks to transmit the first four (write) or two (read)
// bytes. Then it triggers the IIC tranfer for the remaining data and returns.
//
// IN: transfer_type (FRAM_ReadTransfer / FRAM_WriteTransfer)
//     device_addr: FRAM device number (address selector pin's)
//     mem_addr: memory-address in FRAM memory
//     buffer: pointer to r/w buffer
//     buffer_len: number of bytes to read/write
// OUT: returns < 0 on errors, 0 on success
//     to check for errors or wait for the ongoing transfer after the function
//     returned, use FRAM_TransferWaitCheck()
//     (see README for error codes)
/////////////////////////////////////////////////////////////////////////////
s32 FRAM_Transfer(FRAM_transfer_t transfer_type, u8 device_addr, u16 mem_addr, u8 *buffer, u8 buffer_len){
  s32 res;
  u8 ext_buf[4];
  // wait for last transfer (blocking)
  FRAM_TransferWaitCheck(1);
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
    case FRAM_WriteTransfer:
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
      // transmit extension buffer with mem-address and first two data bytes, no stop condition
      MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write_WithoutStop, device_addr, ext_buf, (buffer_len >= 2) ? 4 : (2 + buffer_len) );
      res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
      // transmit buffer with mem-address and (buffer_len - 2) data bytes
      if( res == 0 && (buffer_len > 2) ){
        res = MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write, device_addr, buffer, buffer_len);
        }
      break;
    case FRAM_ReadTransfer:
      // transmit mem-address in extension buffer, no stop
      MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Write_WithoutStop, device_addr, ext_buf, 2);
      res = MIOS32_IIC_TransferWait(FRAM_IIC_PORT);
      // receive bytes starting at mem-address transmitted
      if( res == 0 && buffer_len > 0 ){
       res = MIOS32_IIC_Transfer(FRAM_IIC_PORT, IIC_Read, device_addr, buffer, buffer_len);
       }
      break;
    default:
      res = FRAM_ERROR_TRANSFER_TYPE;// unsupported transfer-type
    }
  transfer_status = (res == 0) ? 1 : res;// set transfer-status to ongoing or error
  return res;
  }
