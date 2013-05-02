// $Id$
//! \defgroup MIOS32_IIC
//!
//! IIC driver for MIOS32
//!
//! Interrupt driven approach, inspired by STM32 AN15021, enriched by
//! more generic buffer send/receive routines and a proper error/failsave handling
//!
//! Some remarks:
//! A common polling method would work unstable on receive transactions if the
//! sending rouine is interrupted, so that NAK + STOP cannot be requested 
//! early enough before the last byte should be received
//! See also this enlightening forum thread http://www.st.com/mcu/forums-cat-6701-23.html
//!
//! DMA transfers are no solution for MIOS32, as it should stay compatible
//! to mid-range devices (-> no DMA2), and the available DMA channels which
//! could be used for I2C2 are already allocated by SPI1 and SPI2
//!
//! The interrupt has to run with higher priority - it has to be ensured that
//! the received data is read from DR register before the ACK of the previous
//! byte is sent, otherwise the I2C peripheral can get busy permanently, waiting
//! for a NAK which it will never transmit as it will never request a byte in 
//! master mode -> lifelock -> design flaw
//!
//! I must highlight that I don't really like the I2C concept of STM32. It's 
//! unbelievable that the guys specified a pipeline based approach, but 
//! haven't put the NAK/Stop condition into the transaction pipeline
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 *  IRQ handlers reworked by Matthias Mächler (juli 2009)
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_IIC)

/////////////////////////////////////////////////////////////////////////////
// Pin definitions
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_IIC0_SCL_PORT    GPIOB
#define MIOS32_IIC0_SCL_PIN     GPIO_Pin_10
#define MIOS32_IIC0_SDA_PORT    GPIOB
#define MIOS32_IIC0_SDA_PIN     GPIO_Pin_11

#define MIOS32_IIC1_SCL_PORT    GPIOB
#define MIOS32_IIC1_SCL_PIN     GPIO_Pin_6
#define MIOS32_IIC1_SDA_PORT    GPIOB
#define MIOS32_IIC1_SDA_PIN     GPIO_Pin_7

/////////////////////////////////////////////////////////////////////////////
// Duty cycle definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef MIOS32_IIC0_DUTYCYCLE
#define MIOS32_IIC0_DUTYCYCLE I2C_DutyCycle_2
#endif

#ifndef MIOS32_IIC1_DUTYCYCLE
#define MIOS32_IIC1_DUTYCYCLE I2C_DutyCycle_2
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

volatile u32 MIOS32_IIC_unexpected_event;

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
    unsigned ABORT_IF_FIRST_BYTE_0:1;
    unsigned WRITE_WITHOUT_STOP:1;
  };
} transfer_state_t;


typedef struct {
  I2C_TypeDef *base;

  u8 iic_address;
  u8 *tx_buffer_ptr;
  u8 *rx_buffer_ptr;
  volatile u16 buffer_len;
  volatile u16 buffer_ix;

  volatile transfer_state_t transfer_state;
  volatile s32 transfer_error;
  volatile s32 last_transfer_error;

  volatile u8 iic_semaphore;
} iic_rec_t;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static void MIOS32_IIC_InitPeripheral(u8 iic_port);
static void EV_IRQHandler(iic_rec_t *iicx);
static void ER_IRQHandler(iic_rec_t *iicx);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


static iic_rec_t iic_rec[MIOS32_IIC_NUM];


/////////////////////////////////////////////////////////////////////////////
//! Initializes IIC driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Init(u32 mode)
{
  int i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // configure IIC pins in open drain mode
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;

  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SCL_PIN;
  GPIO_Init(MIOS32_IIC0_SCL_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SDA_PIN;
  GPIO_Init(MIOS32_IIC0_SDA_PORT, &GPIO_InitStructure);

#if MIOS32_IIC_NUM >= 2
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC1_SCL_PIN;
  GPIO_Init(MIOS32_IIC1_SCL_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC1_SDA_PIN;
  GPIO_Init(MIOS32_IIC1_SDA_PORT, &GPIO_InitStructure);
#endif

  for(i=0; i<MIOS32_IIC_NUM; ++i) {
    // configure I2C peripheral
    MIOS32_IIC_InitPeripheral(i);

    // now accessible for other tasks
    iic_rec[i].iic_semaphore = 0;
    iic_rec[i].last_transfer_error = 0;
  }

  // configure and enable I2C2 interrupts
  MIOS32_IRQ_Install(I2C2_EV_IRQn, MIOS32_IRQ_IIC_EV_PRIORITY);  
#if MIOS32_IIC_NUM >= 2
  MIOS32_IRQ_Install(I2C1_EV_IRQn, MIOS32_IRQ_IIC_EV_PRIORITY);
#endif

  MIOS32_IRQ_Install(I2C2_ER_IRQn, MIOS32_IRQ_IIC_ER_PRIORITY);
#if MIOS32_IIC_NUM >= 2
  MIOS32_IRQ_Install(I2C1_ER_IRQn, MIOS32_IRQ_IIC_ER_PRIORITY);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// internal function to (re-)initialize the I2C peripheral
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_IIC_InitPeripheral(u8 iic_port)
{
  I2C_InitTypeDef  I2C_InitStructure;
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record

  // prepare I2C init-struct
  I2C_StructInit(&I2C_InitStructure);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_OwnAddress1 = 0;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

  switch( iic_port ) {
    case 0:
      // define base address
      iicx->base = I2C2;

      // enable peripheral clock of I2C
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

      // set I2C clock bus clock params
      // note that the STM32 driver handles value <= 100kHz differently! (duty cycle always 1:1)
      // important: bus frequencies > 400kHz don't work stable
      I2C_InitStructure.I2C_DutyCycle = MIOS32_IIC0_DUTYCYCLE;
      I2C_InitStructure.I2C_ClockSpeed = MIOS32_IIC0_BUS_FREQUENCY;

      break;

#if MIOS32_IIC_NUM >= 2
    case 1:
      // define base address
      iicx->base = I2C1;

      // enable peripheral clock of I2C
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
      
      // set I2C clock bus clock params
      // note that the STM32 driver handles value <= 100kHz differently! (duty cycle always 1:1)
      // important: bus frequencies > 400kHz don't work stable
      I2C_InitStructure.I2C_DutyCycle = MIOS32_IIC1_DUTYCYCLE;
      I2C_InitStructure.I2C_ClockSpeed = MIOS32_IIC1_BUS_FREQUENCY; 

      break;
#endif

    default:
      return;
  }

  // trigger software reset via I2C_DeInit
  I2C_DeInit(iicx->base);

  // clear transfer state and error value
  iicx->transfer_state.ALL = 0;
  iicx->transfer_error = 0;


  // configure I2C peripheral
  I2C_Init(iicx->base, &I2C_InitStructure);
}


/////////////////////////////////////////////////////////////////////////////
//! Semaphore handling: requests the IIC interface
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \param[in] semaphore_type is either IIC_Blocking or IIC_Non_Blocking
//! \return Non_Blocking: returns -1 to request a retry
//! \return 0 if IIC interface free
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferBegin(u8 iic_port, mios32_iic_semaphore_t semaphore_type)
{
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record
  s32 status = -1;

  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  do {
    MIOS32_IRQ_Disable();
    if( !iicx->iic_semaphore ) {
      iicx->iic_semaphore = 1;
      status = 0;
    }
    MIOS32_IRQ_Enable();
  } while( semaphore_type == IIC_Blocking && status != 0 );

  // clear transfer errors of last transmission
  iicx->last_transfer_error = 0;
  iicx->transfer_error = 0;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Semaphore handling: releases the IIC interface for other tasks
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferFinished(u8 iic_port)
{
  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  iic_rec[iic_port].iic_semaphore = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the last transfer error<BR>
//! Will be updated by MIOS32_IIC_TransferCheck(), so that the error status
//! doesn't get lost (the check function will return 0 when called again)<BR>
//! Will be cleared when a new transfer has been started successfully
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return last error status
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_LastErrorGet(u8 iic_port)
{
  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  return iic_rec[iic_port].last_transfer_error;
}


/////////////////////////////////////////////////////////////////////////////
//! Checks if transfer is finished
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return 0 if no ongoing transfer
//! \return 1 if ongoing transfer
//! \return < 0 if error during transfer
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferCheck(u8 iic_port)
{
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record

  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  // ongoing transfer?
  if( iicx->transfer_state.BUSY )
    return 1;

  // error during transfer?
  // (must be done *after* BUSY check to avoid race conditon!)
  if( iicx->transfer_error ) {
    // store error status for MIOS32_IIC_LastErrorGet() function
    iicx->last_transfer_error = iicx->transfer_error;
    // clear current error status
    iicx->transfer_error = 0;
    // release semaphore for easier programming at user level
    iicx->iic_semaphore = 0;
    // and exit
    return iicx->last_transfer_error;
  }

  // no transfer
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Waits until transfer is finished
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \return 0 if no ongoing transfer
//! \return < 0 if error during transfer
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_TransferWait(u8 iic_port)
{
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record
  u32 repeat_ctr = MIOS32_IIC_TIMEOUT_VALUE;
  u16 last_buffer_ix = iicx->buffer_ix;

  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  while( --repeat_ctr > 0 ) {
    // check if buffer index has changed - if so, reload repeat counter
    if( iicx->buffer_ix != last_buffer_ix ) {
      repeat_ctr = MIOS32_IIC_TIMEOUT_VALUE;
      last_buffer_ix = iicx->buffer_ix;
    }

    // get transfer state
    s32 check_state = MIOS32_IIC_TransferCheck(iic_port);

    // exit if transfer finished or error detected
    if( check_state <= 0 ) {
      if( check_state < 0 )
	iicx->iic_semaphore = 0; // release semaphore for easier programming at user level
      return check_state;
    }
  }

  // timeout error - something is stalling...

#if 0
  // send stop condition
  I2C_GenerateSTOP(iicx->base, ENABLE);
#else
  // added by wackazong
  // see also http://midibox.org/forums/topic/15770-sda-stuck-low-on-i2c-transfer/
  // try to deblock a SDA line that is stuck low by bit-banging out a clock signal
  // and waiting until the send buffer of any slave is empty

  // disable interrupts
  I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

  I2C_Cmd(I2C2, DISABLE);
  GPIO_InitTypeDef GPIO_InitStructure;

  // reconfigure IIC pins to push pull
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SCL_PIN;
  GPIO_Init(MIOS32_IIC0_SCL_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SDA_PIN;
  GPIO_Init(MIOS32_IIC0_SDA_PORT, &GPIO_InitStructure);

  u32 i;
  for (i=0; i<16; i++) {
    /*Reset the SDA Pin*/
    GPIO_ResetBits(GPIOB, MIOS32_IIC0_SDA_PIN);
    MIOS32_DELAY_Wait_uS(1000);
    /*Reset the SCL Pin*/
    GPIO_ResetBits(GPIOB, MIOS32_IIC0_SCL_PIN);
    MIOS32_DELAY_Wait_uS(1000);
    /*Set the SCL Pin*/
    GPIO_SetBits(GPIOB, MIOS32_IIC0_SCL_PIN);
    MIOS32_DELAY_Wait_uS(1000);
    /*Set the SDA Pin*/
    GPIO_SetBits(GPIOB, MIOS32_IIC0_SDA_PIN);
    MIOS32_DELAY_Wait_uS(1000);
  }

  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;

  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SCL_PIN;
  GPIO_Init(MIOS32_IIC0_SCL_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_IIC0_SDA_PIN;
  GPIO_Init(MIOS32_IIC0_SDA_PORT, &GPIO_InitStructure);

  I2C_Cmd(I2C2, ENABLE); 
#endif

  // re-initialize peripheral
  MIOS32_IIC_InitPeripheral(iic_port);

  // release semaphore (!)
  iicx->iic_semaphore = 0;

  return (iicx->last_transfer_error=MIOS32_IIC_ERROR_TIMEOUT);
}


/////////////////////////////////////////////////////////////////////////////
//! Starts a new transfer. If this function is called during an ongoing
//! transfer, we wait until it has been finished and setup the new transfer
//! \param[in] transfer type:<BR>
//! <UL>
//!   <LI>IIC_Read: a common Read transfer
//!   <LI>IIC_Write: a common Write transfer
//!   <LI>IIC_Read_AbortIfFirstByteIs0: used to poll MBHP_IIC_MIDI: aborts transfer
//!         if the first received byte is 0
//!   <LI>IIC_Write_WithoutStop: don't send stop condition after transfer to allow
//!         a restart condition (e.g. used to access EEPROMs)
//! \param[in] iic_port the IIC port (0..MIOS32_IIC_NUM-1)
//! \param[in] address of IIC device (bit 0 always cleared)
//! \param[in] *buffer pointer to transmit/receive buffer
//! \param[in] len number of bytes which should be transmitted/received
//! \return 0 no error
//! \return < 0 on errors, if MIOS32_IIC_ERROR_PREV_OFFSET is added, the previous
//!      transfer got an error (the previous task didn't use \ref MIOS32_IIC_TransferWait
//!      to poll the transfer state)
//! \note Note that the semaphore will be released automatically after an error
//! (MIOS32_IIC_TransferBegin() has to be called again)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_Transfer(u8 iic_port, mios32_iic_transfer_t transfer, u8 address, u8 *buffer, u16 len)
{
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record
  s32 error;

  if( iic_port >= MIOS32_IIC_NUM )
    return MIOS32_IIC_ERROR_INVALID_PORT;

  // wait until previous transfer finished
  if( (error = MIOS32_IIC_TransferWait(iic_port)) ) { // transmission error during previous transfer
    iicx->iic_semaphore = 0; // release semaphore for easier programming at user level
    return error + MIOS32_IIC_ERROR_PREV_OFFSET;
  }

  // disable interrupts
  I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

  // clear transfer state and error value
  iicx->transfer_state.ALL = 0;
  iicx->transfer_error = 0;

  // set buffer length and start index
  iicx->buffer_len = len;
  iicx->buffer_ix = 0;

  // branch depending on read/write
  if( transfer == IIC_Read || transfer == IIC_Read_AbortIfFirstByteIs0 ) {
    // take new address/buffer/len
    iicx->iic_address = address | 1; // set bit 0 for read operation
    iicx->tx_buffer_ptr = NULL; // ensure that previous TX buffer won't be accessed
    iicx->rx_buffer_ptr = buffer;
    // special option for optimized MBHP_IIC_MIDI
    iicx->transfer_state.ABORT_IF_FIRST_BYTE_0 = transfer == IIC_Read_AbortIfFirstByteIs0 ? 1 : 0;
  } else if( transfer == IIC_Write || transfer == IIC_Write_WithoutStop ) {
    // take new address/buffer/len
    iicx->iic_address = address & 0xfe; // clear bit 0 for write operation
    iicx->tx_buffer_ptr = buffer;
    iicx->rx_buffer_ptr = NULL; // ensure that nothing will be received
    // option to skip stop-condition generation after successful write
    iicx->transfer_state.WRITE_WITHOUT_STOP = transfer == IIC_Write_WithoutStop ? 1 : 0;
  } else {
    iicx->iic_semaphore = 0; // release semaphore for easier programming at user level
    return (iicx->last_transfer_error=MIOS32_IIC_ERROR_UNSUPPORTED_TRANSFER_TYPE);
  }

  // start with ACK
  I2C_AcknowledgeConfig(iicx->base, ENABLE);

  // clear last error status
  iicx->last_transfer_error = 0;

  // notify that transfer has started
  iicx->transfer_state.BUSY = 1;

  // send start condition
  I2C_GenerateSTART(iicx->base, ENABLE);

  // enable I2V2 event, buffer and error interrupt
  // this must be done *after* GenerateStart, for the case last transfer was WRITE_WITHOUT_STOP.
  // in this case, start was already generated at the end of the last communication!
  I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Internal function for handling IIC event interrupts
/////////////////////////////////////////////////////////////////////////////
static void EV_IRQHandler(iic_rec_t *iicx)
{
  u8 b;

  // Read SR1 and SR2 at the beginning (if not done so, flags may get lost)
  u32 event = I2C_GetLastEvent(iicx->base);

  if( event == 0x00030000 ) // TK: required for newer gcc versions, e.g. 4.2.1
    return; // don't know why, but it helps...

  // The order of the handling blocks is chosen by test results @ 1MHZ
  // don't change this order

  // RxNE set, will be cleared by reading/writing DR
  // note: also BTF will be reset after a read of SR1 (TxE flag) followed by either read/write DR
  // or a START or STOP condition generated
  // failsave: really requested a receive transfer? If not, continue to check TXE flag, if not set,
  // we'll end up in the unexpected event handler.
  if( event & I2C_FLAG_RXNE && iicx->rx_buffer_ptr != NULL ){
    // get received data
    b = I2C_ReceiveData(iicx->base);

    // failsave: still place in buffer?
    if( iicx->buffer_ix < iicx->buffer_len )
      iicx->rx_buffer_ptr[iicx->buffer_ix++] = b;

    // last byte received, disable interrupts and return.
    if( iicx->transfer_state.STOP_REQUESTED ) {
      // transfer finished
      iicx->transfer_state.BUSY = 0;
      // disable all interrupts
      I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      return;
    }
 
    // request NAK and stop condition before receiving last data
    if( (iicx->buffer_ix >= iicx->buffer_len-1) || (iicx->transfer_state.ABORT_IF_FIRST_BYTE_0 && iicx->buffer_ix == 1 && b == 0x00) ) {
      // request NAK
      I2C_AcknowledgeConfig(iicx->base, DISABLE);
      // request stop condition
      I2C_GenerateSTOP(iicx->base, ENABLE);
      iicx->transfer_state.STOP_REQUESTED = 1;
    }
    return;
  }

  // ADDR set, TRA flag not set (indicates transmitter/receiver mode).
  // ADDR will be cleared by a read of SR1 followed by a read of SR2 (done by I2C_GetLastEvent)
  // If transmitter mode is selected (TRA set), we go on, TXE will be catched to send the first byte
  if( (event & I2C_FLAG_ADDR) && !(event & I2C_FLAG_TRA) ){
    // address sent (receiver mode), receiving first byte - check if we already have to request NAK/Stop
    if( iicx->buffer_len == 1 ) {
      // request NAK
      I2C_AcknowledgeConfig(iicx->base, DISABLE);
      // request stop condition
      I2C_GenerateSTOP(iicx->base, ENABLE);
      iicx->transfer_state.STOP_REQUESTED = 1;
    }
    return;
  }

  // TxE set, will be cleared by writing DR, or after START or STOP was generated
  // This handling also applies for BTF, as TXE will alway be set if BTF is.
  // note: also BTF will be reset after a read of SR1 (TxE flag) followed by either read/write DR
  // or a START or STOP condition generated
  if( event & I2C_FLAG_TXE ){

    // last byte already sent, disable interrupts and return.
    if( iicx->transfer_state.STOP_REQUESTED ) {
      // transfer finished
      iicx->transfer_state.BUSY = 0;
      // disable all interrupts
      I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      return;
    }

    if( iicx->buffer_ix < iicx->buffer_len ) {
      // checking tx_buffer_ptr for NULL is a failsafe measure.
      I2C_SendData(iicx->base, (iicx->tx_buffer_ptr == NULL) ? 0 : iicx->tx_buffer_ptr[iicx->buffer_ix++]);
      return;
    } 

    // peripheral is transfering last byte, request stop condition /
    // on write-without-stop transfer-type, request start condition instead
    if( !iicx->transfer_state.WRITE_WITHOUT_STOP )
      I2C_GenerateSTOP(iicx->base, ENABLE);
    else
      I2C_GenerateSTART(iicx->base, ENABLE);
    iicx->transfer_state.STOP_REQUESTED = 1;

    if( iicx->buffer_len == 0 ) {
      // transfer finished
      iicx->transfer_state.BUSY = 0;
      // disable all interrupts
      I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
    } else {
      // Disable the I2C_IT_BUF interrupt after sending the last buffer data 
      // (last EV8) to not allow a new interrupt just with TxE - only BTF will generate it
      // if this is not done, BUSY will be cleared before the transfer is finished
      I2C_ITConfig(iicx->base, I2C_IT_BUF, DISABLE);
    }
  return;
  }

  // SB set, cleared by reading SR1 (done by I2C_GetLastEvent) followed by writing DR register
  if( event & I2C_FLAG_SB ){
    // don't send address if stop was requested (WRITE_WITHOUT_STOP - mode, start condition was sent)
    // we have to wait for the application to start the next transfer
    if( iicx->transfer_state.STOP_REQUESTED ) {
      // transfer finished
      iicx->transfer_state.BUSY = 0;
      // disable all interrupts
      I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
      return;
    } 
    // send IIC address
    I2C_Send7bitAddress(iicx->base, iicx->iic_address, 
      (iicx->iic_address & 1)
	  ? I2C_Direction_Receiver
	  : I2C_Direction_Transmitter);
    return;
  }

  // this code is only reached if something got wrong, e.g. interrupt handler is called too late,
  // the device reset itself (while testing, it was always event 0x00000000). we have to stop the transfer,
  // else read/write of corrupt data may be the result.
  I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
  // notify error
  MIOS32_IIC_unexpected_event = event;
  iicx->transfer_error = MIOS32_IIC_ERROR_UNEXPECTED_EVENT;
  iicx->transfer_state.BUSY = 0;
  // do dummy read to send NAK + STOP condition
  I2C_AcknowledgeConfig(iicx->base, DISABLE);
  b = I2C_ReceiveData(iicx->base);
  I2C_GenerateSTOP(iicx->base, ENABLE);
}



/////////////////////////////////////////////////////////////////////////////
// Internal function for handling IIC error interrupts
/////////////////////////////////////////////////////////////////////////////
static void ER_IRQHandler(iic_rec_t *iicx)
{
  // Read SR1 and SR2 at the beginning (if not done so, flags may get lost)
  u32 event = I2C_GetLastEvent(iicx->base);

  // note that only one error number is available
  // the order of these checks defines the priority

  // bus error (start/stop condition during read
  // unlikely, should only be relevant for slave mode?)
  if( event & I2C_FLAG_BERR  ) {
    I2C_ClearITPendingBit(iicx->base, I2C_IT_BERR);
    iicx->transfer_error = MIOS32_IIC_ERROR_BUS;
  }

  // arbitration lost
  if( event & I2C_FLAG_ARLO ) {
    I2C_ClearITPendingBit(iicx->base, I2C_IT_ARLO);
    iicx->transfer_error = MIOS32_IIC_ERROR_ARBITRATION_LOST;
  }

  // no acknowledge received from slave (e.g. slave not connected)
  if( event & I2C_FLAG_AF ) {
    I2C_ClearITPendingBit(iicx->base, I2C_IT_AF);
    iicx->transfer_error = MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED;
    // send stop condition to release bus
    I2C_GenerateSTOP(iicx->base, ENABLE);
  }

  // disable interrupts
  I2C_ITConfig(iicx->base, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);

  // notify that transfer has finished (due to the error)
  iicx->transfer_state.BUSY = 0;
}


/////////////////////////////////////////////////////////////////////////////
// interrupt vectors
/////////////////////////////////////////////////////////////////////////////

void I2C2_EV_IRQHandler(void)
{
  EV_IRQHandler((iic_rec_t *)&iic_rec[0]);
}

void I2C2_ER_IRQHandler(void)
{
  ER_IRQHandler((iic_rec_t *)&iic_rec[0]);
}


#if MIOS32_IIC_NUM >= 2
void I2C1_EV_IRQHandler(void)
{
  EV_IRQHandler((iic_rec_t *)&iic_rec[1]);
}

void I2C1_ER_IRQHandler(void)
{
  ER_IRQHandler((iic_rec_t *)&iic_rec[1]);
}
#endif


//! \}

#endif /* MIOS32_DONT_USE_IIC */
