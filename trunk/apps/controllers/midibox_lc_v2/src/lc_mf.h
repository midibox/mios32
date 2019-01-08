// $Id$
/*
 * Header File for Motorfader Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _LC_MF_H
#define _LC_MF_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 LC_MF_Init(u32 mode);
extern u16 LC_MF_FaderPosGet(u8 fader);
extern s32 LC_MF_FaderEvent(u8 fader, u16 position_16bit);
extern s32 LC_MF_FaderMove(u8 fader, u16 position_16bit);


#endif /* _LC_MF_H */
