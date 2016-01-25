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
#include "app.h"


VgmSourceStream::VgmSourceStream() {
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

s32 VgmSourceStream::startStream(char* filename){
    s32 res = FILE_ReadOpen(&file, filename);
    if(res < 0) return res;
    datalen = FILE_ReadGetCurrentSize();
    //Fill both buffers
    buffer1addr = 0;
    res = FILE_ReadBuffer(buffer1, VGMSOURCESTREAM_BUFSIZE);
    if(res < 0){datalen = 0; return -2;}
    buffer2addr = VGMSOURCESTREAM_BUFSIZE;
    res = FILE_ReadBuffer(buffer2, VGMSOURCESTREAM_BUFSIZE);
    if(res < 0){datalen = 0; return -3;}
    //Close file and save for reopening
    res = FILE_ReadClose(&file);
    if(res < 0){datalen = 0; return -4;}
    return 0;
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
    u8 leds = MIOS32_BOARD_LED_Get();
    MIOS32_BOARD_LED_Set(0b1111, 0b0100);
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
    MIOS32_BOARD_LED_Set(0b1111, leds);
    return buffer1[0];
}

void VgmSourceStream::bg_streamBuffer(){
    if(wantbuffer == 1 || wantbuffer == 2){
        MIOS32_IRQ_Disable();
        u8 leds = MIOS32_BOARD_LED_Get();
        MIOS32_BOARD_LED_Set(0b1111, 0b0001);
        DEBUGVAL = 1;
        s32 res = FILE_ReadReOpen(&file);
        if(res < 0) return;
        DEBUGVAL = 2;
        res = FILE_ReadSeek(wantbufferaddr);
        if(res < 0) return;
        DEBUGVAL = 3;
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
        wantbuffer = 0;
        FILE_ReadClose(&file);
        MIOS32_BOARD_LED_Set(0b1111, leds);
        DEBUGVAL = 0;
        MIOS32_IRQ_Enable();
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
    blocklen = len;
}

void VgmSourceStream::readHeader(){
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
