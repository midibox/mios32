// $Id$
/*
 * Header file for Meter Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LC_METERS_H
#define _LC_METERS_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

// 16 bit words for 8 meters 
// global variable, used also be LC_VPOT_LEDRing_SRHandler
u16 meter_pattern[8];

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_METERS_Init(u32 mode);
extern s32 LC_METERS_CheckUpdates(void);

extern u8 LC_METERS_GlobalModeGet(void);
extern s32 LC_METERS_GlobalModeSet(u8 mode);
extern u8 LC_METERS_ModeGet(u8 meter);
extern s32 LC_METERS_ModeSet(u8 meter, u8 mode);
extern u8 LC_METERS_LevelGet(u8 meter);
extern s32 LC_METERS_LevelSet(u8 meter, u8 level);

extern s32 LC_METERS_Timer(void);

#endif /* _LC_METERS_H */
