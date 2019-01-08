// $Id$
/*
 * Header file for VPot Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MM_VPOT_H
#define _MM_VPOT_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

s32 MM_VPOT_Init(u32 mode);
s32 MM_VPOT_LEDRing_CheckUpdates(void);
s32 MM_VPOT_LEDRing_SRHandler(void);

u8 MM_VPOT_DisplayTypeGet(void);
s32 MM_VPOT_DisplayTypeSet(u8 type);

u8 MM_VPOT_ValueGet(u8 pointer);
s32 MM_VPOT_ValueSet(u8 pointer, u8 value);

s32 MM_VPOT_LEDRingUpdateSet(u8 rings);

s32 MM_VPOT_SendENCEvent(u8 encoder, s32 incrementer);
s32 MM_VPOT_SendJogWheelEvent(s32 incrementer);      

#endif /* _MM_VPOT_H */
