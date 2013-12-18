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
#if MIOS32_BOARD_MBHP_CORE_STM32
# define AOUT_SPI_OUTPUTS_OD 1
#else
  // e.g. MBHP_CORE_LPC17 module
# define AOUT_SPI_OUTPUTS_OD 0
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

#define AOUT_NUM_IF 7 // including "NONE"

// following types must be aligned with AOUT_IfNameGet()
typedef enum {
  AOUT_IF_NONE = 0,
  AOUT_IF_MAX525,
  AOUT_IF_74HC595,
  AOUT_IF_TLV5630,
  AOUT_IF_MCP4922_1, // with gain=1
  AOUT_IF_MCP4922_2, // with gain=2
  AOUT_IF_INTDAC // should always be located at last position to allow the application (like MBSEQ) to remove it from the list (clash with SD Card port)
} aout_if_t;


typedef struct {
  aout_if_t  if_type;
  u8         num_channels;
  u32        if_option;
  u32        chn_inverted;
  u32        chn_hz_v;
} aout_config_t;



// following types must be aligned with AOUT_CaliModeNameGet()
#define AOUT_NUM_CALI_MODES 8

// calibration modes
typedef enum {
  AOUT_CALI_MODE_OFF = 0,
  AOUT_CALI_MODE_MIN,
  AOUT_CALI_MODE_MIDDLE,
  AOUT_CALI_MODE_MAX,
  AOUT_CALI_MODE_1V,
  AOUT_CALI_MODE_2V,
  AOUT_CALI_MODE_4V,
  AOUT_CALI_MODE_8V,
} aout_cali_mode_t;



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 AOUT_Init(u32 mode);
extern s32 AOUT_IF_Init(u32 mode);

extern s32 AOUT_ConfigSet(aout_config_t config);
extern aout_config_t AOUT_ConfigGet(void);

extern s32 AOUT_ConfigChannelInvertedSet(u8 cv, u8 inverted);
extern s32 AOUT_ConfigChannelHzVSet(u8 cv, u8 hz_v);

extern const char* AOUT_IfNameGet(aout_if_t if_type);

extern s32 AOUT_CaliModeSet(u8 cv, aout_cali_mode_t mode);
extern aout_cali_mode_t AOUT_CaliModeGet(void);
extern u8 AOUT_CaliPinGet(void);
extern const char* AOUT_CaliNameGet(aout_cali_mode_t mode);

extern s32 AOUT_PinSet(u8 pin, u16 value);
extern s32 AOUT_PinGet(u8 pin);

extern s32 AOUT_PinSlewRateSet(u8 pin, u8 value);
extern s32 AOUT_PinSlewRateGet(u8 pin);

extern s32 AOUT_PinPitchRangeSet(u8 pin, u8 value);
extern s32 AOUT_PinPitchRangeGet(u8 pin);

extern s32 AOUT_PinPitchSet(u8 pin, s16 value);
extern s32 AOUT_PinPitchGet(u8 pin);

extern s32 AOUT_DigitalPinSet(u8 pin, u8 value);
extern s32 AOUT_DigitalPinGet(u8 pin);

extern s32 AOUT_DigitalPinsSet(u32 value);
extern u32 AOUT_DigitalPinsGet(void);

extern s32 AOUT_SuspendSet(u8 suspend);
extern s32 AOUT_SuspendGet(void);

extern s32 AOUT_Update(void);

extern s32 AOUT_TerminalHelp(void *_output_function);
extern s32 AOUT_TerminalParseLine(char *input, void *_output_function);
extern s32 AOUT_TerminalPrintConfig(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _AOUT_H */
