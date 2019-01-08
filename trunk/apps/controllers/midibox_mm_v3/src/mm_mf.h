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

#ifndef _MM_MF_H
#define _MM_MF_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_MF_Init(u32 mode);
extern u16 MM_MF_FaderPosGet(u8 fader);
extern s32 MM_MF_FaderEvent(u8 fader, u16 position_16bit);
extern s32 MM_MF_FaderMove(u8 fader, u16 position_16bit);


#endif /* _MM_MF_H */
