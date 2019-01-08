// $Id$
/*
 * Header file for DOUT Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_DOUT_H
#define _MIOS32_DOUT_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_DOUT_Init(u32 mode);

extern s32 MIOS32_DOUT_PinGet(u32 pin);
extern s32 MIOS32_DOUT_PinSet(u32 pin, u32 value);

extern s32 MIOS32_DOUT_PagePinGet(u8 page, u32 pin);
extern s32 MIOS32_DOUT_PagePinSet(u8 page, u32 pin, u32 value);

extern s32 MIOS32_DOUT_SRGet(u32 sr);
extern s32 MIOS32_DOUT_SRSet(u32 sr, u8 value);

extern s32 MIOS32_DOUT_PageSRGet(u8 page, u32 sr);
extern s32 MIOS32_DOUT_PageSRSet(u8 page, u32 sr, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const u8 mios32_dout_reverse_tab[256];

#endif /* _MIOS32_DOUT_H */
