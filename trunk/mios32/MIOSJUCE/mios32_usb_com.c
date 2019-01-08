// $Id: mios32_usb_com.c 212 2008-12-26 16:54:09Z tk $
//! \defgroup MIOS32_USB_COM
//!
//! USB COM layer for MIOS32
//! 
//! Based on driver included in STM32 USB library
//! Some code copied and modified from Virtual_COM_Port demo
//! 
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM layer functions
//! 
//! \note this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h) by adding '#define MIOS32_USE_USB_COM'
//! it's disabled by default, since Windows doesn't allow to use USB MIDI and CDC in parallel!
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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
// it's disabled by default, since Windows doesn't allow to use USB MIDI and CDC in parallel!
#if defined(MIOS32_USE_USB_COM)




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
//! Initializes USB COM layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] connected connection status (1 if connected)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! This function returns the connection status of the USB COM interface
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_CheckAvailable(void)
{
  return transfer_possible ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns number of free bytes in receive buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return number of free bytes
//! \return 0 if usb_com not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! Returns number of used bytes in receive buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return number of used bytes
//! \return 0 if usb_com not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! Gets a byte from the receive buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return -1 if USB_COM not available
//! \return -2 if no new byte available
//! \return >= 0: received byte
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
  // MIOS32_IRQ_Disable();

  // TODO: access buffer directly, so that we don't need to copy into temporary buffer
  u8 buffer_out[MIOS32_USB_COM_DATA_OUT_SIZE];
  PMAToUserBufferCopy(buffer_out, MIOS32_USB_ENDP3_RXADDR, GetEPRxCount(ENDP3));
  u8 b = buffer_out[rx_buffer_ix++];
  if( !--rx_buffer_new_data_ctr )
    SetEPRxValid(ENDP3);
  // MIOS32_IRQ_Enable();

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the next byte of the receive buffer without taking it
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return -1 if USB_COM not available
//! \return -2 if no new byte available
//! \return >= 0: received byte
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
  // MIOS32_IRQ_Disable();
  // TODO: access buffer directly, so that we don't need to copy into temporary buffer
  u8 buffer_out[MIOS32_USB_COM_DATA_OUT_SIZE];
  PMAToUserBufferCopy(buffer_out, MIOS32_USB_ENDP3_RXADDR, GetEPRxCount(ENDP3));
  u8 b = buffer_out[rx_buffer_ix];
  // MIOS32_IRQ_Enable();

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in transmit buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return number of free bytes
//! \return 0 if usb_com not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! Returns number of used bytes in transmit buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \return number of used bytes
//! \return 0 if usb_com not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! puts more than one byte onto the transmit buffer (used for atomic sends)
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \param[in] *buffer pointer to buffer which should be transmitted
//! \param[in] len number of bytes which should be transmitted
//! \return 0 if no error
//! \return -1 if USB_COM not available
//! \return -2 if buffer full or cannot get all requested bytes (retry)
//! \return -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//! \return -4 if too many bytes should be sent
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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
//! puts more than one byte onto the transmit buffer (used for atomic sends)<BR>
//! (blocking function)
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \param[in] *buffer pointer to buffer which should be transmitted
//! \param[in] len number of bytes which should be transmitted
//! \return 0 if no error
//! \return -1 if USB_COM not available
//! \return -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//! \return -4 if too many bytes should be sent
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPutMore(u8 usb_com, u8 *buffer, u16 len)
{
  s32 error;

  while( (error=MIOS32_USB_COM_TxBufferPutMore_NonBlocking(usb_com, buffer, len)) == -2 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if USB_COM not available
//! \return -2 if buffer full (retry)
//! \return -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPut_NonBlocking(u8 usb_com, u8 b)
{
  // for more comfortable usage...
  // -> just forward to MIOS32_USB_COM_TxBufferPutMore
  return MIOS32_USB_COM_TxBufferPutMore_NonBlocking(usb_com, &b, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer<BR>
//! (blocking function)
//! \param[in] usb_com USB_COM number (not supported yet, should always be 0)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if USB_COM not available
//! \return -3 if USB_COM not supported by MIOS32_USB_COM_TxBufferPut Routine
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_COM_TxBufferPut(u8 usb_com, u8 b)
{
  s32 error;

  while( (error=MIOS32_USB_COM_TxBufferPutMore_NonBlocking(usb_com, &b, 1)) == -2 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! Called by STM32 USB driver to check for IN streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_COM_EP4_IN_Callback(void)
{
  // package has been sent
  tx_buffer_busy = 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Called by STM32 USB driver to check for OUT streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_COM_EP3_OUT_Callback(void)
{
  // new data has been received - notify this
  rx_buffer_new_data_ctr = GetEPRxCount(ENDP3);
  rx_buffer_ix = 0;
}


/////////////////////////////////////////////////////////////////////////////
//! MIOS32_USB callback functions (forwarded from STM32 USB driver)
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
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

//! \}

#endif /* MIOS32_USE_USB_COM */
