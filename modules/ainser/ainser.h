// $Id$
/*
 * Header file for AINSER functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _AINSER_H
#define _AINSER_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Maximum number of AINSER modules (1..255)
// (Number of modules can be changed via soft-configuration during runtime.)
#ifndef AINSER_NUM_MODULES
#define AINSER_NUM_MODULES 1
#endif


// Maximum number of AIN pins per AINSER module (1..64)
// (Number of pins within this range can be changed via soft-configuration during runtime.)
#ifndef AINSER_NUM_PINS
#define AINSER_NUM_PINS 64
#endif


// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef AINSER_SPI
#define AINSER_SPI 2
#endif

// Which RC pin of the SPI port should be used for the first module
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef AINSER_SPI_RC_PIN_MODULE1
#define AINSER_SPI_RC_PIN_MODULE1 0
#endif

// Which RC pin of the SPI port should be used for the second module
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef AINSER_SPI_RC_PIN_MODULE2
#define AINSER_SPI_RC_PIN_MODULE2 1
#endif
// more CS lines are possible, but not prepared yet (AINSER_SetCs() has to be enhanced)

// should output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#ifndef AINSER_SPI_OUTPUTS_OD
#if MIOS32_BOARD_MBHP_CORE_STM32
# define AINSER_SPI_OUTPUTS_OD 1
#else
  // e.g. MBHP_CORE_LPC17 module
# define AINSER_SPI_OUTPUTS_OD 0
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 AINSER_Init(u32 mode);

extern s32 AINSER_NumModulesGet(void);
extern s32 AINSER_NumModulesSet(u8 num_modules);

extern s32 AINSER_EnabledGet(u8 module);
extern s32 AINSER_EnabledSet(u8 module, u8 enabled);

extern s32 AINSER_NumPinsGet(u8 module);
extern s32 AINSER_NumPinsSet(u8 module, u8 num_pins);

extern s32 AINSER_DeadbandGet(u8 module);
extern s32 AINSER_DeadbandSet(u8 module, u8 deadband);

extern s32 AINSER_PinGet(u8 module, u8 pin);
extern s32 AINSER_PreviousPinValueGet(void);

extern s32 AINSER_Handler(void (*_callback)(u32 module, u32 pin, u32 value));


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _AINSER_H */
