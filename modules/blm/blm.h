// $Id$
/*
 * Header file for Button/LED Matrix
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_H
#define _BLM_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called (e.g. ("-DBLM_DOUT_L1=12")


// define the shift registers to which the anodes of these LEDs are connected
// Note: they can be equal to DEFAULT_GP_DOUT_SR_[LH], this saves two shift registers, but doesn't allow a separate view of UI selections
#ifndef BLM_DOUT_L1
#define BLM_DOUT_L1	2
#endif
#ifndef BLM_DOUT_R1
#define BLM_DOUT_R1	5
#endif


// define the shift register to which the cathodes of these LEDs are connected
// Note that the whole shift register (8 pins) will be allocated! The 4 select lines are duplicated (4 for LED matrix, 4 for button matrix)
// The second DOUT_CATHODES2 selection is optional if LEDs with high power consumption are used - set this to 0 if not used
#ifndef BLM_DOUT_CATHODES1
#define BLM_DOUT_CATHODES1	1
#endif
#ifndef BLM_DOUT_CATHODES2
#define BLM_DOUT_CATHODES2	4
#endif


// set an inversion mask for the DOUT shift registers if sink drivers (transistors)
// have been added to the cathode lines
// Settings: 0x00 - no sink drivers
//           0xf0 - sink drivers connected to D0..D3
//           0x0f - sink drivers connected to D7..D4
#ifndef BLM_CATHODES_INV_MASK
#define BLM_CATHODES_INV_MASK	0x00
#endif

// define the shift registers to which the anodes of the "second colour" (red) LEDs are connected
#ifndef BLM_DOUT_L2
#define BLM_DOUT_L2	3
#endif
#ifndef BLM_DOUT_R2
#define BLM_DOUT_R2	6
#endif

// define the DIN shift registers to which the button matrix is connected
#ifndef BLM_DIN_L
#define BLM_DIN_L	1
#endif
#ifndef BLM_DIN_R
#define BLM_DIN_R	2
#endif

// number of colours (currently only 1 or 2 supported)
// more colours (e.g. for RGB LEDs) require additional DOUT definitions - or maybe a soft configuration via variables?
#ifndef BLM_NUM_COLOURS
#define BLM_NUM_COLOURS 2
#endif

// number of rows (currently only 8 supported)
// more rows require additional DOUT definitions - or maybe a soft configuration via variables?
#ifndef BLM_NUM_ROWS
#define BLM_NUM_ROWS 8
#endif

// 0: no debouncing
// 1: cheap debouncing (all buttons the same time)
// 2: individual debouncing of all buttons
#ifndef BLM_DEBOUNCE_MODE
#define BLM_DEBOUNCE_MODE 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern s32 BLM_Init(u32 mode);
extern s32 BLM_PrepareCol(void);
extern s32 BLM_GetRow(void);
extern s32 BLM_ButtonHandler(void *notify_hook);

extern s32 BLM_DIN_PinGet(u32 pin);
extern u8 BLM_DIN_SRGet(u32 sr);

extern s32 BLM_DOUT_PinSet(u32 colour, u32 pin, u32 value);
extern s32 BLM_DOUT_PinGet(u32 colour, u32 pin);
extern s32 BLM_DOUT_SRSet(u32 colour, u32 sr, u8 value);
extern u8 BLM_DOUT_SRGet(u32 colour, u32 sr);

extern s32 BLM_DebounceDelaySet(u8 delay);
extern u8 BLM_DebounceDelayGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _BLM_H */
