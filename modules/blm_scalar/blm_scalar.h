// $Id$
/*
 * Header file for BLM_SCALAR driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_SCALAR_H
#define _BLM_SCALAR_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called (e.g. ("-DBLM_DOUT_L1=12")
// or write the defines into your mios32_config.h file
#include <mios32_config.h>

#ifndef BLM_SCALAR_NUM_MODULES
#define BLM_SCALAR_NUM_MODULES 5
#endif

#ifndef BLM_SCALAR_NUM_COLOURS
#define BLM_SCALAR_NUM_COLOURS 2
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 dout_cathodes[BLM_SCALAR_NUM_MODULES];
  u8 dout[BLM_SCALAR_NUM_MODULES][BLM_SCALAR_NUM_COLOURS];
  u8 din[BLM_SCALAR_NUM_MODULES];
  u8 cathodes_inv_mask;
  u8 debounce_delay;
} blm_scalar_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
extern s32 BLM_SCALAR_Init(u32 mode);
extern s32 BLM_SCALAR_PrepareCol(void);
extern s32 BLM_SCALAR_GetRow(void);
extern s32 BLM_SCALAR_ButtonHandler(void *notify_hook);

extern s32 BLM_SCALAR_DIN_PinGet(u32 pin);
extern u8  BLM_SCALAR_DIN_SRGet(u32 sr);

extern s32 BLM_SCALAR_PinSet(u32 colour, u32 pin, u32 value);
extern s32 BLM_SCALAR_DOUT_PinGet(u32 colour, u32 pin);
extern s32 BLM_SCALAR_DOUT_SRSet(u32 colour, u32 sr, u8 value);
extern u8  BLM_SCALAR_DOUT_SRGet(u32 colour, u32 sr);

extern s32 BLM_SCALAR_ConfigSet(blm_scalar_config_t config);
extern blm_scalar_config_t BLM_SCALAR_ConfigGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access (bypasses BLM_SCALAR_DOUT_SR* functions)
extern u8 blm_scalar_led[BLM_SCALAR_NUM_MODULES][8][BLM_SCALAR_NUM_COLOURS];


#endif /* _BLM_SCALAR_H */
