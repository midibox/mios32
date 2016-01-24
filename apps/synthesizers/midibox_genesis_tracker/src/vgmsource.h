/*
 * MIDIbox Genesis Tracker: VGM Source Class
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMSOURCE_H
#define _VGMSOURCE_H

#error "Don't use this!"

class VgmSource {
public:
    VgmSource();
    virtual ~VgmSource();
    
    virtual u8 getByte(u32 addr) = 0;
    virtual u32 getSize() = 0;
    
    virtual u8 getBlockByte(u32 blockaddr) = 0;
    virtual u32 getBlockSize() = 0;
    virtual void loadBlock(u32 startaddr, u32 len) = 0;
    
    void readHeader();
    u32 vgmdatastartaddr;
    u32 opn2clock;
    u32 psgclock;
    u32 loopaddr;
    u32 loopsamples;
    
private:
};


#endif /* _VGMSOURCE_H */
