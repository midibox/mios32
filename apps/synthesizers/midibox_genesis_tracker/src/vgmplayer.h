/*
 * MIDIbox Genesis Tracker: VGM Player Class
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
    u32 lastwritetime;
    u8 cmdmadebusy;
};

class VgmPlayer {
    VgmPlayer();
    ~VgmPlayer();
    
    
    
private:
    Vgm** vgms;
    u32 num_vgms;
    
    u32 hr_time;
    u32 sample_time;
    
    vgmp_chipdata chipdata[2*GENESIS_COUNT];
};

extern VgmPlayer vgmplayer;

#endif /* _VGMPLAYER_H */
