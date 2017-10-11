// $Id$
/*
 * Header file for SEQ Button/LED Matrix handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2017 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_BLM8X8_H
#define _SEQ_BLM8X8_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of 8x8 matrices
#ifndef SEQ_BLM8X8_NUM
#define SEQ_BLM8X8_NUM 3
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 rowsel_dout_sr;
  u8 led_dout_sr;
  u8 button_din_sr;
  u8 rowsel_inv_mask;
  u8 col_inv_mask;
  u8 debounce_delay;
} seq_blm8x8_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_BLM8X8_Init(u32 mode);

extern s32 SEQ_BLM8X8_PrepareRow(void);
extern s32 SEQ_BLM8X8_GetRow(void);
extern s32 SEQ_BLM8X8_ButtonHandler(void *notify_hook);

extern s32 SEQ_BLM8X8_ButtonGet(u8 blm, u32 button);
extern u8 SEQ_BLM8X8_ButtonSRGet(u8 blm, u8 row);

extern s32 SEQ_BLM8X8_LEDSet(u8 blm, u8 led, u8 value);
extern s32 SEQ_BLM8X8_LEDSRSet(u8 blm, u8 row, u8 sr_value);

extern s32 SEQ_BLM8X8_LEDGet(u8 blm, u8 led);
extern u8 SEQ_BLM8X8_LEDSRGet(u8 blm, u8 row);

extern s32 SEQ_BLM8X8_ConfigSet(u8 blm, seq_blm8x8_config_t config);
extern seq_blm8x8_config_t SEQ_BLM8X8_ConfigGet(u8 blm);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

u8 seq_blm8x8_led_row[SEQ_BLM8X8_NUM][8];


#endif /* _SEQ_BLM8X8_H */
