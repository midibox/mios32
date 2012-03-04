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

/////////////////////////////////////////////////////////////////////////////
// Pin definitions and UART mappings
/////////////////////////////////////////////////////////////////////////////

// UART peripherals are clocked at CCLK/4
#define UART_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)

// TX MIDI1: P2.0
#define MIOS32_UART0_TX_INIT     { MIOS32_SYS_LPC_PINSEL(2, 0, 2); MIOS32_SYS_LPC_PINMODE(2, 0, 2); MIOS32_SYS_LPC_PINMODE_OD(2, 0, 1); }
// RX MIDI1: P2.1 w/ pull-up enabled
#define MIOS32_UART0_RX_INIT     { MIOS32_SYS_LPC_PINSEL(2, 1, 2); MIOS32_SYS_LPC_PINMODE(2, 1, 0); }
#define MIOS32_UART0             LPC_UART1
#define MIOS32_UART0_IRQ_CHANNEL UART1_IRQn
#define MIOS32_UART0_IRQHANDLER_FUNC void UART1_IRQHandler(void)


// TX MIDI2: P0.0
#define MIOS32_UART1_TX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 0, 2); MIOS32_SYS_LPC_PINMODE(0, 0, 2); MIOS32_SYS_LPC_PINMODE_OD(0, 0, 1); }
// RX MIDI2: P0.1 w/ pull-up enabled
#define MIOS32_UART1_RX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 1, 2); MIOS32_SYS_LPC_PINMODE(0, 1, 0); }
#define MIOS32_UART1             LPC_UART3
#define MIOS32_UART1_IRQ_CHANNEL UART3_IRQn
#define MIOS32_UART1_IRQHANDLER_FUNC void UART3_IRQHandler(void)


// TX MIDI3 (optional at J5B.A7): P0.2
#define MIOS32_UART2_TX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 2, 1); MIOS32_SYS_LPC_PINMODE(0, 2, 2); MIOS32_SYS_LPC_PINMODE_OD(0, 2, 1); }
// RX MIDI3 (optional at J5B.A6): P0.3 w/ pull-up enabled
#define MIOS32_UART2_RX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 3, 1); MIOS32_SYS_LPC_PINMODE(0, 3, 0); }
#define MIOS32_UART2             LPC_UART0
#define MIOS32_UART2_IRQ_CHANNEL UART0_IRQn
#define MIOS32_UART2_IRQHANDLER_FUNC void UART0_IRQHandler(void)


// TX MIDI4 (optional at J4B.SD): P0.10
#define MIOS32_UART3_TX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 10, 1); MIOS32_SYS_LPC_PINMODE(0, 10, 2); MIOS32_SYS_LPC_PINMODE_OD(0, 10, 1); }
// RX MIDI3 (optional at J4B.SC): P0.11 w/ pull-up enabled
#define MIOS32_UART3_RX_INIT     { MIOS32_SYS_LPC_PINSEL(0, 11, 1); MIOS32_SYS_LPC_PINMODE(0, 11, 0); }
#define MIOS32_UART3             LPC_UART2
#define MIOS32_UART3_IRQ_CHANNEL UART2_IRQn
#define MIOS32_UART3_IRQHANDLER_FUNC void UART2_IRQHandler(void)



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
static const LPC_UART_TypeDef *uart_base[4] = { (LPC_UART_TypeDef*)MIOS32_UART0, (LPC_UART_TypeDef*)MIOS32_UART1, (LPC_UART_TypeDef*)MIOS32_UART2, (LPC_UART_TypeDef*)MIOS32_UART3 };

static u32 uart_baudrate[MIOS32_UART_NUM];

// TK: quick fix to ensure that MBSEQV4 can still be compiled: locate rx_buffer in AHB RAM
// can be optionally disabled if required
#ifndef MIOS32_DONT_LOCATE_UART_RXBUFFER_IN_AHB_MEMORY
static u8 __attribute__ ((section (".bss_ahb"))) rx_buffer[MIOS32_UART_NUM][MIOS32_UART_RX_BUFFER_SIZE];
#else
static u8 rx_buffer[MIOS32_UART_NUM][MIOS32_UART_RX_BUFFER_SIZE];
#endif

static volatile u8 rx_buffer_tail[MIOS32_UART_NUM];
static volatile u8 rx_buffer_head[MIOS32_UART_NUM];
static volatile u8 rx_buffer_size[MIOS32_UART_NUM];

#ifndef MIOS32_DONT_LOCATE_UART_TXBUFFER_IN_AHB_MEMORY
static u8 __attribute__ ((section (".bss_ahb"))) tx_buffer[MIOS32_UART_NUM][MIOS32_UART_TX_BUFFER_SIZE];
#else
static u8 tx_buffer[MIOS32_UART_NUM][MIOS32_UART_TX_BUFFER_SIZE];
#endif
static volatile u8 tx_buffer_tail[MIOS32_UART_NUM];
static volatile u8 tx_buffer_head[MIOS32_UART_NUM];
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
#if MIOS32_UART0_ASSIGNMENT != 0
  MIOS32_UART0_TX_INIT;
  MIOS32_UART0_RX_INIT;
#endif
#if MIOS32_UART_NUM >= 2 && MIOS32_UART1_ASSIGNMENT != 0
  MIOS32_UART1_TX_INIT;
  MIOS32_UART1_RX_INIT;
#endif
#if MIOS32_UART_NUM >= 3 && MIOS32_UART2_ASSIGNMENT != 0
  MIOS32_UART2_TX_INIT;
  MIOS32_UART2_RX_INIT;
#endif
#if MIOS32_UART_NUM >= 4 && MIOS32_UART3_ASSIGNMENT != 0
  MIOS32_UART3_TX_INIT;
  MIOS32_UART3_RX_INIT;
#endif

  // clear buffer counters
  int i;
  for(i=0; i<MIOS32_UART_NUM; ++i) {
    rx_buffer_tail[i] = rx_buffer_head[i] = rx_buffer_size[i] = 0;
    tx_buffer_tail[i] = tx_buffer_head[i] = tx_buffer_size[i] = 0;
  }

  // UART configuration
#if MIOS32_UART0_ASSIGNMENT != 0
  MIOS32_UART_BaudrateSet(0, MIOS32_UART0_BAUDRATE);
#endif
#if MIOS32_UART_NUM >=2 && MIOS32_UART1_ASSIGNMENT != 0
  MIOS32_UART_BaudrateSet(1, MIOS32_UART1_BAUDRATE);
#endif
#if MIOS32_UART_NUM >=3 && MIOS32_UART2_ASSIGNMENT != 0
  MIOS32_UART_BaudrateSet(2, MIOS32_UART2_BAUDRATE);
#endif
#if MIOS32_UART_NUM >=4 && MIOS32_UART3_ASSIGNMENT != 0
  MIOS32_UART_BaudrateSet(3, MIOS32_UART3_BAUDRATE);
#endif

  // configure and enable UART interrupts
#if MIOS32_UART0_ASSIGNMENT != 0
  MIOS32_UART0->IER = (1 << 1) | (1 << 0); // enable RBR and THRE (receive/transmit buffer) interrupt
  MIOS32_IRQ_Install(MIOS32_UART0_IRQ_CHANNEL, MIOS32_IRQ_UART_PRIORITY);
#endif

#if MIOS32_UART_NUM >= 2 && MIOS32_UART1_ASSIGNMENT != 0
  MIOS32_UART1->IER = (1 << 1) | (1 << 0); // enable RBR and THRE (receive/transmit buffer) interrupt
  MIOS32_IRQ_Install(MIOS32_UART1_IRQ_CHANNEL, MIOS32_IRQ_UART_PRIORITY);
#endif

#if MIOS32_UART_NUM >= 3 && MIOS32_UART2_ASSIGNMENT != 0
  MIOS32_UART2->IER = (1 << 1) | (1 << 0); // enable RBR and THRE (receive/transmit buffer) interrupt
  MIOS32_IRQ_Install(MIOS32_UART2_IRQ_CHANNEL, MIOS32_IRQ_UART_PRIORITY);
#endif

#if MIOS32_UART_NUM >= 4 && MIOS32_UART2_ASSIGNMENT != 0
  MIOS32_UART3->IER = (1 << 1) | (1 << 0); // enable RBR and THRE (receive/transmit buffer) interrupt
  MIOS32_IRQ_Install(MIOS32_UART3_IRQ_CHANNEL, MIOS32_IRQ_UART_PRIORITY);
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

  // Inspired from lpc17xx_uart.c of the IEC60335 Class B library v1.0
  u32 uClk = UART_PERIPHERAL_FRQ;
  u32 calcBaudrate = 0;
  u32 temp = 0;

  u32 mulFracDiv, dividerAddFracDiv;
  u32 diviser = 0 ;
  u32 mulFracDivOptimal = 1;
  u32 dividerAddOptimal = 0;
  u32 diviserOptimal = 0;

  u32 relativeError = 0;
  u32 relativeOptimalError = 100000;

  uClk = uClk >> 4; /* div by 16 */
  /* In the Uart IP block, baud rate is calculated using FDR and DLL-DLM registers
   * The formula is :
   * BaudRate= uClk * (mulFracDiv/(mulFracDiv+dividerAddFracDiv) / (16 * (DLL)
   * It involves floating point calculations. That's the reason the formulae are adjusted with
   * Multiply and divide method.*/
  /* The value of mulFracDiv and dividerAddFracDiv should comply to the following expressions:
   * 0 < mulFracDiv <= 15, 0 <= dividerAddFracDiv <= 15 */
  for(mulFracDiv = 1 ; mulFracDiv <= 15 ;mulFracDiv++) {
    for(dividerAddFracDiv = 0 ; dividerAddFracDiv <= 15 ;dividerAddFracDiv++) {
      temp = (mulFracDiv * uClk) / ((mulFracDiv + dividerAddFracDiv));

      diviser = temp / baudrate;
      if ((temp % baudrate) > (baudrate / 2))
	diviser++;

      if (diviser > 2 && diviser < 65536) {
	calcBaudrate = temp / diviser;

	if (calcBaudrate <= baudrate)
	  relativeError = baudrate - calcBaudrate;
	else
	  relativeError = calcBaudrate - baudrate;

	if( (relativeError < relativeOptimalError) ) {
	  mulFracDivOptimal = mulFracDiv ;
	  dividerAddOptimal = dividerAddFracDiv;
	  diviserOptimal = diviser;
	  relativeOptimalError = relativeError;
	  if (relativeError == 0)
	    break;
	}
      } /* End of if */
    } /* end of inner for loop */

    if( relativeError == 0 )
      break;
  } /* end of outer for loop  */

  u->LCR = 0x83; // 8 bits, no Parity, 1 Stop bit, DLAB=1
  u->DLM = (diviserOptimal >> 8) & 0xff;
  u->DLL = diviserOptimal & 0xff;
  u->LCR = 0x03; // disable access to divisor latches
  u->FDR = ((mulFracDivOptimal&0xf) << 4) | (dividerAddOptimal&0xf);
  u->FCR = 0x07; // Enable and reset TX and RX FIFO

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
  else
    return MIOS32_UART_RX_BUFFER_SIZE - rx_buffer_size[uart];
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
  else
    return rx_buffer_size[uart];
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

  if( !rx_buffer_size[uart] )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[uart][rx_buffer_tail[uart]];
  if( ++rx_buffer_tail[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    rx_buffer_tail[uart] = 0;
  --rx_buffer_size[uart];
  MIOS32_IRQ_Enable();

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

  if( !rx_buffer_size[uart] )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[uart][rx_buffer_tail[uart]];
  MIOS32_IRQ_Enable();

  return b; // return received byte
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
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1; // UART not available

  if( rx_buffer_size[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    return -2; // buffer full (retry)

  // copy received byte into receive buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();
  rx_buffer[uart][rx_buffer_head[uart]] = b;
  if( ++rx_buffer_head[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    rx_buffer_head[uart] = 0;
  ++rx_buffer_size[uart];
  MIOS32_IRQ_Enable();

  return 0; // no error
#endif
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
  else
    return MIOS32_UART_TX_BUFFER_SIZE - tx_buffer_size[uart];
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
  else
    return tx_buffer_size[uart];
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
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= MIOS32_UART_NUM )
    return -1; // UART not available

  if( !tx_buffer_size[uart] )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = tx_buffer[uart][tx_buffer_tail[uart]];
  if( ++tx_buffer_tail[uart] >= MIOS32_UART_TX_BUFFER_SIZE )
    tx_buffer_tail[uart] = 0;
  --tx_buffer_size[uart];
  MIOS32_IRQ_Enable();

  return b; // return transmitted byte
#endif
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

  if( (tx_buffer_size[uart]+len) >= MIOS32_UART_TX_BUFFER_SIZE )
    return -2; // buffer full or cannot get all requested bytes (retry)

  // copy bytes to be transmitted into transmit buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();

  // unfortunately LPC based uart doesn't provide access to the FIFO pointer,
  // currently I don't see another way than using only a single byte of the FIFO, and to push the remaining bytes into the ringbuffer

  int start_byte = 0;
  if( !tx_buffer_size[uart] ) {
    start_byte = 1;

    LPC_UART_TypeDef *u = (LPC_UART_TypeDef *)uart_base[uart];
    u->THR = *buffer++;
  }

  // put remaining bytes into ringbuffer
  int i;
  for(i=start_byte; i<len; ++i) {
    tx_buffer[uart][tx_buffer_head[uart]] = *buffer++;
    ++tx_buffer_size[uart];

    if( ++tx_buffer_head[uart] >= MIOS32_UART_TX_BUFFER_SIZE )
      tx_buffer_head[uart] = 0;
  }

  MIOS32_IRQ_Enable();

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


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for first UART
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_UART_NUM >= 1
MIOS32_UART0_IRQHANDLER_FUNC
{
  u32 iir_intid = MIOS32_UART0->IIR & 0xe; // IIR will be released with this access

  if( iir_intid == 0x4 ) { // IntId = 0x2 (RDA)

    if( (MIOS32_UART0->LSR & LSR_RDR) ) { 
      u8 b = MIOS32_UART0->RBR;

#if MIOS32_UART0_ASSIGNMENT == 1
      s32 status = MIOS32_MIDI_SendByteToRxCallback(UART0, b);
#else
      s32 status = 0;
#endif

      // read byte from FIFO and put into SW based ringbuffer
      if( status == 0 && MIOS32_UART_RxBufferPut(0, b) < 0 ) {
	// here we could add some error handling
      }
    }
  } else if( iir_intid == 0x2 ) { // IntId = 0x1 (THRE)
    if( MIOS32_UART_TxBufferUsed(0) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(0);
      if( b < 0 ) {
	// here we could add some error handling
	b = 0xff;
      }
      MIOS32_UART0->THR = b;
    }
  }
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for second UART
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_UART_NUM >= 2
MIOS32_UART1_IRQHANDLER_FUNC
{
  u32 iir_intid = MIOS32_UART1->IIR & 0xe; // IIR will be released with this access

  if( iir_intid == 0x4 ) { // IntId = 0x2 (RDA)

    if( (MIOS32_UART1->LSR & LSR_RDR) ) { 
      u8 b = MIOS32_UART1->RBR;

#if MIOS32_UART1_ASSIGNMENT == 1
      s32 status = MIOS32_MIDI_SendByteToRxCallback(UART1, b);
#else
      s32 status = 0;
#endif

      // read byte from FIFO and put into SW based ringbuffer
      if( status == 0 && MIOS32_UART_RxBufferPut(1, b) < 0 ) {
	// here we could add some error handling
      }
    }
  } else if( iir_intid == 0x2 ) { // IntId = 0x1 (THRE)
    if( MIOS32_UART_TxBufferUsed(1) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(1);
      if( b < 0 ) {
	// here we could add some error handling
	b = 0xff;
      }
      MIOS32_UART1->THR = b;
    }
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for third UART
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_UART_NUM >= 3
MIOS32_UART2_IRQHANDLER_FUNC
{
  u32 iir_intid = MIOS32_UART2->IIR & 0xe; // IIR will be released with this access

  if( iir_intid == 0x4 ) { // IntId = 0x2 (RDA)

    if( (MIOS32_UART2->LSR & LSR_RDR) ) { 
      u8 b = MIOS32_UART2->RBR;

#if MIOS32_UART2_ASSIGNMENT == 1
      s32 status = MIOS32_MIDI_SendByteToRxCallback(UART2, b);
#else
      s32 status = 0;
#endif

      // read byte from FIFO and put into SW based ringbuffer
      if( status == 0 && MIOS32_UART_RxBufferPut(2, b) < 0 ) {
	// here we could add some error handling
      }
    }
  } else if( iir_intid == 0x2 ) { // IntId = 0x1 (THRE)
    if( MIOS32_UART_TxBufferUsed(2) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(2);
      if( b < 0 ) {
	// here we could add some error handling
	b = 0xff;
      }
      MIOS32_UART2->THR = b;
    }
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for fourth UART
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_UART_NUM >= 4
MIOS32_UART3_IRQHANDLER_FUNC
{
  u32 iir_intid = MIOS32_UART3->IIR & 0xe; // IIR will be released with this access

  if( iir_intid == 0x4 ) { // IntId = 0x2 (RDA)

    if( (MIOS32_UART3->LSR & LSR_RDR) ) { 
      u8 b = MIOS32_UART3->RBR;

#if MIOS32_UART3_ASSIGNMENT == 1
      s32 status = MIOS32_MIDI_SendByteToRxCallback(UART3, b);
#else
      s32 status = 0;
#endif

      // read byte from FIFO and put into SW based ringbuffer
      if( status == 0 && MIOS32_UART_RxBufferPut(3, b) < 0 ) {
	// here we could add some error handling
      }
    }
  } else if( iir_intid == 0x2 ) { // IntId = 0x1 (THRE)
    if( MIOS32_UART_TxBufferUsed(3) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(3);
      if( b < 0 ) {
	// here we could add some error handling
	b = 0xff;
      }
      MIOS32_UART3->THR = b;
    }
  }
}
#endif


#endif /* MIOS32_DONT_USE_UART */
