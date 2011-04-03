// $Id$
//! \defgroup MIOS32_SPI
//!
//! Hardware Abstraction Layer for SPI ports of LPC17xx
//!
//! Not adapted to LPC17xx yet
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
#if !defined(MIOS32_DONT_USE_SPI)


/////////////////////////////////////////////////////////////////////////////
// SPI Pin definitions
// (not part of mios32_spi.h file, since overruling would lead to a hardware
// dependency in MIOS32 applications)
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// MIOS32_SPI1 uses SSP1
// Assigned to J8/9 of MBHP_CORE_LPC17
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_SPI1_PTR        LPC_SSP1

// RCLK1: P0.6 (used as GPIO)
#define MIOS32_SPI1_RCLK1_INIT { LPC_PINCON->PINSEL0 &= ~(3 << (6*2)); LPC_GPIO0->FIODIR |= (1 << 6); }
#define MIOS32_SPI1_RCLK1_SET(v) { if( v ) LPC_GPIO0->FIOSET = (1 << 6); else LPC_GPIO0->FIOCLR = (1 << 6); }

// RCLK2: P0.5 (used as GPIO)
#define MIOS32_SPI1_RCLK2_INIT { LPC_PINCON->PINSEL0 &= ~(3 << (5*2)); LPC_GPIO0->FIODIR |= (1 << 5); }
#define MIOS32_SPI1_RCLK2_SET(v) { if( v ) LPC_GPIO0->FIOSET = (1 << 5); else LPC_GPIO0->FIOCLR = (1 << 5); }

// SCLK: P0.7 (assigned to SSP1)
#define MIOS32_SPI1_SCLK_INIT  { LPC_PINCON->PINSEL0 &= ~(3 << (7*2)); LPC_PINCON->PINSEL0 |= (2 << (7*2)); }
// MISO: P0.8 (assigned to SSP1)
#define MIOS32_SPI1_MISO_INIT  { LPC_PINCON->PINSEL0 &= ~(3 << (8*2)); LPC_PINCON->PINSEL0 |= (2 << (8*2)); }
// MISO: P0.9 (assigned to SSP1)
#define MIOS32_SPI1_MOSI_INIT  { LPC_PINCON->PINSEL0 &= ~(3 << (9*2)); LPC_PINCON->PINSEL0 |= (2 << (9*2)); }

// Push-Pull Config
#define MIOS32_SPI1_PP_INIT    { LPC_PINCON->PINMODE_OD0 &= ~((1 << 9) | (1 << 8) | (1 << 7) | (1 << 6) | (1 << 5)); }
// Open-Drain Config
#define MIOS32_SPI1_OD_INIT    { LPC_PINCON->PINMODE_OD0 |=  ((1 << 9) | (1 << 8) | (1 << 7) | (1 << 6) | (1 << 5)); }

// DMA Request Inputs (DMACSREQ)
#define MIOS32_SPI1_DMA_TX_REQ  2
#define MIOS32_SPI1_DMA_RX_REQ  3

// Allocated DMA Channels - they are matching with the request input number, but this is no must!
#define MIOS32_SPI1_DMA_TX_CHN  2
#define MIOS32_SPI1_DMA_RX_CHN  3


/////////////////////////////////////////////////////////////////////////////
// MIOS32_SPI2 uses LPC_SPI (disadvantage: no DMA support available)
// Assigned to J19 of MBHP_CORE_LPC17
// To be checked: according to datasheet, SSP0 and SPI can't be used in
// parallel - this has to be proven, because SSP0 provides an alternative pinning (is the IO collision meant?)
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_SPI2_PTR        LPC_SPI

// RCLK1: P0.16 (used as GPIO)
#define MIOS32_SPI2_RCLK1_INIT { LPC_PINCON->PINSEL1 &= ~(3 << ((16-16)*2)); LPC_GPIO0->FIODIR |= (1 << 16); }
#define MIOS32_SPI2_RCLK1_SET(v) { if( v ) LPC_GPIO0->FIOSET = (1 << 16); else LPC_GPIO0->FIOCLR = (1 << 16); }

// RCLK2: P0.21 (used as GPIO)
#define MIOS32_SPI2_RCLK2_INIT { LPC_PINCON->PINSEL1 &= ~(3 << ((21-16)*2)); LPC_GPIO0->FIODIR |= (1 << 21); }
#define MIOS32_SPI2_RCLK2_SET(v) { if( v ) LPC_GPIO0->FIOSET = (1 << 21); else LPC_GPIO0->FIOCLR = (1 << 21); }

// SCLK: P0.15 (assigned to SPI)
#define MIOS32_SPI2_SCLK_INIT  { LPC_PINCON->PINSEL0 |= (3 << (15*2)); }
// MISO: P0.17 (assigned to SPI)
#define MIOS32_SPI2_MISO_INIT  { LPC_PINCON->PINSEL1 |= (3 << ((17-16)*2)); }
// MISO: P0.18 (assigned to SPI)
#define MIOS32_SPI2_MOSI_INIT  { LPC_PINCON->PINSEL1 |= (3 << ((18-16)*2)); }

// Push-Pull Config
#define MIOS32_SPI2_PP_INIT    { LPC_PINCON->PINMODE_OD0 &= ~((1 << 21) | (1 << 18) | (1 << 17) | (1 << 16) | (1 << 15)); }
// Open-Drain Config
#define MIOS32_SPI2_OD_INIT    { LPC_PINCON->PINMODE_OD0 |=  ((1 << 21) | (1 << 18) | (1 << 17) | (1 << 16) | (1 << 15)); }

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Initializes SPI pins
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  ///////////////////////////////////////////////////////////////////////////
  // SPI1
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI1
  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(1, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(1, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(1, MIOS32_SPI_PIN_DRIVER_WEAK);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(1, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);
#endif /* MIOS32_DONT_USE_SPI2 */


  ///////////////////////////////////////////////////////////////////////////
  // SPI2
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI2
  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(2, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(2, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(2, MIOS32_SPI_PIN_DRIVER_WEAK);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(2, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);
#endif /* MIOS32_DONT_USE_SPI2 */


  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! (Re-)initializes SPI IO Pins
//! By default, all output pins are configured with weak open drain drivers for 2 MHz
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] spi_pin_driver configures the driver strength:
//! <UL>
//!   <LI>MIOS32_SPI_PIN_DRIVER_STRONG: configures outputs for up to 50 MHz
//!   <LI>MIOS32_SPI_PIN_DRIVER_STRONG_OD: configures outputs as open drain 
//!       for up to 50 MHz (allows voltage shifting via pull-resistors)
//!   <LI>MIOS32_SPI_PIN_DRIVER_WEAK: configures outputs for up to 2 MHz (better EMC)
//!   <LI>MIOS32_SPI_PIN_DRIVER_WEAK_OD: configures outputs as open drain for 
//!       up to 2 MHz (allows voltage shifting via pull-resistors)
//! </UL>
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported pin driver mode
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver)
{
  // tmp
  spi_pin_driver = MIOS32_SPI_PIN_DRIVER_STRONG;

  switch( spi ) {
    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      if( spi_pin_driver == MIOS32_SPI_PIN_DRIVER_STRONG || MIOS32_SPI_PIN_DRIVER_WEAK ) {
	MIOS32_SPI1_PP_INIT;
      } else if( spi_pin_driver == MIOS32_SPI_PIN_DRIVER_STRONG_OD || MIOS32_SPI_PIN_DRIVER_WEAK_OD ) {
	MIOS32_SPI1_OD_INIT;
      } else {
	return -3; // unsupported pin driver mode
      }

      MIOS32_SPI1_RCLK1_INIT;
      MIOS32_SPI1_RCLK2_INIT;
      MIOS32_SPI1_SCLK_INIT;
      MIOS32_SPI1_MISO_INIT;
      MIOS32_SPI1_MOSI_INIT;
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      if( spi_pin_driver == MIOS32_SPI_PIN_DRIVER_STRONG || MIOS32_SPI_PIN_DRIVER_WEAK ) {
	MIOS32_SPI2_PP_INIT;
      } else if( spi_pin_driver == MIOS32_SPI_PIN_DRIVER_STRONG_OD || MIOS32_SPI_PIN_DRIVER_WEAK_OD ) {
	MIOS32_SPI2_OD_INIT;
      } else {
	return -3; // unsupported pin driver mode
      }

      MIOS32_SPI2_RCLK1_INIT;
      MIOS32_SPI2_RCLK2_INIT;
      MIOS32_SPI2_SCLK_INIT;
      MIOS32_SPI2_MISO_INIT;
      MIOS32_SPI2_MOSI_INIT;
      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! (Re-)initializes SPI peripheral transfer mode
//! By default, all SPI peripherals are configured with 
//! MIOS32_SPI_MODE_CLK1_PHASE1 and MIOS32_SPI_PRESCALER_128
//!
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] spi_mode configures clock and capture phase:
//! <UL>
//!   <LI>MIOS32_SPI_MODE_CLK0_PHASE0: Idle level of clock is 0, data captured at rising edge
//!   <LI>MIOS32_SPI_MODE_CLK0_PHASE1: Idle level of clock is 0, data captured at falling edge
//!   <LI>MIOS32_SPI_MODE_CLK1_PHASE0: Idle level of clock is 1, data captured at falling edge
//!   <LI>MIOS32_SPI_MODE_CLK1_PHASE1: Idle level of clock is 1, data captured at rising edge
//! </UL>
//! \param[in] spi_prescaler configures the SPI speed:
//! <UL>
//!   <LI>MIOS32_SPI_PRESCALER_2: sets clock rate 27.7~ nS @ 72 MHz (36 MBit/s) (only supported for spi==0, spi1 uses 4 instead)
//!   <LI>MIOS32_SPI_PRESCALER_4: sets clock rate 55.5~ nS @ 72 MHz (18 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_8: sets clock rate 111.1~ nS @ 72 MHz (9 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_16: sets clock rate 222.2~ nS @ 72 MHz (4.5 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_32: sets clock rate 444.4~ nS @ 72 MHz (2.25 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_64: sets clock rate 888.8~ nS @ 72 MHz (1.125 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_128: sets clock rate 1.7~ nS @ 72 MHz (0.562 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_256: sets clock rate 3.5~ nS @ 72 MHz (0.281 MBit/s)
//! </UL>
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if invalid spi_prescaler selected
//! \return -4 if invalid spi_mode selected
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler)
{
  if( spi_prescaler >= 8 )
    return -3; // invalid spi_prescaler selected

  if( spi_mode >= 4 )
    return -4; // invalid spi_mode selected

  switch( spi ) {
    case 1: {
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      // LPC17xx provides a finer control over clock than STM32 (7bit resolution)
      // however, the 2^n prescaler should be sufficient...
      u32 prescaler = 1 << (spi_prescaler+1);
      if( prescaler < 2 )
	prescaler = 2; // requirement according to datasheet (almost redundant, the formula above already ensures this)
      else if( prescaler >= 0x100 )
	prescaler = 0xff;
      MIOS32_SPI1_PTR->CPSR = prescaler;

      // note: CPOL/CPHA swapped on CR0 - strange decition from design team to make SSP incompatible to SPI
      u16 prev_mode = ((MIOS32_SPI1_PTR->CR0 >> 7) & 1) | ((MIOS32_SPI1_PTR->CR0 >> (6-1)) & 2);

      // SPI frame format, clock/phase according to spi_mode, 8 bits transfered
      MIOS32_SPI1_PTR->CR0 = ((spi_mode & 1) << 7) | ((spi_mode & 2) << (6-1)) | (7 << 0);
      MIOS32_SPI1_PTR->CR1 = (1 << 1); // enable SSP

      MIOS32_SPI1_PTR->DMACR = 0x3; // enable Rx/Tx DMA request

      if( prev_mode != spi_mode ) {
	// clock configuration has been changed - we should send a dummy byte
	// before the application activates chip select.
	// this solves a dependency between SDCard and ENC28J60 driver
	MIOS32_SPI_TransferByte(spi, 0xff);
      }
#endif
    } break;

    case 2: {
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      // LPC17xx provides a finer control over clock than STM32 (7bit resolution)
      // however, the 2^n prescaler should be sufficient...
      u32 prescaler = (1 << (spi_prescaler+1));
      if( prescaler < 8 )
	prescaler = 8; // requirement according to datasheet
      else if( prescaler >= 0x100 )
	prescaler = 0xff;
      MIOS32_SPI2_PTR->SPCCR = prescaler;

      u16 prev_mode = (MIOS32_SPI2_PTR->SPCR >> 3) & 0x3;

      // clock/phase according to spi_mode, master mode, MSB first
      MIOS32_SPI2_PTR->SPCR = ((spi_mode & 3) << 3) | (1 << 5) | (0 << 6);

      if( prev_mode != spi_mode ) {
	// clock configuration has been changed - we should send a dummy byte
	// before the application activates chip select.
	// this solves a dependency between SDCard and ENC28J60 driver
	MIOS32_SPI_TransferByte(spi, 0xff);
      }
#endif
    } break;

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Controls the RC (Register Clock alias Chip Select) pin of a SPI port
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] rc_pin RCLK pin (0 or 1 for RCLK1 or RCLK2)
//! \param[in] pin_value 0 or 1
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported RCx pin selected
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value)
{
  switch( spi ) {

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      switch( rc_pin ) {
        case 0:
	  MIOS32_SPI1_RCLK1_SET(pin_value);
	  break;

        case 1:
	  MIOS32_SPI1_RCLK2_SET(pin_value);
	  break;

        default:
	  return -3; // unsupported RC pin
      }
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      switch( rc_pin ) {
        case 0:
	  MIOS32_SPI2_RCLK1_SET(pin_value);
	  break;

        case 1:
	  MIOS32_SPI2_RCLK2_SET(pin_value);
	  break;

        default:
	  return -3; // unsupported RC pin
      }
      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Transfers a byte to SPI output and reads back the return value from SPI input
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] b the byte which should be transfered
//! \return >= 0: the read byte
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported SPI mode configured via MIOS32_SPI_TransferModeInit()
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferByte(u8 spi, u8 b)
{
  switch( spi ) {
    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      // poll TNF flag (transfer FIFO not full)
      while( !(MIOS32_SPI1_PTR->SR & (1 << 1)) ); // TNF flag

      // send byte
      MIOS32_SPI1_PTR->DR = b;

      // wait until RNE flag is set (Receive FIFO not empty)
      while( !(MIOS32_SPI1_PTR->SR & (1 << 2)) ); // RNE Flag

      // return received byte
      return MIOS32_SPI1_PTR->DR;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      // send byte
      MIOS32_SPI2_PTR->SPDR = b;

      // wait until SPI transfer finished
      while( !(MIOS32_SPI2_PTR->SPSR & (1 << 7)) ); // SPIF flag

      // return received byte
      return MIOS32_SPI2_PTR->SPDR;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
//! Transfers a block of bytes via DMA.
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] send_buffer pointer to buffer which should be sent.<BR>
//! If NULL, 0xff (all-one) will be sent.
//! \param[in] receive_buffer pointer to buffer which should get the received values.<BR>
//! If NULL, received bytes will be discarded.
//! \param[in] len number of bytes which should be transfered
//! \param[in] callback pointer to callback function which will be executed
//! from DMA channel interrupt once the transfer is finished.
//! If NULL, no callback function will be used, and MIOS32_SPI_TransferBlock() will
//! block until the transfer is finished.
//! \return >= 0 if no error during transfer
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if function has been called during an ongoing DMA transfer
//! \return -4 too many bytes (len too long)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback)
{
  u8 rx_chn, tx_chn;
  u32 rx_chn_req, tx_chn_req;
  LPC_SSP_TypeDef *ssp_ptr = NULL;

  switch( spi ) {
    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      // provide pointers to Rx/Tx DMA channel and SSP peripheral
      rx_chn = MIOS32_SPI1_DMA_RX_CHN;
      tx_chn = MIOS32_SPI1_DMA_TX_CHN;
      rx_chn_req = MIOS32_SPI1_DMA_RX_REQ;
      tx_chn_req = MIOS32_SPI1_DMA_TX_REQ;
      ssp_ptr = MIOS32_SPI1_PTR;
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
    // no connection to DMA available
    // Software Emulation
    {
      u32 pos;

      // we have 4 cases:
      if( receive_buffer != NULL ) {
	if( send_buffer != NULL ) {
	  for(pos=0; pos<len; ++pos)
	    *receive_buffer++ = MIOS32_SPI_TransferByte(spi, *send_buffer++);
	} else {
	  for(pos=0; pos<len; ++pos)
	    *receive_buffer++ = MIOS32_SPI_TransferByte(spi, 0xff);
	}
      } else {
	// TODO: we could use an optimized "send only" function to speed up the SW emulation!
	if( send_buffer != NULL ) {
	  for(pos=0; pos<len; ++pos)
	    MIOS32_SPI_TransferByte(spi, *send_buffer++);
	} else {
	  // nothing to do...
	}
      }

      // callback
      if( callback != NULL ) {
	void (*_callback)(void) = callback;
	_callback();
      }

      return 0; // END of SW emulation - EXIT here!
    } break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  if( len >= 0x1000 )
    return -4; // too many bytes (len too long)

  LPC_GPDMACH_TypeDef *rx_chn_ptr = (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + rx_chn*0x20);
  LPC_GPDMACH_TypeDef *tx_chn_ptr = (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + tx_chn*0x20);

  // disable callback
  MIOS32_SYS_DMA_CallbackSet(0, rx_chn, NULL);

  // disable DMA channels
  rx_chn_ptr->DMACCConfig = 0;
  tx_chn_ptr->DMACCConfig = 0;

  // clear pending interrupt requests
  LPC_GPDMA->DMACIntTCClear = (1 << rx_chn) | (1 << tx_chn);
  LPC_GPDMA->DMACIntErrClr = (1 << rx_chn) | (1 << tx_chn);

  // install DMA callback for receive channel
  MIOS32_SYS_DMA_CallbackSet(0, rx_chn, callback);
  
  // set source/destination address
  rx_chn_ptr->DMACCSrcAddr = (u32)(&ssp_ptr->DR);
  rx_chn_ptr->DMACCDestAddr = (u32)receive_buffer;
  // no linked list
  rx_chn_ptr->DMACCLLI = 0;
  // set transfer size, enable increment for destination address, enable terminal count interrupt
  rx_chn_ptr->DMACCControl = (len << 0) | (0 << 26) | (1 << 27) | (1 << 31);
  // enable channel, source peripheral is SSP1 Rx, destination is ignored, peripheral-to-memory, enable mask for terminal count irq
  u32 itc = (callback != NULL) ? (1 << 15) : 0; // enable terminal count interrupt only if callback enabled
  rx_chn_ptr->DMACCConfig = (1 << 0) | (rx_chn_req << 1) | (0 << 6) | (2 << 11) | itc;

  // set source/destination address
  tx_chn_ptr->DMACCSrcAddr = (u32)send_buffer;
  tx_chn_ptr->DMACCDestAddr = (u32)(&ssp_ptr->DR);
  // no linked list
  tx_chn_ptr->DMACCLLI = 0;
  // set transfer size, enable increment for source address
  tx_chn_ptr->DMACCControl = (len << 0) | (1 << 26) | (0 << 27);
  // enable channel, dest. peripheral is SSP1 Tx, source is ignored, memory-to-peripheral
  tx_chn_ptr->DMACCConfig = (1 << 0) | (0 << 1) | (tx_chn_req << 6) | (1 << 11);

  // start DMA via soft request on Tx channel
  LPC_GPDMA->DMACSoftSReq = (1 << tx_chn);

  // if no callback: wait until all bytes have been received
  if( callback == NULL )
    while( rx_chn_ptr->DMACCControl & 0xfff ); // wait until TransferSize field is 0

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_SPI */

