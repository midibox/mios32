// $Id$
/*
 * Header file for MBSID Parameter Handlers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_PAR_H
#define _SID_PAR_H


/////////////////////////////////////////////////////////////////////////////
// Global Defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_PAR_Set(u8 sid, u8 par, u16 value, u8 sidlr, u8 ins);
extern s32 SID_PAR_Set16(u8 sid, u8 par, u16 value, u8 sidlr, u8 ins);
extern s32 SID_PAR_SetNRPN(u8 sid, u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins);
extern s32 SID_PAR_SetWT(u8 sid, u8 par, u8 wt_value, u8 sidlr, u8 ins);

extern u16 SID_PAR_Get(u8 sid, u8 par, u8 sidlr, u8 ins, u8 shadow);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SID_PAR_H */
