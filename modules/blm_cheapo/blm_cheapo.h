// $Id$
/*
 * Header file for Cheapo BLM
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_CHEAPO_H
#define _BLM_CHEAPO_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// reverse pins for right half (layout measure)
// can optionally be disabled in mios32_config.h by setting this define to 0
#ifndef BLM_CHEAPO_RIGHT_HALF_REVERSED
#define BLM_CHEAPO_RIGHT_HALF_REVERSED 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BLM_CHEAPO_Init(u32 mode);
extern s32 BLM_CHEAPO_PrepareCol(void);
extern s32 BLM_CHEAPO_GetRow(void);
extern s32 BLM_CHEAPO_ButtonHandler(void *_notify_hook);

extern s32 BLM_CHEAPO_DebounceSet(u8 delay);
extern s32 BLM_CHEAPO_DebounceGet(void);

extern s32 BLM_CHEAPO_DOUT_PinSet(u32 pin, u32 value);
extern s32 BLM_CHEAPO_DOUT_PinGet(u32 pin);
extern s32 BLM_CHEAPO_DOUT_SRSet(u32 row, u8 value);
extern u8 BLM_CHEAPO_DOUT_SRGet(u32 row);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _BLM_CHEAPO_H */
