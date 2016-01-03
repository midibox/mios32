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

class VgmPlayer {
    VgmPlayer();
    ~VgmPlayer();
    
    
    
private:
    Vgm** vgms;
    u32 num_vgms;
};


#endif /* _VGMPLAYER_H */
