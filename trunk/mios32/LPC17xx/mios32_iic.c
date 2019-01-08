// $Id$
//! \defgroup MIOS32_IIC
//!
//! IIC driver for MIOS32
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
#if !defined(MIOS32_DONT_USE_IIC)


// I2C peripherals are clocked at CCLK/4
#define I2C_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)


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
    unsigned ABORT_IF_FIRST_BYTE_0:1;
    unsigned WRITE_WITHOUT_STOP:1;
  };
} transfer_state_t;


typedef struct {
  LPC_I2C_TypeDef *base;

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

  for(i=0; i<MIOS32_IIC_NUM; ++i) {
    // configure I2C peripheral
    MIOS32_IIC_InitPeripheral(i);

    // now accessible for other tasks
    iic_rec[i].iic_semaphore = 0;
    iic_rec[i].last_transfer_error = 0;
  }


  /* Install interrupt handlers */
  MIOS32_IRQ_Install(I2C0_IRQn, MIOS32_IRQ_IIC_EV_PRIORITY);
#if MIOS32_IIC_NUM >= 2
  MIOS32_IRQ_Install(I2C1_IRQn, MIOS32_IRQ_IIC_EV_PRIORITY);
#endif
#if MIOS32_IIC_NUM >= 3
  MIOS32_IRQ_Install(I2C2_IRQn, MIOS32_IRQ_IIC_EV_PRIORITY);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// internal function to (re-)initialize the I2C peripheral
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_IIC_InitPeripheral(u8 iic_port)
{
  iic_rec_t *iicx = &iic_rec[iic_port];// simplify addressing of record
  u32 frq_clocks;

  switch( iic_port ) {
    case 0:
      // define base address
      iicx->base = LPC_I2C0;

      // frequency
      frq_clocks = I2C_PERIPHERAL_FRQ / (MIOS32_IIC0_BUS_FREQUENCY);

      // init pins
      MIOS32_SYS_LPC_PINSEL (0, 27, 1); // SDA: P0.27 (function 1)
      MIOS32_SYS_LPC_PINSEL (0, 28, 1); // SCK: P0.28 (function 1)
      break;

#if MIOS32_IIC_NUM >= 2
    case 1:
      // define base address
      iicx->base = LPC_I2C1;

      // frequency
      frq_clocks = I2C_PERIPHERAL_FRQ / (MIOS32_IIC1_BUS_FREQUENCY);

      // init pins
      MIOS32_SYS_LPC_PINSEL(0, 19, 3); // SDA: P0.19 (function 3)
      MIOS32_SYS_LPC_PINSEL(0, 20, 3); // SCK: P0.20 (function 3)
      break;
#endif

#if MIOS32_IIC_NUM >= 3
    case 2:
      // define base address
      iicx->base = LPC_I2C2;

      // frequency
      frq_clocks = I2C_PERIPHERAL_FRQ / (MIOS32_IIC2_BUS_FREQUENCY);

      // init pins
      MIOS32_SYS_LPC_PINSEL(0, 10, 2); // SDA: P0.10 (function 2)
      MIOS32_SYS_LPC_PINSEL(0, 11, 2); // SCK: P0.11 (function 2)
      break;
#endif

    default:
      return;
  }

  // clear flags
  iicx->base->I2CONCLR = 0x6c; // I2EN, STA, SIC, AAC

  // init clock
  u32 clk_l = frq_clocks / 2;
  u32 clk_h = frq_clocks-clk_l; // consider rounding error...
  iicx->base->I2SCLL = clk_l;
  iicx->base->I2SCLH = clk_h;

  // clear transfer state and error value
  iicx->transfer_state.ALL = 0;
  iicx->transfer_error = 0;

  // enable peripheral
  iicx->base->I2CONSET = 0x40; // I2EN
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
  MIOS32_IRQ_Disable(); // TODO: disable peripheral

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
    MIOS32_IRQ_Enable(); // TODO: enable peripheral??
    return (iicx->last_transfer_error=MIOS32_IIC_ERROR_UNSUPPORTED_TRANSFER_TYPE);
  }

  // clear last error status
  iicx->last_transfer_error = 0;

  // notify that transfer has started
  iicx->transfer_state.BUSY = 1;

  MIOS32_IRQ_Enable(); // TODO: enable peripheral

  // send start condition
  iicx->base->I2CONCLR = 0x04; // clear AA (assert acknowledge) bit
  iicx->base->I2CONSET = 0x20; // set STA (start) bit

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// interrupt service routine for all IIC peripherals
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_IIC_IRQ_Handler(iic_rec_t *iicx)
{
  u8 b;
  u32 i2stat = iicx->base->I2STAT;

  // see also chapter 19.10.5 of UM10360.pdf
  // main modifications of the proposed sequence:
  // - STA flag always cleared after START condition has been transmitted (or error has been received) to prevent endless loop
  // - AA flag *not* always set as implied in chapter 19.9.1 ("if AA is reset, the I2C block will not acknowledge its own slave address)

#if 0
  MIOS32_MIDI_SendDebugMessage("%02x\n", i2stat);
#endif

  switch( i2stat ) {
  case 0x00: // Bus Error
    // error
    iicx->transfer_error = MIOS32_IIC_ERROR_BUS;
    iicx->transfer_state.BUSY = 0;
    // transmit STOP condition
    iicx->base->I2CONCLR = 0x20; // clear the STA (start) bit
    iicx->base->I2CONSET = 0x10; // set the STO (stop) bit
    break;

  case 0x08: // a START condition has been transmitted. The Slave Address + R/W bit will now be transmitted
  case 0x10: // A repeated START condition has been transmitted. The Slave address + R/W bit will now be transmitted
    iicx->base->I2CONCLR = 0x20; // clear the STA (start) bit
    // send address
    iicx->base->I2DAT = iicx->iic_address;
    // ensure that data starts at index 0
    iicx->buffer_ix = 0;
    break;

  case 0x38: // Arbitration lost during Slave Address + Write or data
    // error
    iicx->transfer_error = MIOS32_IIC_ERROR_ARBITRATION_LOST;
    iicx->transfer_state.BUSY = 0;
    // transmit STOP condition
    iicx->base->I2CONCLR = 0x20; // clear the STA (start) bit
    iicx->base->I2CONSET = 0x10; // set the STO (stop) bit
    break;

  // TRANSMIT STATES ACK
  case 0x18: // previous state was 0x08 or 0x10, Slave Address + Write has been transmitted, ACK has been received - transmit first data
  case 0x28: // data has been transmitted, ACK has been received - transmit next data

    // last byte sent?
    if( (iicx->buffer_ix) >= iicx->buffer_len ) {
      if( iicx->transfer_state.WRITE_WITHOUT_STOP ) {
	//iicx->base->I2CONSET = 0x30; // set the STA (start) and STO (stop) bit for a restart
	iicx->base->I2CONSET = 0x10; // set the STO (stop) bit -- TODO: try to find a solution which omits the stop 
      } else
	iicx->base->I2CONSET = 0x10; // set the STO (stop) bit

      // transfer finished
      iicx->transfer_state.BUSY = 0;
    } else {
      // checking tx_buffer_ptr for NULL is a failsafe measure.
      iicx->base->I2DAT = (iicx->tx_buffer_ptr == NULL) ? 0 : iicx->tx_buffer_ptr[iicx->buffer_ix++];
    }
    break;

  // TRANSMIT STATES NAK
  case 0x20: // previous state was 0x08 or 0x10, Slave Address + Write has been transmitted, NOT ACK has been received
  case 0x30: // data has been transmitted, NOT ACK has been received
    // error
    iicx->transfer_error = MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED;
    iicx->transfer_state.BUSY = 0;

    // transmit STOP condition
    iicx->base->I2CONCLR = 0x20; // clear the STA (start) bit
    iicx->base->I2CONSET = 0x10; // set the STO (stop) bit
    break;

  // RECEIVE STATES ACK
  case 0x40: // Previous state was 0x08 or 0x10. Slave address + Read has been transmitted, ACK has been received
    iicx->base->I2CONSET = 0x04; // set the AA (assert acknowledge) bit
    break;

  case 0x50: // Data has been received, ACK has been returned
  case 0x58: // Data has been received, NOT ACK has been returned (last data)
    b = iicx->base->I2DAT;

    // failsave: still place in buffer?
    if( iicx->rx_buffer_ptr && iicx->buffer_ix < iicx->buffer_len )
      iicx->rx_buffer_ptr[iicx->buffer_ix++] = b;

    if( i2stat == 0x58 ) {
      // request STOP
      iicx->base->I2CONSET = 0x10; // set the STO (stop) bit
      // transfer finished
      iicx->transfer_state.BUSY = 0;
    } else {
      // last byte received
      // request NAK and stop condition before receiving last data
      if( (iicx->buffer_ix >= iicx->buffer_len) ||
	  (iicx->transfer_state.ABORT_IF_FIRST_BYTE_0 && iicx->buffer_ix == 1 && b == 0x00) ) {

	// request NAK
	iicx->base->I2CONCLR = 0x04; // clear the AA (assert acknowledge) bit
      } else {
	iicx->base->I2CONSET = 0x04; // set the AA (assert acknowledge) bit
      }
    }
    break;

  // RECEIVE STATES NOT ACK
  case 0x48: // Slave address + Read has been transmitted, NOT ACK has been received
    // error
    iicx->transfer_error = MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED;
    iicx->transfer_state.BUSY = 0;
    iicx->base->I2CONSET = 0x10; // set the STO (stop) bit
    break;

  default:
    // error
    iicx->transfer_error = MIOS32_IIC_ERROR_UNEXPECTED_EVENT;
    MIOS32_IIC_unexpected_event = i2stat;
    iicx->base->I2CONCLR = 0x20; // clear START bit
    iicx->transfer_state.BUSY = 0;
    break;
  }

  // clear the SI (interrupt) flag
  iicx->base->I2CONCLR = 0x08;
}


/////////////////////////////////////////////////////////////////////////////
// interrupt vectors
/////////////////////////////////////////////////////////////////////////////

void I2C0_IRQHandler(void)  
{
  MIOS32_IIC_IRQ_Handler((iic_rec_t *)&iic_rec[0]);
}

#if MIOS32_IIC_NUM >= 2
void I2C1_IRQHandler(void)  
{
  MIOS32_IIC_IRQ_Handler((iic_rec_t *)&iic_rec[1]);
}
#endif

#if MIOS32_IIC_NUM >= 3
void I2C2_IRQHandler(void)  
{
  MIOS32_IIC_IRQ_Handler((iic_rec_t *)&iic_rec[2]);
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_IIC */
