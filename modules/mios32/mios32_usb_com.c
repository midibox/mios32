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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
// it's disabled by default, since Windows doesn't allow to use USB MIDI and CDC in parallel!
#if defined(MIOS32_USE_USB_COM)

#include <usb_lib.h>

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Local Defines
/////////////////////////////////////////////////////////////////////////////

#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23


/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

typedef struct
{
  u32 bitrate;
  u8 format;
  u8 paritytype;
  u8 datatype;
} LINE_CODING;


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

// transfer possible?
static u8 transfer_possible = 0;

// COM Requests
u8 Request = 0;

// Default linecoding
LINE_CODING linecoding = {
  115200, /* baud rate*/
  0x00,   /* stop bits-1*/
  0x00,   /* parity - none*/
  0x08    /* no. of bits 8*/
};



/////////////////////////////////////////////////////////////////////////////
// Initializes the USB COM Protocol
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

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
s32 MIOS32_USB_COM_TxBufferPutMore_NonBlocking(u8 usb_com, u8 *buffer, u16 len)
{
#if MIOS32_USB_COM_NUM == 0
  return -1; // no USB_COM available
#else
  if( usb_com >= MIOS32_USB_COM_NUM )
    return -1; // USB_COM not available

  if( len > MIOS32_USB_COM_DATA_IN_SIZE )
    return -4; // cannot get all requested bytes

  if( tx_buffer_busy )
    return -2; // buffer full (retry)

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
// puts more than one byte onto the transmit buffer (used for atomic sends)
// (blocking function)
// IN: USB_COM number (0..1), buffer to be sent and buffer length
// OUT: 0 if no error
//      -1 if USB_COM not available
//      -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//      -4 if too many bytes should be sent
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPutMore(u8 usb_com, u8 *buffer, u16 len)
{
  s32 error;

  while( (error=MIOS32_USB_COM_TxBufferPutMore(usb_com, buffer, len)) == -2 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the transmit buffer
// IN: USB_COM number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if USB_COM not available
//      -2 if buffer full (retry)
//      -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPut_NonBlocking(u8 usb_com, u8 b)
{
  // for more comfortable usage...
  // -> just forward to MIOS32_USB_COM_TxBufferPutMore
  return MIOS32_USB_COM_TxBufferPutMore_NonBlocking(usb_com, &b, 1);
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the transmit buffer
// (blocking function)
// IN: USB_COM number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if USB_COM not available
//      -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPut(u8 usb_com, u8 b)
{
  s32 error;

  while( (error=MIOS32_USB_COM_TxBufferPutMore_NonBlocking(usb_com, &b, 1)) == -2 );

  return error;
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


/////////////////////////////////////////////////////////////////////////////
// MIOS32_USB callback functions (forwarded from STM32 USB driver)
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_COM_CB_StatusIn(void)
{
  if( Request == SET_LINE_CODING ) {
    // configure UART here...
    Request = 0;
  }
}


// handles the data class specific requests
static u8 *Virtual_Com_Port_GetLineCoding(u16 Length) {
  if( Length == 0 ) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }

  return(u8 *)&linecoding;
}

u8 *Virtual_Com_Port_SetLineCoding(u16 Length)
{
  if( Length == 0 ) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }

  return(u8 *)&linecoding;
}

s32 MIOS32_USB_COM_CB_Data_Setup(u8 RequestNo)
{
  u8 *(*CopyRoutine)(u16) = NULL;

  if( RequestNo == GET_LINE_CODING ) {
    if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) ) {
      CopyRoutine = Virtual_Com_Port_GetLineCoding;
    }
  } else if( RequestNo == SET_LINE_CODING ) {
    if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) ) {
      CopyRoutine = Virtual_Com_Port_SetLineCoding;
    }
    Request = SET_LINE_CODING;
  }

  if( CopyRoutine == NULL ) {
    return USB_UNSUPPORT;
  }

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);

  return USB_SUCCESS;
}

s32 MIOS32_USB_COM_CB_NoData_Setup(u8 RequestNo)
{
  if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) ) {
    if( RequestNo == SET_COMM_FEATURE ) {
      return USB_SUCCESS;
    } else if( RequestNo == SET_CONTROL_LINE_STATE ) {
      return USB_SUCCESS;
    }
  }

  return USB_UNSUPPORT;
}

#endif /* MIOS32_USE_USB_COM */
