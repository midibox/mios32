// $Id: mios32_spi.h 358 2009-02-21 23:21:17Z tk $
/*
 * Header file for SPI Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SPI_H
#define _MIOS32_SPI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MIOS32_SPI_PIN_DRIVER_STRONG=0,
  MIOS32_SPI_PIN_DRIVER_STRONG_OD=1,
  MIOS32_SPI_PIN_DRIVER_WEAK=2,
  MIOS32_SPI_PIN_DRIVER_WEAK_OD=3,
} mios32_spi_pin_driver_t;

typedef enum {
  MIOS32_SPI_MODE_CLK0_PHASE0=0,
  MIOS32_SPI_MODE_CLK0_PHASE1=1,
  MIOS32_SPI_MODE_CLK1_PHASE0=2,
  MIOS32_SPI_MODE_CLK1_PHASE1=3
} mios32_spi_mode_t;

typedef enum {
  MIOS32_SPI_PRESCALER_2=0,
  MIOS32_SPI_PRESCALER_4=1,
  MIOS32_SPI_PRESCALER_8=2,
  MIOS32_SPI_PRESCALER_16=3,
  MIOS32_SPI_PRESCALER_32=4,
  MIOS32_SPI_PRESCALER_64=5,
  MIOS32_SPI_PRESCALER_128=6,
  MIOS32_SPI_PRESCALER_256=7
} mios32_spi_prescaler_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SPI_Init(u32 mode);

extern s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver);
extern s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler);


extern s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value);
extern s32 MIOS32_SPI_TransferByte(u8 spi, u8 b);
extern s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_SPI_H */
