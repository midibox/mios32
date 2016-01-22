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

class VgmSource {
public:
    virtual VgmSource();
    virtual ~VgmSource();
    
    virtual u8 getByte(u32 addr);
    virtual u32 getSize();
    
    virtual u8 getBlockByte(u32 blockaddr);
    virtual u32 getBlockSize();
    virtual void loadBlock(u32 startaddr, u32 len);
    
    void readHeader();
    u32 vgmdatastartaddr;
    u32 opn2clock;
    u32 psgclock;
    u32 loopaddr;
    u32 loopsamples;
    
private:
};


#endif /* _VGMSOURCE_H */
