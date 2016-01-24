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

#include <mios32.h>
#include "vgmsource.h"

VgmSource::VgmSource(){
    vgmdatastartaddr = 0;
    opn2clock = 8000000;
    psgclock = 4000000;
    loopaddr = 0;
    loopsamples = 0;
}

VgmSource::~VgmSource(){
    
}

void VgmSource::readHeader(){
    if(getByte(0) == 'V' && getByte(1) == 'g' && getByte(2) == 'm' && getByte(3) == ' '){
        //File has header
        //Get version
        u8 ver_lo = getByte(8);
        u8 ver_hi = getByte(9);
        psgclock = ((u32)getByte(0x0C)) | ((u32)getByte(0x0D) << 8)
                 | ((u32)getByte(0x0E) << 16) | ((u32)getByte(0x0F) << 24);
        loopaddr = (((u32)getByte(0x1C)) | ((u32)getByte(0x1D) << 8)
                 | ((u32)getByte(0x1E) << 16) | ((u32)getByte(0x1F) << 24))
                 + 0x1C;
        loopsamples = ((u32)getByte(0x20)) | ((u32)getByte(0x21) << 8)
                 | ((u32)getByte(0x22) << 16) | ((u32)getByte(0x23) << 24);
        opn2clock = ((u32)getByte(0x2C)) | ((u32)getByte(0x2D) << 8)
                 | ((u32)getByte(0x2E) << 16) | ((u32)getByte(0x2F) << 24);
        if(ver_hi < 1 || (ver_hi == 1 && ver_lo < 0x50)){
            vgmdatastartaddr = 0x40;
        }else{
            vgmdatastartaddr = (((u32)getByte(0x34)) | ((u32)getByte(0x35) << 8)
                 | ((u32)getByte(0x36) << 16) | ((u32)getByte(0x37) << 24))
                 + 0x34;
        }
    }else{
        vgmdatastartaddr = 0;
        opn2clock = psgclock = 0;
    }
}

