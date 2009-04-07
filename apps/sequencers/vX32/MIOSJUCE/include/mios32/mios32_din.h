// $Id: mios32_din.h 144 2008-12-02 00:07:05Z tk $
/*
 * Header file for DIN Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_DIN_H
#define _MIOS32_DIN_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_DIN_Init(u32 mode);

extern s32 MIOS32_DIN_PinGet(u32 pin);
extern s32 MIOS32_DIN_SRGet(u32 sr);
u8 MIOS32_DIN_SRChangedGetAndClear(u32 sr, u8 mask);

extern s32 MIOS_DIN_PinAutoRepeatDisable(u32 pin);
extern s32 MIOS_DIN_PinAutoRepeatEnable(u32 pin);
extern s32 MIOS_DIN_PinAutoRepeatGet(u32 pin);

extern u32 MIOS_DIN_DebounceGet(void);
extern s32 MIOS_DIN_DebounceSet(u32 debounce_time);

extern s32 MIOS32_DIN_Handler(void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_DIN_H */
