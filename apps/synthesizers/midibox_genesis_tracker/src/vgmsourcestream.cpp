/*
 * MIDIbox Genesis Tracker: VGM Source Stream from SD Card Class
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
#include "vgmsourcestream.h"


VgmSourceStream::VgmSourceStream(){
    block = NULL;
    blocklen = 0;
    blockorigaddr = 0xFFFFFFFF;
    buffer1 = new u8[VGMSOURCESTREAM_BUFSIZE];
    buffer2 = new u8[VGMSOURCESTREAM_BUFSIZE];
    datalen = 0;
    wantbuffer = 0;
}

VgmSourceStream::~VgmSourceStream(){
    if(block != NULL){
        delete[] block;
    }
}

bool VgmSourceStream::startStream(char* filename){
    s32 res = FILE_ReadOpen(&file, filename);
    if(res < 0) return false;
    datalen = FILE_ReadGetCurrentSize();
    //Fill both buffers
    buffer1addr = 0;
    res = FILE_ReadBuffer(buffer1, VGMSOURCESTREAM_BUFSIZE);
    if(res < 0){datalen = 0; return false;}
    buffer2addr = VGMSOURCESTREAM_BUFSIZE;
    res = FILE_ReadBuffer(buffer2, VGMSOURCESTREAM_BUFSIZE);
    if(res < 0){datalen = 0; return false;}
    //Close file and save for reopening
    res = FILE_ReadClose(&file);
    if(res < 0){datalen = 0; return false;}
    return true;
}
    
u8 VgmSourceStream::getByte(u32 addr){
    if(addr >= datalen) return 0;
    if(addr >= buffer1addr && addr < (buffer1addr + VGMSOURCESTREAM_BUFSIZE)){
        if(buffer2addr != buffer1addr + VGMSOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            wantbuffer = 2;
            wantbufferaddr = buffer1addr + VGMSOURCESTREAM_BUFSIZE;
        }
        return buffer1[addr - buffer1addr];
    }
    if(addr >= buffer2addr && addr < (buffer2addr + VGMSOURCESTREAM_BUFSIZE)){
        if(buffer1addr != buffer2addr + VGMSOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            wantbuffer = 1;
            wantbufferaddr = buffer2addr + VGMSOURCESTREAM_BUFSIZE;
        }
        return buffer2[addr - buffer2addr];
    }
    //Have to load something right now
    s32 res = FILE_ReadReOpen(&file);
    if(res < 0) return 0;
    res = FILE_ReadSeek(addr);
    if(res < 0) return 0;
    res = FILE_ReadBuffer(buffer1, VGMSOURCESTREAM_BUFSIZE);
    if(res < 0) return 0;
    buffer1addr = addr;
    FILE_ReadClose(&file);
    //Set up to load the next buffer next
    wantbuffer = 2;
    wantbufferaddr = addr + VGMSOURCESTREAM_BUFSIZE;
    //Done
    return buffer1[0];
}

void VgmSourceStream::bg_streamBuffer(){
    if(wantbuffer == 1 || wantbuffer == 2){
        s32 res = FILE_ReadReOpen(&file);
        if(res < 0) return;
        res = FILE_ReadSeek(wantbufferaddr);
        if(res < 0) return;
        if(wantbuffer == 1){
            buffer1addr = 0xFFFF0000; //if you have a 4GB VGM that's a problem
            res = FILE_ReadBuffer(buffer1, VGMSOURCESTREAM_BUFSIZE);
            if(res < 0) return;
            buffer1addr = wantbufferaddr;
        }else{
            buffer2addr = 0xFFFF0000;
            res = FILE_ReadBuffer(buffer2, VGMSOURCESTREAM_BUFSIZE);
            if(res < 0) return;
            buffer2addr = wantbufferaddr;
        }
        FILE_ReadClose(&file);
    }
}

void VgmSourceStream::loadBlock(u32 startaddr, u32 len){
    if(startaddr == blockorigaddr && len == blocklen) return; //Don't reload existing block
    if(block != NULL){
        delete[] block;
    }
    block = new u8[len];
    s32 res = FILE_ReadReOpen(&file);
    if(res < 0) return;
    res = FILE_ReadSeek(startaddr);
    if(res < 0) return;
    res = FILE_ReadBuffer(block, len);
    if(res < 0) return;
    FILE_ReadClose(&file);
}

