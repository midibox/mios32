// $Id$
/*
 * Header file for LED Digits Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LC_LEDDIGITS_H
#define _LC_LEDDIGITS_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern u8 lc_leddigits_mtc[10];

extern u8 lc_leddigits_status[2];

	
/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_LEDDIGITS_Init(u32 mode);
extern s32 LC_LEDDIGITS_MTCSet(u8 number, u8 pattern);
extern s32 LC_LEDDIGITS_StatusSet(u8 number, u8 pattern);

#endif /* _LC_LEDDIGITS_H */
