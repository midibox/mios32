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

#ifndef _LC_VPOT_H
#define _LC_VPOT_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

s32 LC_VPOT_Init(u32 mode);
s32 LC_VPOT_LEDRing_CheckUpdates(void);
s32 LC_VPOT_LEDRing_SRHandler(void);

u8 LC_VPOT_DisplayTypeGet(void);
s32 LC_VPOT_DisplayTypeSet(u8 type);

u8 LC_VPOT_ValueGet(u8 pointer);
s32 LC_VPOT_ValueSet(u8 pointer, u8 value);

s32 LC_VPOT_LEDRingUpdateSet(u8 rings);

s32 LC_VPOT_SendENCEvent(u8 encoder, s32 incrementer);
s32 LC_VPOT_SendJogWheelEvent(s32 incrementer);      
s32 LC_VPOT_SendFADEREvent(u8 encoder, s32 incrementer);

#endif /* _LC_VPOT_H */
