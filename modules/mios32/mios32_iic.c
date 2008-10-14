// $Id$
/*
 * IIC functions for MIOS32
 *
 * Interrupt driven approach, inspired by STM32 AN15021, enriched by
 * more generic buffer send/receive routines and a proper error/failsave handling
 *
 * Some remarks:
 * A common polling method would work unstable on receive transactions if the
 * sending rouine is interrupted, so that NAK + STOP cannot be requested 
 * early enough before the last byte should be received
 * See also this enlightening forum thread http://www.st.com/mcu/forums-cat-6701-23.html
 *
 * DMA transfers are no solution for MIOS32, as it should stay compatible
 * to mid-range devices (-> no DMA2), and the available DMA channels which
 * could be used for I2C2 are already allocated by SPI1 and SPI2
 *
 * The interrupt has to run with higher priority - it has to be ensured that
 * the received data is read from DR register before the ACK of the previous
 * byte is sent, otherwise the I2C peripheral can get busy permanently, waiting
 * for a NAK which it will never transmit as it will never request a byte in 
 * master mode -> lifelock -> design flaw
 *
 * I must highlight that I don't really like the I2C concept of STM32. It's 
 * unbelievable that the guys specified a pipeline based approach, but 
 * haven't put the NAK/Stop condition into the transaction pipeline
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
#if !defined(MIOS32_DONT_USE_IIC)

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Pin definitions
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_IIC_SCL_PORT    GPIOB
#define MIOS32_IIC_SCL_PIN     GPIO_Pin_10
#define MIOS32_IIC_SDA_PORT    GPIOB
#define MIOS32_IIC_SDA_PIN     GPIO_Pin_11


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////


// taken from STM32 v2.0.3 library - define it here if an older library is used
#ifndef I2C_EVENT_MASTER_BYTE_TRANSMITTING
#define I2C_EVENT_MASTER_BYTE_TRANSMITTING ((u32)0x00070080) /* TRA, BUSY, MSL, TXE flags */
#endif


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static void MIOS32_IIC_InitPeripheral(void);


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned BUSY:1;
    unsigned STOP_REQUESTED:1;
    unsigned RX_OVERRUN:1;
    unsigned ABORT_IF_FIRST_BYTE_0:1;
  };
} transfer_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 iic_address;
static u8 tx_buffer[MIOS32_IIC_BUFFER_SIZE];
static u8 *rx_buffer_ptr;
static volatile u8 buffer_len;
static volatile u8 buffer_ix;

static volatile transfer_state_t transfer_state;
static volatile s32 transfer_error;
static volatile s32 last_transfer_error = 0;

static volatile u8 iic_semaphore = 1; // if 1, interface hasn't been initialized yet, or is allocated by another task


/////////////////////////////////////////////////////////////////////////////
// Initializes IIC interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  I2C_InitTypeDef  I2C_InitStructure;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // configure IIC pins in open drain mode
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;

  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC_SCL_PIN;
  GPIO_Init(MIOS32_IIC_SCL_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC_SDA_PIN;
  GPIO_Init(MIOS32_IIC_SDA_PORT, &GPIO_InitStructure);

  // configure I2C peripheral
  MIOS32_IIC_InitPeripheral();

  // now accessible for other tasks
  iic_semaphore = 0;

  // configure and enable I2C2 interrupts
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQChannel;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_IIC_EV_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_IRQ_IIC_ER_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// internal function to (re-)initialize the I2C peripheral
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_IIC_InitPeripheral(void)
{
  I2C_InitTypeDef  I2C_InitStructure;

  // enable peripheral clock of I2C
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

  // trigger software reset via I2C_DeInit
  I2C_DeInit(I2C2);

  // clear transfer state and error value
  transfer_state.ALL = 0;
  transfer_error = 0;

  // enable I2C peripheral
  I2C_Cmd(I2C2, ENABLE);

  // configure I2C peripheral
  I2C_StructInit(&I2C_InitStructure);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = MIOS32_IIC_BUS_FREQUENCY; // note that the STM32 driver handles value >400000 differently!
  I2C_Init(I2C2, &I2C_InitStructure);
}


/////////////////////////////////////////////////////////////////////////////
// Semaphore handling: requests the IIC interface
// IN: <semaphore_type> is either IIC_Blocking or IIC_Non_Blocking
// OUT: if Non_Blocking: returns -1 to request a retry
//      returns 0 if IIC interface free
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferBegin(mios32_iic_semaphore_t semaphore_type)
{
  s32 status = -1;

  do {
    vPortEnterCritical(); // port specific FreeRTOS function to disable IRQs (nested)
    if( !iic_semaphore ) {
      iic_semaphore = 1;
      status = 0;
    }
    vPortExitCritical(); // port specific FreeRTOS function to enable IRQs (nested)
  } while( semaphore_type == IIC_Blocking && status != 0 );

  // clear transfer errors of last transmission
  last_transfer_error = 0;
  transfer_error = 0;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// Semaphore handling: releases the IIC interface for other tasks
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferFinished(void)
{
  iic_semaphore = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the last transfer error
// Will be updated by MIOS32_IIC_TransferCheck(), so that the error status
// doesn't get lost (the check function will return 0 when called again)
// Will be cleared when a new transfer has been started successfully
// IN: -
// OUT: last error status
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_LastErrorGet(void)
{
  return last_transfer_error;
}


/////////////////////////////////////////////////////////////////////////////
// Checks if transfer is finished
// IN: -
// OUT: 0 if no ongoing transfer
//      1 if ongoing transfer
//      <0 if error during transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferCheck(void)
{
  // error during transfer?
  if( transfer_error ) {
    // store error status for MIOS32_IIC_LastErrorGet() function
    last_transfer_error = transfer_error;
    // clear current error status
    transfer_error = 0;
    // and exit
    return last_transfer_error;
  }

  // ongoing transfer?
  if( transfer_state.BUSY )
    return 1;

  // no transfer
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Waits until transfer is finished
// IN: -
// OUT: 0 if no ongoing transfer
//      <0 if error during transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferWait(void)
{
  u32 repeat_ctr = MIOS32_IIC_TIMEOUT_VALUE;
  u8 last_buffer_ix = buffer_ix;

  while( --repeat_ctr > 0 ) {
    // check if buffer index has changed - if so, reload repeat counter
    if( buffer_ix != last_buffer_ix ) {
      repeat_ctr = MIOS32_IIC_TIMEOUT_VALUE;
      last_buffer_ix = buffer_ix;
    }

    // get transfer state
    s32 check_state = MIOS32_IIC_TransferCheck();

    // exit if transfer finished or error detected
    if( check_state <= 0 )
      return check_state;
  }

  // timeout error - something is stalling...

  // send stop condition
  I2C_GenerateSTOP(I2C2, ENABLE);

  // re-initialize peripheral
  MIOS32_IIC_InitPeripheral();

  return (last_transfer_error=MIOS32_IIC_ERROR_TIMEOUT);
}


/////////////////////////////////////////////////////////////////////////////
// Starts a new transfer. If this function is called during an ongoing
// transfer, we wait until it has been finished and setup the new transfer
// IN: <transfer> type:
//       - IIC_Read: a common Read transfer
//       - IIC_Write: a common Write transfer
//       - IIC_Read_AbortIfFirstByteIs0: used to poll MBHP_IIC_MIDI: aborts transfer
//         if the first received byte is 0
//       - IIC_Write_WithoutStop: don't send stop condition after transfer to allow
//         a restart condition (e.g. used to access EEPROMs)
// OUT: 0 no error
//      < 0 on errors, if MIOS32_IIC_ERROR_PREV_OFFSET is added, the previous
//      transfer got an error (the previous task didn't use MIOS32_IIC_TransferWait()
//      to poll the transfer state)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Transfer(mios32_iic_transfer_t transfer, u8 address, u8 *buffer, u8 len)
{
  s32 error;

  // wait until previous transfer finished
  if( error = MIOS32_IIC_TransferWait() ) // transmission error during previous transfer
    return error + MIOS32_IIC_ERROR_PREV_OFFSET;

  // disable interrupts
  I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

  // clear transfer state and error value
  transfer_state.ALL = 0;
  transfer_error = 0;

  // set buffer length and start index
  buffer_len = len;
  buffer_ix = 0;

  // branch depending on read/write
  if( transfer == IIC_Read || transfer == IIC_Read_AbortIfFirstByteIs0 ) {
    // take new address/buffer/len
    iic_address = address | 1; // set bit 0 for read operation
    rx_buffer_ptr = buffer;
    // special option for optimized MBHP_IIC_MIDI
    transfer_state.ABORT_IF_FIRST_BYTE_0 = transfer == IIC_Read_AbortIfFirstByteIs0 ? 1 : 0;
  } else if( transfer == IIC_Write || transfer == IIC_Write_WithoutStop ) {
    // check length
    if( len > MIOS32_IIC_BUFFER_SIZE )
      return (last_transfer_error=MIOS32_IIC_ERROR_TX_BUFFER_NOT_BIG_ENOUGH);

    // take new address/buffer/len
    iic_address = address & 0xfe; // clear bit 0 for write operation
    rx_buffer_ptr = NULL; // ensure that nothing will be received

    if( len ) {
      // copy destination buffer into tx_buffer
      u8 *tmp_buffer_ptr = tx_buffer;
      u8 i;
      for(i=0; i<len; ++i)
	*tmp_buffer_ptr++ = *buffer++; // copies faster than using indexed arrays
    }
  } else
    return (last_transfer_error=MIOS32_IIC_ERROR_UNSUPPORTED_TRANSFER_TYPE);

  // start with ACK
  I2C_AcknowledgeConfig(I2C2, ENABLE);

  // enable I2V2 event, buffer and error interrupt
  I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);

  // clear last error status
  last_transfer_error = 0;

  // notify that transfer has started
  transfer_state.BUSY = 1;

  // send start condition
  I2C_GenerateSTART(I2C2, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// interrupt handler for I2C events
/////////////////////////////////////////////////////////////////////////////
void I2C2_EV_IRQHandler(void)
{
  u8 b;

#if 0
  // Receiver handling:
  // enable this to test failsave handling, if the IRQ won't be executed early
  // enough, so that DR won't be read before the previous ACK has been sent
  volatile u32 delay;
  for(delay=0; delay<1000; ++delay);
#endif

  switch( I2C_GetLastEvent(I2C2) ) {
    case I2C_EVENT_MASTER_MODE_SELECT:
      // send IIC address
      I2C_Send7bitAddress(I2C2, iic_address, 
			  (iic_address & 1)
			  ? I2C_Direction_Receiver
			  : I2C_Direction_Transmitter);

      // address sent, if no byte should be sent: request NAK now!
      if( buffer_len == 0 ) {
	// request NAK
	I2C_AcknowledgeConfig(I2C2, DISABLE);
	// request stop condition
	I2C_GenerateSTOP(I2C2, ENABLE);
	transfer_state.STOP_REQUESTED = 1;
      }
      break;

    case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:
      // address sent, transfer first byte - check if we already have to request NAK/Stop
      if( buffer_len == 1 ) {
	// request NAK
	I2C_AcknowledgeConfig(I2C2, DISABLE);
	// request stop condition
	I2C_GenerateSTOP(I2C2, ENABLE);
	transfer_state.STOP_REQUESTED = 1;
      }
      break;
      
    case I2C_EVENT_MASTER_BYTE_RECEIVED:
      // get received data
      b = I2C_ReceiveData(I2C2);

      if( rx_buffer_ptr == NULL ) {
	// failsave: really requested a receive transfer?
	// ignore...
      } else if( buffer_ix >= buffer_len ) {
	// failsave: still place in buffer?
	// stop transfer, notify error
	transfer_state.RX_OVERRUN = 1;
	// request NAK
	I2C_AcknowledgeConfig(I2C2, DISABLE);
	// request stop condition
	I2C_GenerateSTOP(I2C2, ENABLE);
	transfer_state.STOP_REQUESTED = 1;
      } else {
	rx_buffer_ptr[buffer_ix++] = b;
	// request NAK and stop condition before receiving last data
	if( (buffer_ix == buffer_len-1) || (transfer_state.ABORT_IF_FIRST_BYTE_0 && buffer_ix == 1 && b == 0x00) ) {
	  // request NAK
	  I2C_AcknowledgeConfig(I2C2, DISABLE);
	  // request stop condition
	  I2C_GenerateSTOP(I2C2, ENABLE);
	  transfer_state.STOP_REQUESTED = 1;
	} else if( transfer_state.STOP_REQUESTED ) { // last byte received
	  transfer_state.BUSY = 0;
#if 0
	  // disabled due to peripheral imperfections (sometimes an additional byte is received)
	  // set error state to Rx Overrun if notified on previous byte
	  if( transfer_state.RX_OVERRUN ) {
	    transfer_error = MIOS32_IIC_ERROR_RX_BUFFER_OVERRUN;
	  }
#endif
	  // disable all interrupts
	  I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
	}
      }
      break;

    case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
    case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
      if( buffer_ix < buffer_len ) {
	I2C_SendData(I2C2, tx_buffer[buffer_ix++]);
      } else if( buffer_len == 0 ) {
	// only relevant if no bytes should be sent (only address to check ACK response)
	// transfer finished
	transfer_state.BUSY = 0;
	// disable all interrupts
	I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      } else {
	// send stop condition
	I2C_GenerateSTOP(I2C2, ENABLE);
	// request unbusy
	transfer_state.STOP_REQUESTED = 1;

	// Disable the I2C_IT_BUF interrupt after sending the last buffer data 
	// (last EV8) to not allow a new interrupt with TxE - only BTF will generate it
	I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);
      }

      break;

    case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
      if( transfer_state.STOP_REQUESTED ) {
	// transfer finished
	transfer_state.BUSY = 0;
	// disable all interrupts
	I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      }
      break;

    default:
      // an unexpected event has triggered the interrupt
      // e.g. this can happen during receive if BTF flag is set, so that the pipeline flow cannot be guaranteed
      // we have to ensure, that the handler won't be invoked endless, therefore:
      // disable interrupts
      I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      // notify error
      transfer_state.BUSY = 0;
      transfer_error = MIOS32_IIC_ERROR_UNEXPECTED_EVENT;
      // do dummy read to send NAK + STOP condition
      I2C_AcknowledgeConfig(I2C2, DISABLE);
      b = I2C_ReceiveData(I2C2);
      I2C_GenerateSTOP(I2C2, ENABLE);
  }
}


/////////////////////////////////////////////////////////////////////////////
// interrupt handler for I2C errors
/////////////////////////////////////////////////////////////////////////////
void I2C2_ER_IRQHandler(void)
{
  // note that only one error number is available
  // the order of these checks defines the priority

  // bus error (start/stop condition during read
  // unlikely, should only be relevant for slave mode?)
  if( I2C_GetITStatus(I2C2, I2C_IT_BERR) ) {
    I2C_ClearITPendingBit(I2C2, I2C_IT_BERR);
    transfer_error = MIOS32_IIC_ERROR_BUS;
  }

  // arbitration lost
  if( I2C_GetITStatus(I2C2, I2C_IT_ARLO) ) {
    I2C_ClearITPendingBit(I2C2, I2C_IT_ARLO);
    transfer_error = MIOS32_IIC_ERROR_ARBITRATION_LOST;
  }

  // no acknowledge received from slave (e.g. slave not connected)
  if( I2C_GetITStatus(I2C2, I2C_IT_AF) ) {
    I2C_ClearITPendingBit(I2C2, I2C_IT_AF);
    transfer_error = MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED;
    // send stop condition to release bus
    I2C_GenerateSTOP(I2C2, ENABLE);
  }

  // disable interrupts
  I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

  // notify that transfer has finished (due to the error)
  transfer_state.BUSY = 0;
}


#endif /* MIOS32_DONT_USE_IIC */
