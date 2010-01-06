// $Id$
/*
 * Header file for Voice Allocation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_VOICE_H
#define _SID_VOICE_H


/////////////////////////////////////////////////////////////////////////////
// Global Defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_VOICE_Init(u32 mode);

extern s32 SID_VOICE_QueueInit(u8 sid);
extern s32 SID_VOICE_QueueInitExclusive(u8 sid);

extern u8 SID_VOICE_Get(u8 sid, u8 instrument, u8 voice_asg, u8 num_voices);
extern u8 SID_VOICE_GetLast(u8 sid, u8 instrument, u8 voice_asg, u8 num_voices, u8 search_voice);
extern u8 SID_VOICE_Release(u8 sid, u8 release_voice);

extern s32 SID_VOICE_SendDebugMessage(u8 sid);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SID_VOICE_H */
