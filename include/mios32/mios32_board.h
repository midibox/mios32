// $Id$
/*
 * Header file for Board Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_BOARD_H
#define _MIOS32_BOARD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MIOS32_BOARD_PIN_MODE_IGNORE = 0,
  MIOS32_BOARD_PIN_MODE_ANALOG,
  MIOS32_BOARD_PIN_MODE_INPUT,
  MIOS32_BOARD_PIN_MODE_INPUT_PD,
  MIOS32_BOARD_PIN_MODE_INPUT_PU,
  MIOS32_BOARD_PIN_MODE_OUTPUT_PP,
  MIOS32_BOARD_PIN_MODE_OUTPUT_OD
} mios32_board_pin_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_BOARD_Init(u32 mode);

extern s32 MIOS32_BOARD_LED_Init(u32 leds);
extern s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value);
extern u32 MIOS32_BOARD_LED_Get(void);

extern s32 MIOS32_BOARD_J5_PinInit(u8 pin, mios32_board_pin_mode_t mode);
extern s32 MIOS32_BOARD_J5_Set(u16 value);
extern s32 MIOS32_BOARD_J5_PinSet(u8 pin, u8 value);
extern s32 MIOS32_BOARD_J5_Get(void);
extern s32 MIOS32_BOARD_J5_PinGet(u8 pin);


extern s32 MIOS32_BOARD_J10_PinInit(u8 pin, mios32_board_pin_mode_t mode);
extern s32 MIOS32_BOARD_J10_Set(u16 value);
extern s32 MIOS32_BOARD_J10_PinSet(u8 pin, u8 value);
extern s32 MIOS32_BOARD_J10_Get(void);
extern s32 MIOS32_BOARD_J10_PinGet(u8 pin);


extern s32 MIOS32_BOARD_J28_PinInit(u8 pin, mios32_board_pin_mode_t mode);
extern s32 MIOS32_BOARD_J28_Set(u16 value);
extern s32 MIOS32_BOARD_J28_PinSet(u8 pin, u8 value);
extern s32 MIOS32_BOARD_J28_Get(void);
extern s32 MIOS32_BOARD_J28_PinGet(u8 pin);


extern s32 MIOS32_BOARD_J15_PortInit(u32 mode);
extern s32 MIOS32_BOARD_J15_DataSet(u8 data);
extern s32 MIOS32_BOARD_J15_SerDataShift(u8 data);
extern s32 MIOS32_BOARD_J15_RS_Set(u8 rs);
extern s32 MIOS32_BOARD_J15_RW_Set(u8 rw);
extern s32 MIOS32_BOARD_J15_E_Set(u8 lcd, u8 e);
extern s32 MIOS32_BOARD_J15_PollUnbusy(u8 lcd, u32 time_out);

extern s32 MIOS32_BOARD_DAC_PinInit(u8 chn, u8 enable);
extern s32 MIOS32_BOARD_DAC_PinSet(u8 chn, u16 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_BOARD_H */
