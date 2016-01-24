/*
 * MIDIbox Genesis Tracker: VGM Source from RAM Class
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMSOURCERAM_H
#define _VGMSOURCERAM_H

#include "vgmsource.h"

class VgmSourceRam : public VgmSource {
public:
    VgmSourceRam();
    virtual ~VgmSourceRam();
    
    virtual inline u8 getByte(u32 addr) { return ((addr < datalen) ? (data[addr]) : 0); }
    virtual inline u32 getSize() { return datalen; }
    
    virtual inline u8 getBlockByte(u32 blockaddr) { return 0; }
    virtual inline u32 getBlockSize() { return 0; }
    virtual inline void loadBlock(u32 startaddr, u32 len) { return; }
    
    bool loadFromSDCard(char* filename);
    
private:
    u8* data;
    u32 datalen;
};


#endif /* _VGMSOURCERAM_H */
