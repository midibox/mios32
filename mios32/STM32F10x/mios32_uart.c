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
#if !defined(MIOS32_DONT_USE_UART)


/////////////////////////////////////////////////////////////////////////////
// Pin definitions and USART mappings
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_UART0_TX_PORT     GPIOA
#define MIOS32_UART0_TX_PIN      GPIO_Pin_9
#define MIOS32_UART0_RX_PORT     GPIOA
#define MIOS32_UART0_RX_PIN      GPIO_Pin_10
#define MIOS32_UART0             USART1
#define MIOS32_UART0_IRQ_CHANNEL USART1_IRQn
#define MIOS32_UART0_IRQHANDLER_FUNC void USART1_IRQHandler(void)
#define MIOS32_UART0_REMAP_FUNC  {}

#define MIOS32_UART1_TX_PORT     GPIOC
#define MIOS32_UART1_TX_PIN      GPIO_Pin_10
#define MIOS32_UART1_RX_PORT     GPIOC
#define MIOS32_UART1_RX_PIN      GPIO_Pin_11
#define MIOS32_UART1             USART3
#define MIOS32_UART1_IRQ_CHANNEL USART3_IRQn
#define MIOS32_UART1_IRQHANDLER_FUNC void USART3_IRQHandler(void)
#define MIOS32_UART1_REMAP_FUNC  { GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE); }

#define MIOS32_UART2_TX_PORT     GPIOA
#define MIOS32_UART2_TX_PIN      GPIO_Pin_2
#define MIOS32_UART2_RX_PORT     GPIOA
#define MIOS32_UART2_RX_PIN      GPIO_Pin_3
#define MIOS32_UART2             USART2
#define MIOS32_UART2_IRQ_CHANNEL USART2_IRQn
#define MIOS32_UART2_IRQHANDLER_FUNC void USART2_IRQHandler(void)
#define MIOS32_UART2_REMAP_FUNC  {}


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_UART_NUM >= 1
static u32 uart_baudrate[MIOS32_UART_NUM];

static u8 rx_buffer[MIOS32_UART_NUM][MIOS32_UART_RX_BUFFER_SIZE];
static volatile u8 rx_buffer_tail[MIOS32_UART_NUM];
static volatile u8 rx_buffer_head[MIOS32_UART_NUM];
static volatile u8 rx_buffer_size[MIOS32_UART_NUM];

static u8 tx_buffer[MIOS32_UART_NUM][MIOS32_UART_TX_BUFFER_SIZE];
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
  GPIO_InitTypeDef GPIO_InitStructure;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_UART_NUM == 0
  return -1; // no UARTs
#else

  // map UART pins
  MIOS32_UART0_REMAP_FUNC;
#if MIOS32_UART_NUM >= 2
  MIOS32_UART1_REMAP_FUNC;
#endif
#if MIOS32_UART_NUM >= 3
  MIOS32_UART2_REMAP_FUNC;
#endif

  // configure UART pins
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  // outputs as open-drain
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART0_TX_PIN;
#if MIOS32_UART0_TX_OD
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
  GPIO_Init(MIOS32_UART0_TX_PORT, &GPIO_InitStructure);

#if MIOS32_UART_NUM >= 2
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART1_TX_PIN;
#if MIOS32_UART1_TX_OD
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
  GPIO_Init(MIOS32_UART1_TX_PORT, &GPIO_InitStructure);
#endif

#if MIOS32_UART_NUM >= 3
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART2_TX_PIN;
#if MIOS32_UART2_TX_OD
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
  GPIO_Init(MIOS32_UART2_TX_PORT, &GPIO_InitStructure);
#endif

  // inputs with internal pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART0_RX_PIN;
  GPIO_Init(MIOS32_UART0_RX_PORT, &GPIO_InitStructure);
#if MIOS32_UART_NUM >= 2
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART1_RX_PIN;
  GPIO_Init(MIOS32_UART1_RX_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_UART_NUM >= 3
  GPIO_InitStructure.GPIO_Pin = MIOS32_UART2_RX_PIN;
  GPIO_Init(MIOS32_UART2_RX_PORT, &GPIO_InitStructure);
#endif

  // enable all USART clocks
  // TODO: more generic approach for different UART selections
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3, ENABLE);

  // USART configuration
  MIOS32_UART_BaudrateSet(0, MIOS32_UART0_BAUDRATE);
#if MIOS32_UART_NUM >=2
  MIOS32_UART_BaudrateSet(1, MIOS32_UART1_BAUDRATE);
#endif
#if MIOS32_UART_NUM >=3
  MIOS32_UART_BaudrateSet(2, MIOS32_UART2_BAUDRATE);
#endif

  // configure and enable UART interrupts
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_UART0_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_UART_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  USART_ITConfig(MIOS32_UART0, USART_IT_RXNE, ENABLE);

#if MIOS32_UART_NUM >= 2
  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_UART1_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_UART_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  USART_ITConfig(MIOS32_UART1, USART_IT_RXNE, ENABLE);
#endif

#if MIOS32_UART_NUM >= 3
  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_UART2_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_UART_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  USART_ITConfig(MIOS32_UART2, USART_IT_RXNE, ENABLE);
#endif

  // clear buffer counters
  int i;
  for(i=0; i<MIOS32_UART_NUM; ++i) {
    rx_buffer_tail[i] = rx_buffer_head[i] = rx_buffer_size[i] = 0;
    tx_buffer_tail[i] = tx_buffer_head[i] = tx_buffer_size[i] = 0;
  }

  // enable UARTs
  USART_Cmd(MIOS32_UART0, ENABLE);
#if MIOS32_UART_NUM >= 2
  USART_Cmd(MIOS32_UART1, ENABLE);
#endif
#if MIOS32_UART_NUM >= 3
  USART_Cmd(MIOS32_UART2, ENABLE);
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

  // USART configuration
  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_InitStructure.USART_BaudRate = baudrate;

  switch( uart ) {
  case 0: USART_Init(MIOS32_UART0, &USART_InitStructure); break;
#if MIOS32_UART_NUM >= 2
  case 1: USART_Init(MIOS32_UART1, &USART_InitStructure); break;
#endif
#if MIOS32_UART_NUM >= 3
  case 2: USART_Init(MIOS32_UART2, &USART_InitStructure); break;
#endif
  default:
    return -2; // not prepared
  }

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

  u16 i;
  for(i=0; i<len; ++i) {
    tx_buffer[uart][tx_buffer_head[uart]] = *buffer++;

    if( ++tx_buffer_head[uart] >= MIOS32_UART_TX_BUFFER_SIZE )
      tx_buffer_head[uart] = 0;

    // enable Tx interrupt if buffer was empty
    if( ++tx_buffer_size[uart] == 1 ) {
      switch( uart ) {
        case 0: MIOS32_UART0->CR1 |= (1 << 7); break; // enable TXE interrupt (TXEIE=1)
        case 1: MIOS32_UART1->CR1 |= (1 << 7); break; // enable TXE interrupt (TXEIE=1)
        case 2: MIOS32_UART2->CR1 |= (1 << 7); break; // enable TXE interrupt (TXEIE=1)
        default: MIOS32_IRQ_Enable(); return -3; // uart not supported by routine (yet)
      }
    }
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
  if( MIOS32_UART0->SR & (1 << 5) ) { // check if RXNE flag is set
    u8 b = MIOS32_UART0->DR;

#if MIOS32_UART0_ASSIGNMENT == 1
    s32 status = MIOS32_MIDI_SendByteToRxCallback(UART0, b);
#else
    s32 status = 0;
#endif

    if( status == 0 && MIOS32_UART_RxBufferPut(0, b) < 0 ) {
      // here we could add some error handling
    }
  }

  if( MIOS32_UART0->SR & (1 << 7) ) { // check if TXE flag is set
    if( MIOS32_UART_TxBufferUsed(0) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(0);
      if( b < 0 ) {
	// here we could add some error handling
	MIOS32_UART0->DR = 0xff;
      } else {
	MIOS32_UART0->DR = b;
      }
    } else {
      MIOS32_UART0->CR1 &= ~(1 << 7); // disable TXE interrupt (TXEIE=0)
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
  if( MIOS32_UART1->SR & (1 << 5) ) { // check if RXNE flag is set
    u8 b = MIOS32_UART1->DR;

#if MIOS32_UART1_ASSIGNMENT == 1
    s32 status = MIOS32_MIDI_SendByteToRxCallback(UART1, b);
#else
    s32 status = 0;
#endif

    if( status == 0 && MIOS32_UART_RxBufferPut(1, b) < 0 ) {
      // here we could add some error handling
    }
  }
  
  if( MIOS32_UART1->SR & (1 << 7) ) { // check if TXE flag is set
    if( MIOS32_UART_TxBufferUsed(1) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(1);
      if( b < 0 ) {
	// here we could add some error handling
	MIOS32_UART1->DR = 0xff;
      } else {
	MIOS32_UART1->DR = b;
      }
    } else {
      MIOS32_UART1->CR1 &= ~(1 << 7); // disable TXE interrupt (TXEIE=0)
    }
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for third UART
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_UART_NUM >= 2
MIOS32_UART2_IRQHANDLER_FUNC
{
  if( MIOS32_UART2->SR & (1 << 5) ) { // check if RXNE flag is set
    u8 b = MIOS32_UART2->DR;

#if MIOS32_UART2_ASSIGNMENT == 1
    s32 status = MIOS32_MIDI_SendByteToRxCallback(UART2, b);
#else
    s32 status = 0;
#endif

    if( status == 0 && MIOS32_UART_RxBufferPut(2, b) < 0 ) {
      // here we could add some error handling
    }
  }
  
  if( MIOS32_UART2->SR & (1 << 7) ) { // check if TXE flag is set
    if( MIOS32_UART_TxBufferUsed(2) > 0 ) {
      s32 b = MIOS32_UART_TxBufferGet(2);
      if( b < 0 ) {
	// here we could add some error handling
	MIOS32_UART2->DR = 0xff;
      } else {
	MIOS32_UART2->DR = b;
      }
    } else {
      MIOS32_UART2->CR1 &= ~(1 << 7); // disable TXE interrupt (TXEIE=0)
    }
  }
}
#endif


#endif /* MIOS32_DONT_USE_UART */
