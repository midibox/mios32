// $Id$
//! \defgroup MIOS32_UART
//!
//! U(S)ART functions for MIOS32
//!
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
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

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_UART)

// using integrated UART FIFOs, no need for SW buffering
#undef  MIOS32_UART_RX_BUFFER_SIZE
#define MIOS32_UART_RX_BUFFER_SIZE 16

#undef  MIOS32_UART_TX_BUFFER_SIZE
#define MIOS32_UART_TX_BUFFER_SIZE 16

/////////////////////////////////////////////////////////////////////////////
// Pin definitions and UART mappings
/////////////////////////////////////////////////////////////////////////////

// all peripherals are clocked at CCLK/4
#define UART_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4000000)

// TX: P0.2, RX: P0.3
#define MIOS32_UART0_TX_INIT     { LPC_PINCON->PINSEL0 &= ~(3 << (2*2)); LPC_PINCON->PINSEL0 |= (1 << (2*2)); }
#define MIOS32_UART0_RX_INIT     { LPC_PINCON->PINSEL0 &= ~(3 << (2*3)); LPC_PINCON->PINSEL0 |= (1 << (2*3)); }
#define MIOS32_UART0             LPC_UART0


// help masks
#define LSR_RDR         0x01
#define LSR_OE          0x02
#define LSR_PE          0x04
#define LSR_FE          0x08
#define LSR_BI          0x10
#define LSR_THRE        0x20
#define LSR_TEMT        0x40
#define LSR_RXFE        0x80

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_UART_NUM >= 1
static const LPC_UART_TypeDef *uart_base[MIOS32_UART_NUM] = { (LPC_UART_TypeDef*)MIOS32_UART0 };

static u32 uart_baudrate[MIOS32_UART_NUM];

static volatile u8 tx_buffer_size[MIOS32_UART_NUM];
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes UART interfaces
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_UART_NUM == 0
  return -1; // no UARTs
#else

  // configure UART pins
  MIOS32_UART0_TX_INIT;
  MIOS32_UART0_RX_INIT;
#if MIOS32_UART_NUM >= 2
  MIOS32_UART1_TX_INIT;
  MIOS32_UART1_RX_INIT;
#endif
#if MIOS32_UART_NUM >= 3
  MIOS32_UART2_TX_INIT;
  MIOS32_UART2_RX_INIT;
#endif

  // clear size counters
  int i;
  for(i=0; i<MIOS32_UART_NUM; ++i) {
    tx_buffer_size[i] = 0;
  }

  // UART configuration
  MIOS32_UART_BaudrateSet(0, MIOS32_UART0_BAUDRATE);
#if MIOS32_UART_NUM >=2
  MIOS32_UART_BaudrateSet(1, MIOS32_UART1_BAUDRATE);
#endif
#if MIOS32_UART_NUM >=3
  MIOS32_UART_BaudrateSet(2, MIOS32_UART2_BAUDRATE);
#endif

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! sets the baudrate of a UART port
//! \param[in] uart UART number (0..2)
//! \param[in] baudrate the baudrate
//! \return 0: baudrate has been changed
//! \return -1: uart not available
//! \return -2: function not prepared for this UART
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_BaudrateSet(u8 uart, u32 baudrate)
{
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1;

  LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];

  // USART configuration
  u->LCR = 0x83; // 8 bits, no Parity, 1 Stop bit, DLAB=1
  u32 Fdiv = ( UART_PERIPHERAL_FRQ / 16 ) / baudrate ;   // Set baud rate
  u->DLM = Fdiv / 256;                                                        
  u->DLL = Fdiv % 256;
  u->LCR = 0x03;              // 8 bits, no Parity, 1 Stop bit DLAB = 0
  u->FCR = 0x07;              // Enable and reset TX and RX FIFO

  // store baudrate in array
  uart_baudrate[uart] = baudrate;

  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! returns the current baudrate of a UART port
//! \param[in] uart UART number (0..2)
//! \return 0: uart not available
//! \return all other values: the current baudrate
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_UART_BaudrateGet(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return 0;
  else
    return uart_baudrate[uart];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in receive buffer
//! \param[in] uart UART number (0..2)
//! \return uart number of free bytes
//! \return 1: uart available
//! \return 0: uart not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferFree(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return 0;
  else {
    // fake size, since LPC based uart doesn't provide access to the FIFO pointer
    LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
    return (u->LSR & LSR_RDR) ? 0 : MIOS32_UART_RX_BUFFER_SIZE;
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in receive buffer
//! \param[in] uart UART number (0..2)
//! \return > 0: number of used bytes
//! \return 0 if uart not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferUsed(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return 0;
  else {
    // fake size, since LPC based uart doesn't provide access to the FIFO pointer
    LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
    return (u->LSR & LSR_RDR) ? MIOS32_UART_RX_BUFFER_SIZE : 0;
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the receive buffer
//! \param[in] uart UART number (0..2)
//! \return -1 if UART not available
//! \return -2 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferGet(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1; // UART not available

  LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
  if( (u->LSR & LSR_RDR) == 0 )
    return -2; // nothing new in buffer

  // read byte from FIFO
  u8 b = LPC_UART0->RBR;

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns the next byte of the receive buffer without taking it
//! \param[in] uart UART number (0..2)
//! \return -1 if UART not available
//! \return -2 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferPeek(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1; // UART not available

  LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
  if( (u->LSR & LSR_RDR) == 0 )
    return -2; // nothing new in buffer

  // not supported by LPC based UART
  return -3;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the receive buffer
//! \param[in] uart UART number (0..2)
//! \param[in] b byte which should be put into Rx buffer
//! \return 0 if no error
//! \return -1 if UART not available
//! \return -2 if buffer full (retry)
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferPut(u8 uart, u8 b)
{
  return -1; // not applicable
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in transmit buffer
//! \param[in] uart UART number (0..2)
//! \return number of free bytes
//! \return 0 if uart not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferFree(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return 0;
  else {
    // fake size, since LPC based uart doesn't provide access to the FIFO pointer
    LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
    return (u->LSR & LSR_THRE) ? MIOS32_UART_TX_BUFFER_SIZE : 0;
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in transmit buffer
//! \param[in] uart UART number (0..2)
//! \return number of used bytes
//! \return 0 if uart not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferUsed(u8 uart)
{
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return 0;
  else {
    // fake size, since LPC based uart doesn't provide access to the FIFO pointer
    LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
    return (u->LSR & LSR_THRE) ? 0 : MIOS32_UART_TX_BUFFER_SIZE;
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the transmit buffer
//! \param[in] uart UART number (0..2)
//! \return -1 if UART not available
//! \return -2 if no new byte available
//! \return >= 0: transmitted byte
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferGet(u8 uart)
{
  return -1; // not applicable
}


/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)
//! \param[in] uart UART number (0..2)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if UART not available
//! \return -2 if buffer full or cannot get all requested bytes (retry)
//! \return -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPutMore_NonBlocking(u8 uart, u8 *buffer, u16 len)
{
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1; // UART not available

  LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
  if( (u->LSR & LSR_THRE) == 0 )
    return -2; // buffer full or cannot get all requested bytes (retry)

  // copy bytes to be transmitted into transmit buffer
  u16 i;
  for(i=0; i<len; ++i) {
    while( (u->LSR & LSR_THRE) == 0 ); // blocking... :-/
    u->THR = *buffer++;
  }

  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)<BR>
//! (blocking function)
//! \param[in] uart UART number (0..2)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if UART not available
//! \return -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPutMore(u8 uart, u8 *buffer, u16 len)
{
  s32 error;

  while( (error=MIOS32_UART_TxBufferPutMore_NonBlocking(uart, buffer, len)) == -2 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer
//! \param[in] uart UART number (0..2)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if UART not available
//! \return -2 if buffer full (retry)
//! \return -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPut_NonBlocking(u8 uart, u8 b)
{
  // for more comfortable usage...
  // -> just forward to MIOS32_UART_TxBufferPutMore
  return MIOS32_UART_TxBufferPutMore(uart, &b, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer<BR>
//! (blocking function)
//! \param[in] uart UART number (0..2)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if UART not available
//! \return -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPut(u8 uart, u8 b)
{
  s32 error;

  while( (error=MIOS32_UART_TxBufferPutMore(uart, &b, 1)) == -2 );

  return error;
}

#endif /* MIOS32_DONT_USE_UART */
