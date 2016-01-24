/*
 * MIDIbox Genesis Tracker: VGM Player
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMPLAYER_H
#define _VGMPLAYER_H


#include "vgmhead.h"


struct vgmp_chipdata {
    u16 lastwritetime;
    u8 cmdmadebusy;
};

// Call at startup
extern void VgmPlayer_Init();

extern void VgmPlayer_AddHead(VgmHead* vgmh);
extern void VgmPlayer_RemoveHead(VgmHead* vgmh);

extern u16 VgmPlayer_WorkCallback(u16 hr_time, u16 vgm_time);

#endif /* _VGMPLAYER_H */
