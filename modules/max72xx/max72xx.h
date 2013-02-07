// $Id$
/*
 * Header file for MAX72XX functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MAX72XX_H
#define _MAX72XX_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Maximum number of MAX72XX chains (1 or 2)
// Each chain has a dedicated "load" line (e.g. J19:RC1 or J19:RC2)
// (Number of chains can be changed via soft-configuration during runtime.)
#ifndef MAX72XX_NUM_CHAINS
#define MAX72XX_NUM_CHAINS 1
#endif


// Maximum number of MAX72xx devices connected per chains
// (Number of devices within this range can be changed via soft-configuration during runtime.)
#ifndef MAX72XX_NUM_DEVICES_PER_CHAIN
#define MAX72XX_NUM_DEVICES_PER_CHAIN 16
#endif


// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef MAX72XX_SPI
#define MAX72XX_SPI 2
#endif

// Which RC pin of the SPI port should be used for the first chain
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef MAX72XX_SPI_RC_PIN_CHAIN1
#define MAX72XX_SPI_RC_PIN_CHAIN1 0
#endif

// Which RC pin of the SPI port should be used for the second module
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef MAX72XX_SPI_RC_PIN_CHAIN2
#define MAX72XX_SPI_RC_PIN_CHAIN2 1
#endif
// more CS lines are possible, but not prepared yet (MAX72XX_SetCs() has to be enhanced)

// should output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#ifndef MAX72XX_SPI_OUTPUTS_OD
#if MIOS32_BOARD_MBHP_CORE_STM32
# define MAX72XX_SPI_OUTPUTS_OD 1
#else
  // e.g. MBHP_CORE_LPC17 module
# define MAX72XX_SPI_OUTPUTS_OD 0
#endif
#endif


// MAX72xx registers
#define MAX72XX_REG_NOP          0x00
#define MAX72XX_REG_DIGIT0       0x01
#define MAX72XX_REG_DIGIT1       0x02
#define MAX72XX_REG_DIGIT2       0x03
#define MAX72XX_REG_DIGIT3       0x04
#define MAX72XX_REG_DIGIT4       0x05
#define MAX72XX_REG_DIGIT5       0x06
#define MAX72XX_REG_DIGIT6       0x07
#define MAX72XX_REG_DIGIT7       0x08
#define MAX72XX_REG_DECODE_MODE  0x09
#define MAX72XX_REG_INTENSITY    0x0a
#define MAX72XX_REG_SCAN_LIMIT   0x0b
#define MAX72XX_REG_SHUTDOWN     0x0c
#define MAX72XX_REG_TESTMODE     0x0f



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MAX72XX_Init(u32 mode);

extern s32 MAX72XX_NumChainsGet(void);
extern s32 MAX72XX_NumChainsSet(u8 num_chains);

extern s32 MAX72XX_EnabledGet(u8 chain);
extern s32 MAX72XX_EnabledSet(u8 chain, u8 enabled);

extern s32 MAX72XX_NumDevicesPerChainGet(u8 chain);
extern s32 MAX72XX_NumDevicesPerChainSet(u8 chain, u8 num_devices);

extern s32 MAX72XX_DigitSet(u8 chain, u8 device, u8 digit, u8 value);
extern s32 MAX72XX_AllDigitsSet(u8 chain, u8 device, u8 *values);
extern s32 MAX72XX_DigitGet(u8 chain, u8 device, u8 digit);

extern s32 MAX72XX_WriteReg(u8 chain, u8 device, u8 reg, u8 value);
extern s32 MAX72XX_WriteAllRegs(u8 chain, u8 reg, u8 value);

extern s32 MAX72XX_UpdateDigit(u8 chain, u8 digit);
extern s32 MAX72XX_UpdateAllDigits(u8 chain);
extern s32 MAX72XX_UpdateAllChains(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _MAX72XX_H */
