// $Id$
/*
 * Header file for AOUT functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _AOUT_H
#define _AOUT_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Maximum number of AOUT channels (1..32)
// (Number of channels within this range can be changed via soft-configuration during runtime.)
#ifndef AOUT_NUM_CHANNELS
#define AOUT_NUM_CHANNELS 8
#endif


// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef AOUT_SPI
#define AOUT_SPI 2
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef AOUT_SPI_RC_PIN
#define AOUT_SPI_RC_PIN 0
#endif

// should output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#ifndef AOUT_SPI_OUTPUTS_OD
#define AOUT_SPI_OUTPUTS_OD 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  AOUT_IF_NONE = 0,
  AOUT_IF_MAX525,
  AOUT_IF_74HC595,
  AOUT_IF_TLV5630,
  AOUT_IF_INTDAC
} aout_if_t;


typedef struct {
  aout_if_t  if_type;
  u8         num_channels;
  u32        if_option;
  u32        chn_inverted;
} aout_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 AOUT_Init(u32 mode);
extern s32 AOUT_IF_Init(u32 mode);

extern s32 AOUT_ConfigSet(aout_config_t config);
extern aout_config_t AOUT_ConfigGet(void);

extern s32 AOUT_PinSet(u8 pin, u16 value);
extern s32 AOUT_PinGet(u8 pin);

extern s32 AOUT_DigitalPinSet(u8 pin, u8 value);
extern s32 AOUT_DigitalPinGet(u8 pin);

extern s32 AOUT_DigitalPinsSet(u32 value);
extern u32 AOUT_DigitalPinsGet(void);

extern s32 AOUT_Update(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _AOUT_H */
