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

#include <mios32.h>
#include "vgmsourceram.h"

VgmSourceRam::VgmSourceRam() {
    data = nullptr;
    datalen = 0;
}
VgmSourceRam::~VgmSourceRam(){
    if(data != nullptr){
        delete[] data;
    }
}

bool VgmSourceRam::loadFromSDCard(char* filename){
    //TODO preprocessing
    file_t file;
    s32 res = FILE_ReadOpen(&file, filename);
    if(res < 0) return false;
    datalen = FILE_ReadGetCurrentSize();
    data = new u8[datalen];
    res = FILE_ReadBuffer(data, datalen);
    if(res < 0) return false;
    res = FILE_ReadClose(&file);
    if(res < 0) return false;
    return true;
}
