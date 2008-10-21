// $Id$
/*
 * USB COM driver for MIOS32
 *
 * Based on driver included in STM32 USB library
 * Some code copied and modified from Virtual_COM_Port demo
 *
 * ==========================================================================
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
#if !defined(MIOS32_DONT_USE_USB_COM)

#include <usb_lib.h>

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

void MIOS32_USB_COM_TxBufferHandler(void);
void MIOS32_USB_COM_RxBufferHandler(void);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// rx/tx status
static volatile u8 rx_buffer_new_data_ctr;
static volatile u8 rx_buffer_ix;

static volatile u8 tx_buffer_busy;

// optional non-blocking mode
static u8 non_blocking_mode = 0;

// transfer possible?
static u8 transfer_possible = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes the USB COM Protocol
// IN: <mode>: 0: MIOS32_COM_TxBufferPut* works in blocking mode - function will
//                (shortly) stall if the output buffer is full
//             1: MIOS32_COM_TxBufferPut* works in non-blocking mode - function will
//                return -2 if buffer is full, the caller has to loop if this
//                value is returned until the transfer was successful
//                A common method is to release the RTOS task for 1 mS
//                so that other tasks can be executed until the sender can
//                continue
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_Init(u32 mode)
{
  switch( mode ) {
    case 0:
      non_blocking_mode = 0;
      break;
    case 1:
      non_blocking_mode = 1;
      break;
    default:
      return -1; // unsupported mode
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by the USB driver on cable connection/disconnection
// IN: <connected>: connection status (1 if connected)
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_ChangeConnectionState(u8 connected)
{
  // in all cases: re-initialize USB COM driver

  if( connected ) {
    transfer_possible = 1;
    rx_buffer_new_data_ctr = 0;
    SetEPRxValid(ENDP3);
    tx_buffer_busy = 0; // buffer not busy anymore
  } else {
    // cable disconnected: disable transfers
    transfer_possible = 0;
    rx_buffer_new_data_ctr = 0;
    tx_buffer_busy = 1; // buffer busy
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the connection status of the USB COM interface
// IN: -
// OUT: 1: interface available
//      0: interface not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_CheckAvailable(void)
{
  return transfer_possible ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns number of free bytes in receive buffer
// IN: USB_COM number
// OUT: number of free bytes
//      if usb_com not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_RxBufferFree(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return 0; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return 0;
  else
    return MIOS32_USB_COM_DATA_OUT_SIZE-rx_buffer_new_data_ctr;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns number of used bytes in receive buffer
// IN: USB_COM number (0..1)
// OUT: number of used bytes
//      if usb_com not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_RxBufferUsed(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return 0; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return 0;
  else
    return rx_buffer_new_data_ctr;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// gets a byte from the receive buffer
// IN: USB_COM number (0..1)
// OUT: -1 if USB_COM not available
//      -2 if no new byte available
//      otherwise received byte
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_RxBufferGet(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return -1; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return -1; // USB_COM not available

  if( !rx_buffer_new_data_ctr )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  //  portENTER_CRITICAL(); // port specific FreeRTOS function to disable IRQs (nested)

  // TODO: access buffer directly, so that we don't need to copy into temporary buffer
  u8 buffer_out[MIOS32_USB_COM_DATA_OUT_SIZE];
  PMAToUserBufferCopy(buffer_out, MIOS32_USB_ENDP3_RXADDR, GetEPRxCount(ENDP3));
  u8 b = buffer_out[rx_buffer_ix++];
  if( !--rx_buffer_new_data_ctr )
    SetEPRxValid(ENDP3);
  //  portEXIT_CRITICAL(); // port specific FreeRTOS function to enable IRQs (nested)

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns the next byte of the receive buffer without taking it
// IN: USB_COM number (0..1)
// OUT: -1 if USB_COM not available
//      -2 if no new byte available
//      otherwise received byte
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_RxBufferPeek(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return -1; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return -1; // USB_COM not available

  if( !rx_buffer_new_data_ctr )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  //  portENTER_CRITICAL(); // port specific FreeRTOS function to disable IRQs (nested)
  // TODO: access buffer directly, so that we don't need to copy into temporary buffer
  u8 buffer_out[MIOS32_USB_COM_DATA_OUT_SIZE];
  PMAToUserBufferCopy(buffer_out, MIOS32_USB_ENDP3_RXADDR, GetEPRxCount(ENDP3));
  u8 b = buffer_out[rx_buffer_ix];
  //  portEXIT_CRITICAL(); // port specific FreeRTOS function to enable IRQs (nested)

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns number of free bytes in transmit buffer
// IN: USB_COM number (0..1)
// OUT: number of free bytes
//      if usb_com not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferFree(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return 0; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return 0;
  else
    return tx_buffer_busy ? 0 : MIOS32_USB_COM_DATA_IN_SIZE;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns number of used bytes in transmit buffer
// IN: USB_COM number (0..1)
// OUT: number of used bytes
//      if usb_com not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferUsed(u8 usb_com)
{
#if MIOS32_USB_COM_NUM == 0
  return 0; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return 0;
  else
    return tx_buffer_busy ? MIOS32_USB_COM_DATA_IN_SIZE : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// puts more than one byte onto the transmit buffer (used for atomic sends)
// IN: USB_COM number (0..1), buffer to be sent and buffer length
// OUT: 0 if no error
//      -1 if USB_COM not available
//      -2 if buffer full or cannot get all requested bytes (retry)
//      -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//      -4 if too many bytes should be sent
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPutMore(u8 usb_com, u8 *buffer, u16 len)
{
#if MIOS32_USB_COM_NUM == 0
  return -1; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return -1; // USB_COM not available

  if( tx_buffer_busy )
    return -2; // buffer full (retry)

  if( len > MIOS32_USB_COM_DATA_IN_SIZE )
    return -4; // cannot get all requested bytes

  // copy bytes to be transmitted into transmit buffer
  UserToPMABufferCopy(buffer, MIOS32_USB_ENDP4_TXADDR, len);

  // send buffer
  tx_buffer_busy = 1;
  SetEPTxCount(ENDP4, len);
  SetEPTxValid(ENDP4);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the transmit buffer
// IN: USB_COM number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if USB_COM not available
//      -2 if buffer full (retry)
//      -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPut(u8 usb_com, u8 b)
{
  // for more comfortable usage...
  // -> just forward to MIOS32_USB_COM_TxBufferPutMore
  return MIOS32_USB_COM_TxBufferPutMore(usb_com, &b, 1);
}


/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for IN streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_COM_EP4_IN_Callback(void)
{
  // package has been sent
  tx_buffer_busy = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for OUT streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_COM_EP3_OUT_Callback(void)
{
  // new data has been received - notify this
  rx_buffer_new_data_ctr = GetEPRxCount(ENDP3);
  rx_buffer_ix = 0;
}

#endif /* MIOS32_DONT_USE_USB_COM */
