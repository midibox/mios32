// $Id$
/*
 * Header file for 8x8 Button/LED Matrix
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM8X8_H
#define _BLM8X8_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called (e.g. ("-DBLM8X8_DOUT_L1=12")
// or write the defines into your mios32_config.h file
#include <mios32_config.h>


// define the shift register to which the anodes of the LEDs are connected
#ifndef BLM8X8_DOUT
#define BLM8X8_DOUT	1
#endif


// define the shift register to which the cathodes of the LEDs are connected
#ifndef BLM8X8_DOUT_CATHODES
#define BLM8X8_DOUT_CATHODES	2
#endif


// set an inversion mask for the DOUT shift registers if sink drivers (transistors)
// have been added to the cathode lines
// Settings: 0x00 - no sink drivers
//           0xff - sink drivers connected to D7..D0
#ifndef BLM8X8_CATHODES_INV_MASK
#define BLM8X8_CATHODES_INV_MASK	0x00
#endif

// define the DIN shift registers to which the button matrix is connected
#ifndef BLM8X8_DIN
#define BLM8X8_DIN	1
#endif

// number of rows (currently only 8 supported)
// more rows require additional DOUT definitions - or maybe a soft configuration via variables?
#ifndef BLM8X8_NUM_ROWS
#define BLM8X8_NUM_ROWS 8
#endif

// 0: no debouncing
// 1: cheap debouncing (all buttons the same time)
// 2: individual debouncing of all buttons
#ifndef BLM8X8_DEBOUNCE_MODE
#define BLM8X8_DEBOUNCE_MODE 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern s32 BLM8X8_Init(u32 mode);
extern s32 BLM8X8_PrepareCol(void);
extern s32 BLM8X8_GetRow(void);
extern s32 BLM8X8_ButtonHandler(void *notify_hook);

extern s32 BLM8X8_DIN_PinGet(u32 pin);
extern u8 BLM8X8_DIN_SRGet(u32 sr);

extern s32 BLM8X8_DOUT_PinSet(u32 pin, u32 value);
extern s32 BLM8X8_DOUT_PinGet(u32 pin);
extern s32 BLM8X8_DOUT_SRSet(u32 sr, u8 value);
extern u8 BLM8X8_DOUT_SRGet(u32 sr);

extern s32 BLM8X8_DebounceDelaySet(u8 delay);
extern u8 BLM8X8_DebounceDelayGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access (bypasses BLM8X8_DOUT_SR* functions)
extern u8 blm8x8_led_row[BLM8X8_NUM_ROWS];


#endif /* _BLM8X8_H */
