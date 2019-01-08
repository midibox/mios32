// $Id$
/*
 * Header File for General Purpose Controller
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MM_GPC_H
#define _MM_GPC_H

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MM_GPC_Init(u32 mode);
extern u8 MM_GPC_VPotValueGet(u8 vpot);
extern s32 MM_GPC_Received(mios32_midi_package_t package);
extern s32 MM_GPC_AbsValue_Received(u8 entry, u8 value);
extern s32 MM_GPC_Msg_Update(void);
extern s32 MM_GPC_Msg_UpdateValues(void);

extern s32 MM_GPC_SendENCEvent(u8 encoder, s32 incrementer);
extern s32 MM_GPC_SendJogWheelEvent(s32 incrementer);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

extern char gpc_msg[128];

#endif /* _MM_GPC_H */
