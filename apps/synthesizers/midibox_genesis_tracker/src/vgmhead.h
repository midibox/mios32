/*
 * MIDIbox Genesis Tracker: VGM Playback Head Class
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMHEAD_H
#define _VGMHEAD_H

#include "vgmsourcestream.h"

union ChipWriteCmd {
    u32 all;
    struct {
        u8 cmd; //0x50 PSG write, data in data only; 0x52, 0x53 OPN2 writes port 0, 1
        u8 addr;
        u8 data;
        u8 data2;
    };
};

class VgmHead {
public:
    VgmHead(VgmSourceStream* src);
    ~VgmHead();
    
    void restart(u32 vgm_time);
    
    void cmdNext(u32 vgm_time);
    inline bool cmdIsWait() {return iswait || isdone;}
    inline s32 cmdGetWaitRemaining(u32 vgm_time) {return (isdone ? 65535 : ((s32)ticks - (s32)vgm_time));}
    inline bool cmdIsChipWrite() {return iswrite && !isdone;}
    inline ChipWriteCmd cmdGetChipWrite() {return writecmd;}
    
    inline void setOPN2FreqMultiplier(u32 mult) {opn2mult = mult;} // Ratio of VGM to actual OPN2 clock, times 0x1000
    
    inline u32 getCurAddress() { return srcaddr; }
    
private:
    VgmSourceStream* source;
    u32 srcaddr;
    u32 srcblockaddr;
    
    bool iswait, iswrite;
    ChipWriteCmd writecmd;
    u32 ticks;
    
    u8 subbuffer[4];
    u8 subbufferlen;
    
    bool isdacwrite, isfreqwrite;
    
    bool isdone;
    u32 delay62, delay63;
    
    u32 opn2mult;
    
};

#endif /* _VGMHEAD_H */
