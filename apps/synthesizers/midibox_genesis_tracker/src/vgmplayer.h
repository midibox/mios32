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


#include <genesis.h>
#include "vgm.h"


struct vgmp_chipdata {
    u16 lastwritetime;
    u8 cmdmadebusy;
};

// Call at startup
extern void VgmPlayer_Init();

extern void VgmPlayer_AddHead(VgmHead* vgmh);
extern void VgmPlayer_RemoveHead(VgmHead* vgmh);


#endif /* _VGMPLAYER_H */
