// $Id$
/*
 * Header file for SRIO Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SRIO_H
#define _MIOS32_SRIO_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// 16 should be maximum, more registers would require buffers at the SCLK/RCLK lines
// and probably also a lower scan frequency
// The number of SRs can be optionally overruled from the local mios32_config.h file
#ifndef MIOS32_SRIO_NUM_SR
#define MIOS32_SRIO_NUM_SR 16
#endif


// Which SPI peripheral should be used
// allowed values: 0 and 1
// (note: SPI0 will allocate DMA channel 2 and 3, SPI1 will allocate DMA channel 4 and 5)
#ifndef MIOS32_SRIO_SPI
#ifdef MIOS32_BOARD_STM32_PRIMER
# define MIOS32_SRIO_SPI 0 // since RCLK conflicts with USB detach pin @B12
#else
# define MIOS32_SRIO_SPI 1
#endif
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC)
#ifndef MIOS32_SRIO_SPI_RC_PIN
#define MIOS32_SRIO_SPI_RC_PIN 0
#endif

// should output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#ifndef MIOS32_SRIO_OUTPUTS_OD
#define MIOS32_SRIO_OUTPUTS_OD 1
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SRIO_Init(u32 mode);

extern s32 MIOS32_SRIO_ScanStart(void *notify_hook);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern volatile u8 mios32_srio_dout[MIOS32_SRIO_NUM_SR];
extern volatile u8 mios32_srio_din[MIOS32_SRIO_NUM_SR];
extern volatile u8 mios32_srio_din_buffer[MIOS32_SRIO_NUM_SR]; // only required for emulation
extern volatile u8 mios32_srio_din_changed[MIOS32_SRIO_NUM_SR];


#endif /* _MIOS32_SRIO_H */
