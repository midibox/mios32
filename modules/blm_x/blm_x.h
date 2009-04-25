/*
 * Header file for blm_x.c
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_X_H
#define _BLM_X_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called (e.g. ("-DBLM_X_NUM_ROWS=8")
// or write the defines into your mios32_config.h file
#include <mios32_config.h>


//---------------------------- matrix dimensions ------------------------------------------

// number of rows for button/LED matrix (max. 8)
#ifndef BLM_X_NUM_ROWS
#define BLM_X_NUM_ROWS 4
#endif

// number of cols for DIN matrix.
// this value affects the number of DIN serial-registers used
#ifndef BLM_X_BTN_NUM_COLS
#define BLM_X_BTN_NUM_COLS 4
#endif

// number of cols for DOUT matrix.
// this value affects the number of DOUT serial-registers used
#ifndef BLM_X_LED_NUM_COLS
#define BLM_X_LED_NUM_COLS 4
#endif

// number of colors / different LEDS used at one matrix-crosspoint.
// this value affects the number of DOUT serial-registers used
#ifndef BLM_X_LED_NUM_COLORS
#define BLM_X_LED_NUM_COLORS 3
#endif


//--------------------------- serial registers assignment ----------------------------------
//-- note that serial registers start to count at 0 (not like in PIC MIOS) --


// DOUT shift register to which the cathodes of the LEDs are connected (row selectors).
// if less than 5 rows are defined, the higher nibble of the SR outputs will be always 
// identical to the lower nibble.
#ifndef BLM_X_ROWSEL_DOUT_SR
#define BLM_X_ROWSEL_DOUT_SR	0
#endif


// first DOUT shift register to which the anodes of the LEDs are connected. the number
// of registers used is ceil(BLM_X_LED_NUM_COLS*BLM_X_LED_NUM_COLORS / 8), subsequent
// registers will be used
#ifndef BLM_X_LED_FIRST_DOUT_SR
#define BLM_X_LED_FIRST_DOUT_SR	1
#endif


// first DIN shift registers to which the button matrix is connected.
// subsequent shift registers will be used, if more than 8 cols are defined.
#ifndef BLM_X_BTN_FIRST_DIN_SR
#define BLM_X_BTN_FIRST_DIN_SR	0
#endif

//--------------------------- additional options --------------------------

// set an inversion mask for the row selection shift registers if sink drivers (transistors)
// have been added to the cathode lines
// Settings: 0x00 - no sink drivers
//           0xff - sink drivers connected to D7..D0
//           0x0f - sink drivers connected to D3..D0
//           0xf0 - sink drivers connected to D7..D4
#ifndef BLM_X_ROWSEL_INV_MASK
#define BLM_X_ROWSEL_INV_MASK	0x00
#endif


// 1: cheap debouncing (all buttons the same time)
// 2: individual debouncing of all buttons
#ifndef BLM_X_DEBOUNCE_MODE
#define BLM_X_DEBOUNCE_MODE 2
#endif


/////////////////////////////////////////////////////////////////////////////
// Computed defines
/////////////////////////////////////////////////////////////////////////////

#if (BLM_X_BTN_NUM_COLS % 8) == 0
#define BLM_X_NUM_BTN_SR (BLM_X_BTN_NUM_COLS / 8)
#else
#define BLM_X_NUM_BTN_SR (BLM_X_BTN_NUM_COLS / 8 + 1)
#endif

#if ((BLM_X_LED_NUM_COLS * BLM_X_LED_NUM_COLORS) % 8) == 0
#define BLM_X_NUM_LED_SR  ( (BLM_X_LED_NUM_COLS * BLM_X_LED_NUM_COLORS) / 8)
#else
#define BLM_X_NUM_LED_SR  ( (BLM_X_LED_NUM_COLS * BLM_X_LED_NUM_COLORS) / 8 + 1)
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// module initialization
extern s32 BLM_X_Init(void);

// module hooks
extern s32 BLM_X_PrepareRow(void);
extern s32 BLM_X_GetRow(void);
extern s32 BLM_X_BtnHandler(void *notify_hook);

// get buttons states
extern s32 BLM_X_BtnGet(u32 btn);
extern u8 BLM_X_BtnSRGet(u8 row, u8 sr);

// set LED states
extern s32 BLM_X_LEDSet(u32 led, u32 color, u32 value);
extern s32 BLM_X_LEDColorSet(u32 led, u32 color_mask);
extern s32 BLM_X_LEDSRSet(u8 row, u8 sr, u8 sr_value);

// get LED states
extern s32 BLM_X_LEDGet(u32 led, u32 color);
extern u8 BLM_X_LEDSRGet(u8 row, u8 sr);

// set / get debounce-delay
extern s32 BLM_X_DebounceDelaySet(u8 delay);
extern u8 BLM_X_DebounceDelayGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// Access to the DOUT shift-registers of the single rows.
// one row needs (num_cols * num_colors) bits, each register holds 8 bits,
// first color first, last color last.
// [color 0, col 0][color 0, col 1]...[color 1, col 0][color 1, col 1]....
extern u8 BLM_X_LED_rows[BLM_X_NUM_ROWS][BLM_X_NUM_LED_SR];


#endif /* _BLM_X_H */
