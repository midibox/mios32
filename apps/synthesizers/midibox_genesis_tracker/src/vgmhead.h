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
    VgmHead(VgmSource* src);
    ~VgmHead();
    
    void cmdNext(u32 curtime);
    inline bool cmdIsWait() {return iswait || isdone;}
    inline s32 cmdGetWaitRemaining(u32 curtime) {return (isdone ? 65535 : (waitduration - (curtime - waitstarttime)));}
    inline bool cmdIsChipWrite() {return iswrite && !isdone;}
    inline ChipWriteCmd cmdGetChipWrite() {return writecmd;}
    
    inline void setOPN2FreqMultiplier(u32 mult) {opn2mult = mult;} // Ratio of VGM to actual OPN2 clock, times 0x1000
    
    
private:
    VgmSource* source;
    u32 srcaddr;
    u32 srcblockaddr;
    
    bool iswait, iswrite;
    u32 waitduration, waitstarttime;
    ChipWriteCmd writecmd;
    
    bool isdacwrite, isfreqwrite;
    
    bool isdone;
    u32 delay62, delay63;
    
    u32 opn2mult;
};

#endif /* _VGMHEAD_H */
