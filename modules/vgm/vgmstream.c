/*
 * VGM Data and Playback Driver: VGM Streaming Data
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmstream.h"
#include "vgmsdtask.h"
#include "vgmperfmon.h"
#include "vgmplayer.h"
#include "vgmtuning.h"
#include "vgm_heap2.h"
#include <genesis.h>


u8 VGM_HeadStream_getCommandLen(u8 type){
    if((type & 0xFE) == 0x52){
        //OPN2 write
        return 2;
    }else if(type == 0x50){
        //PSG write
        return 1;
    }else if(type >= 0x70 && type <= 0x8F){
        //Short wait or OPN2 DAC write
        return 0;
    }else if(type == 0x61){
        //Long wait
        return 2;
    }else if(type == 0x64){
        //Override wait lengths
        return 3;
    }else if(type >= 0x62 && type <= 0x66){
        //60 Hz wait, 50 Hz wait, Nop [unofficial], End of data
        return 0;
    }else if(type == 0x67){
        //Data block
        return 6;
    }else if(type == 0xE0){
        //Seek in data block
        return 4;
    }else if(type == 0x90){
        //Setup Stream Control not supported
        return 4;
    }else if(type == 0x91){
        //Set Stream Data not supported
        return 4;
    }else if(type == 0x92){
        //Set Stream Frequency not supported
        return 5;
    }else if(type == 0x93){
        //Start Stream not supported
        return 10;
    }else if(type == 0x94){
        //Stop Stream not supported
        return 1;
    }else if(type == 0x95){
        //Start Stream fast not supported
        return 4;
    }else if(type == 0x68){
        //PCM RAM write, not supported
        return 11;
    }else if(type >= 0x30 && type <= 0x3F){
        //Single-byte command
        return 1;
    }else if((type >= 0x40 && type <= 0x4E) || (type >= 0xA0 && type <= 0xBF)){
        //Double-byte command
        return 2;
    }else if(type >= 0xC0 && type <= 0xDF){
        //Triple-byte command
        return 3;
    }else if(type >= 0xE1 && type <= 0xFF){
        //Quadruple-byte command
        return 4;
    }else{
        return 0;
    }
}
u8 VGM_HeadStream_bufferNextCommand(VgmHead* head, VgmHeadStream* vhs, VgmSourceStream* vss){
    if(vhs->subbufferlen == VGM_HEADSTREAM_SUBBUFFER_MAXLEN) return 0;
    u8 type = VGM_HeadStream_getByte(vss, vhs, head->srcaddr);
    u8 len = VGM_HeadStream_getCommandLen(type);
    if(len + 1 + vhs->subbufferlen <= VGM_HEADSTREAM_SUBBUFFER_MAXLEN){
        ++head->srcaddr;
        vhs->subbuffer[vhs->subbufferlen++] = type;
        u8 i;
        for(i=0; i<len; ++i){
            vhs->subbuffer[vhs->subbufferlen++] = VGM_HeadStream_getByte(vss, vhs, head->srcaddr++);
        }
        return 1;
    }else{
        return 0;
    }
}
void VGM_HeadStream_unBuffer(VgmHeadStream* vhs, u8 len){
    if(len > vhs->subbufferlen) len = vhs->subbufferlen;
    vhs->subbufferlen -= len;
    u8 i;
    for(i=0; i<vhs->subbufferlen; i++){
        vhs->subbuffer[i] = vhs->subbuffer[i+len];
    }
}

VgmHeadStream* VGM_HeadStream_Create(VgmSource* source){
    VgmHeadStream* vhs = vgmh2_malloc(sizeof(VgmHeadStream));
    vhs->srcblockaddr = 0;
    vhs->subbufferlen = 0;
    vhs->buffer1 = malloc(VGM_SOURCESTREAM_BUFSIZE); //Buffers accessed using DMA,
    vhs->buffer2 = malloc(VGM_SOURCESTREAM_BUFSIZE); //have to use normal malloc
    vhs->buffer1addr = 0xFFFFFFFF;
    vhs->buffer2addr = 0xFFFFFFFF;
    vhs->wantbuffer = 0;
    vhs->wantbufferaddr = 0;
    return vhs;
}
void VGM_HeadStream_Delete(void* headstream){
    VgmHeadStream* vhs = (VgmHeadStream*)headstream;
    free(vhs->buffer1);
    free(vhs->buffer2);
    vgmh2_free(vhs);
}
void VGM_HeadStream_Restart(VgmHead* head){
    VgmHeadStream* vhs = (VgmHeadStream*)head->data;
    VgmSourceStream* vss = (VgmSourceStream*)head->source->data;
    head->srcaddr = (head->source->markstart < vss->vgmdatastartaddr) 
            ? vss->vgmdatastartaddr : head->source->markstart;
    vhs->srcblockaddr = 0;
    vhs->buffer1addr = 0xFFFFFFFF;
    vhs->buffer2addr = 0xFFFFFFFF;
    vhs->wantbufferaddr = head->srcaddr;
    vhs->wantbuffer = 1;
    DBG("HeadStream_Restart srcaddr=%d", head->srcaddr);
    VGM_HeadStream_cmdNext(head, VGM_Player_GetVGMTime());
}
u8 VGM_HeadStream_cmdNext(VgmHead* head, u32 vgm_time){
    VgmHeadStream* vhs = (VgmHeadStream*)head->data;
    VgmSourceStream* vss = (VgmSourceStream*)head->source->data;
    u8 type, cmdlen;
    head->iswait = head->iswrite = 0;
    u8 dontunbuffer;
    while(!(head->iswait || head->iswrite || head->isdone)){
        //Check if we will have to skip a data block (buffer miss)
        if(vhs->subbufferlen != 0 && vhs->subbuffer[0] == 0x67){ //There's a command buffered, and it's data block
            //Load the block parameters and skip
            u32 l = vhs->subbuffer[3] | ((u32)vhs->subbuffer[4] << 8) 
                | ((u32)vhs->subbuffer[5] << 16) | ((u32)vhs->subbuffer[6] << 24);
            head->srcaddr += l;
            VGM_HeadStream_unBuffer(vhs, 7); //Remove this command from subbuffer
            //Set up the stream where the block ends
            vhs->wantbufferaddr = head->srcaddr;
            vhs->wantbuffer = 1;
            head->iswait = 1; //Act as a wait for 0 (or negative) time
            return 0; //Report that the command couldn't be loaded
        }else if(vhs->subbufferlen == 0){
            //Check if we're about to run out of buffer
            if(head->srcaddr >= vhs->buffer1addr && head->srcaddr < (vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE)){
                //We're in buffer1
                if((vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE - head->srcaddr) < VGM_HEADSTREAM_SUBBUFFER_MAXLEN 
                        && vhs->buffer2addr != (vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE)){
                    //About to run out of buffer1, and buffer2 isn't ready
                    vhs->wantbufferaddr = (vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE);
                    vhs->wantbuffer = 2; //in case you didn't know already
                    head->iswait = 1; //Act as a wait for 0 (or negative) time
                    return 0; //Report that the command couldn't be loaded
                }
            }else if(head->srcaddr >= vhs->buffer2addr && head->srcaddr < (vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE)){
                //We're in buffer2
                if((vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE - head->srcaddr) < VGM_HEADSTREAM_SUBBUFFER_MAXLEN 
                        && vhs->buffer1addr != (vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE)){
                    //About to run out of buffer2, and buffer1 isn't ready
                    vhs->wantbufferaddr = (vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE);
                    vhs->wantbuffer = 1; //in case you didn't know already
                    head->iswait = 1; //Act as a wait for 0 (or negative) time
                    return 0; //Report that the command couldn't be loaded
                }
            }else{
                //We're not in either buffer
                vhs->wantbufferaddr = head->srcaddr;
                vhs->wantbuffer = 1; //in case you didn't know already
                head->iswait = 1; //Act as a wait for 0 (or negative) time
                return 0; //Report that the command couldn't be loaded
            }
        }
        if(head->srcaddr > head->source->markend){
            head->isdone = 1;
            break;
        }
        if(vhs->subbufferlen == 0){
            VGM_HeadStream_bufferNextCommand(head, vhs, vss); //Should never return 0
        }
        type = vhs->subbuffer[0];
        cmdlen = VGM_HeadStream_getCommandLen(type);
        dontunbuffer = 0;
        if(type == 0x50){
            //PSG write
            head->iswrite = 1;
            head->writecmd.cmd = 0x00;
            head->writecmd.data = vhs->subbuffer[1];
            if((head->writecmd.data & 0x80) && !(head->writecmd.data & 0x10) && (head->writecmd.data < 0xE0)){
                //It's a main write, not attenuation, and not noise
                u8 bufferpos, newtype;
                bufferpos = vhs->subbufferlen;
                while(VGM_HeadStream_bufferNextCommand(head, vhs, vss)){
                    newtype = vhs->subbuffer[bufferpos];
                    if(newtype == type){
                        //Next command is another PSG write
                        head->writecmd.data2 = vhs->subbuffer[bufferpos+1]; //Load command
                        if(!(head->writecmd.data2 & 0x80)){
                            //Second command is a frequency MSB write
                            VGM_fixPSGFrequency(&(head->writecmd), head->psgmult, head->psgfreq0to1);
                            //Reconstruct next command
                            vhs->subbuffer[bufferpos+1] = head->writecmd.data2;
                        }
                        //If it's another main write, don't modify anything, and stop
                        break;
                    }
                    bufferpos = vhs->subbufferlen;
                }
            }
            VGM_Head_doMapping(head, &(head->writecmd));
        }else if((type & 0xFE) == 0x52){
            //OPN2 write
            head->iswrite = 1;
            head->writecmd.cmd = (type & 0x01) | 0x02;
            head->writecmd.addr = vhs->subbuffer[1];
            head->writecmd.data = vhs->subbuffer[2];
            if((head->writecmd.addr & 0xF4) == 0xA4){
                //Frequency MSB write, read to find frequency LSB write command
                u8 bufferpos, newtype;
                bufferpos = vhs->subbufferlen;
                while(VGM_HeadStream_bufferNextCommand(head, vhs, vss)){
                    newtype = vhs->subbuffer[bufferpos];
                    if(newtype == type){
                        //Next command is another OPN2 write to the same addrhi
                        if((head->writecmd.addr & 0xFB) == (vhs->subbuffer[bufferpos+1] & 0xFB)){
                            //Second command is a frequency write to same channel
                            if(!(vhs->subbuffer[bufferpos+1] & 0x04)){
                                //It's a frequency LSB write
                                head->writecmd.data2 = vhs->subbuffer[bufferpos+2];
                                VGM_fixOPN2Frequency(&(head->writecmd), head->opn2mult);
                                //Reconstruct next command
                                vhs->subbuffer[bufferpos+2] = head->writecmd.data2;
                            }
                            //If it's a frequency MSB command, don't modify anything, and stop
                            break;
                        }
                    }
                    bufferpos = vhs->subbufferlen;
                }
            }
            VGM_Head_doMapping(head, &(head->writecmd));
        }else if(type >= 0x80 && type <= 0x8F){
            //OPN2 DAC write
            head->iswrite = 1;
            head->writecmd.cmd = 0x02;
            head->writecmd.addr = 0x2A;
            head->writecmd.data = VGM_SourceStream_getBlockByte(vss, vhs->srcblockaddr++);
            if(type != 0x80){
                //Replace the command with the equivalent wait command
                vhs->subbuffer[0] = type - 0x11;
                dontunbuffer = 1;
            }
            VGM_Head_doMapping(head, &(head->writecmd));
        }else if(type >= 0x70 && type <= 0x7F){
            //Short wait
            head->iswait = 1;
            head->ticks += type - 0x6F;
        }else if(type == 0x61){
            //Long wait
            head->iswait = 1;
            head->ticks += vhs->subbuffer[1] | ((u32)vhs->subbuffer[2] << 8);
        }else if(type == 0x62){
            //60 Hz wait
            head->iswait = 1;
            head->ticks += VGM_DELAY62;
        }else if(type == 0x63){
            //50 Hz wait
            head->iswait = 1;
            head->ticks += VGM_DELAY63;
        }else if(type == 0x64){
            //Override wait lengths; now unsupported
            /*
            u8 tooverride = subbuffer[1];
            u32 newdelay = subbuffer[2] | ((u32)subbuffer[3] << 8);
            if(tooverride == 0x62){
                delay62 = newdelay;
            }else if(tooverride == 0x63){
                delay63 = newdelay;
            }
            */
        }else if(type == 0x65){
            //Nop [unofficial]
        }else if(type == 0x66){
            //End of data
            if(head->source->loopaddr >= vss->vgmdatastartaddr && head->source->loopaddr < vss->datalen){
                //Jump to loop point
                head->srcaddr = head->source->loopaddr;
                VGM_HeadStream_unBuffer(vhs, 1); //Remove this command from subbuffer
                //Set up to stream from here
                vhs->wantbufferaddr = head->srcaddr;
                vhs->wantbuffer = 1;
                head->iswait = 1; //Act as a wait for 0 (or negative) time
                return 0; //Report that the command couldn't be loaded
            }else{
                //Behaves like endless stream of 65535-tick waits
                head->isdone = 1;
            }
        }else if(type == 0x67){
            //Data block
            //Skip 0x66 in subbuffer[1]
            //a = vhs->subbuffer[2]; //== 0, format other than uncompressed YM2612 PCM not supported
            u32 l = vhs->subbuffer[3] | ((u32)vhs->subbuffer[4] << 8) 
                    | ((u32)vhs->subbuffer[5] << 16) | ((u32)vhs->subbuffer[6] << 24);
            head->srcaddr += l;
            VGM_HeadStream_unBuffer(vhs, 7); //Remove this command from subbuffer
            //Set up the stream where the block ends
            vhs->wantbufferaddr = head->srcaddr;
            vhs->wantbuffer = 1;
            head->iswait = 1; //Act as a wait for 0 (or negative) time
            return 0; //Report that the command couldn't be loaded
            //}
        }else if(type == 0xE0){
            //Seek in data block
            vhs->srcblockaddr = vhs->subbuffer[1] | ((u32)vhs->subbuffer[2] << 8) 
                | ((u32)vhs->subbuffer[3] << 16) | ((u32)vhs->subbuffer[4] << 24);
        }
        if(!dontunbuffer){
            VGM_HeadStream_unBuffer(vhs, cmdlen+1);
        }
    }
    return 1;
}
u8 VGM_HeadStream_getByte(VgmSourceStream* vss, VgmHeadStream* vhs, u32 addr){
    if(addr >= vss->datalen) return 0;
    if(addr >= vhs->buffer1addr && addr < (vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE)){
        if(vhs->buffer2addr != vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            vhs->wantbuffer = 2;
            vhs->wantbufferaddr = vhs->buffer1addr + VGM_SOURCESTREAM_BUFSIZE;
        }
        return vhs->buffer1[addr - vhs->buffer1addr];
    }
    if(addr >= vhs->buffer2addr && addr < (vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE)){
        if(vhs->buffer1addr != vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            vhs->wantbuffer = 1;
            vhs->wantbufferaddr = vhs->buffer2addr + VGM_SOURCESTREAM_BUFSIZE;
        }
        return vhs->buffer2[addr - vhs->buffer2addr];
    }
    /*
    //Have to load something right now
    u8 leds = MIOS32_BOARD_LED_Get();
    MIOS32_BOARD_LED_Set(0b1111, 0b0100);
    s32 res = FILE_ReadReOpen(&vss->file);
    if(res < 0) return 0;
    res = FILE_ReadSeek(addr);
    if(res < 0) return 0;
    res = FILE_ReadBuffer(vhs->buffer1, VGM_SOURCESTREAM_BUFSIZE);
    if(res < 0) return 0;
    vhs->buffer1addr = addr;
    FILE_ReadClose(&vss->file);
    //Set up to load the next buffer next
    vhs->wantbuffer = 2;
    vhs->wantbufferaddr = addr + VGM_SOURCESTREAM_BUFSIZE;
    //Done
    MIOS32_BOARD_LED_Set(0b1111, leds);
    return vhs->buffer1[0];
    */
    DBG("VGM_HeadStream_getByte() buffer underflow!");
    return 0x66; //error, stop stream
}
void VGM_HeadStream_BackgroundBuffer(VgmHead* head){
    VgmHeadStream* vhs = (VgmHeadStream*)head->data;
    VgmSourceStream* vss = (VgmSourceStream*)head->source->data;
    u8* bufferto = NULL;
    u32* addrto = NULL;
    if(vhs->wantbuffer == 1){
        bufferto = vhs->buffer1;
        addrto = &(vhs->buffer1addr);
    }else if(vhs->wantbuffer == 2){
        bufferto = vhs->buffer2;
        addrto = &(vhs->buffer2addr);
    }
    if(bufferto != NULL){
        u8 leds = MIOS32_BOARD_LED_Get();
        MIOS32_BOARD_LED_Set(0b1111, 0b0100);
        VGM_PerfMon_ClockIn(VGM_PERFMON_TASK_CARD);
        vgm_sdtask_usingsdcard = 1;
        MUTEX_SDCARD_TAKE;
        FILE_ReadReOpen(&vss->file);
        FILE_ReadSeek(vhs->wantbufferaddr);
        FILE_ReadBuffer(bufferto, VGM_SOURCESTREAM_BUFSIZE);
        FILE_ReadClose(&vss->file);
        MUTEX_SDCARD_GIVE;
        vgm_sdtask_usingsdcard = 0;
        VGM_PerfMon_ClockOut(VGM_PERFMON_TASK_CARD);
        MIOS32_BOARD_LED_Set(0b1111, leds);
        vhs->wantbuffer = 0;
        *addrto = vhs->wantbufferaddr;
    }
}

VgmSource* VGM_SourceStream_Create(){
    VgmSource* source = vgmh2_malloc(sizeof(VgmSource));
    source->type = VGM_SOURCE_TYPE_STREAM;
    source->mutes = 0;
    source->opn2clock = genesis_clock_opn2;
    source->psgclock = genesis_clock_psg;
    source->psgfreq0to1 = 1;
    source->loopaddr = 0xFFFFFFFF;
    source->loopsamples = 0;
    source->markstart = 0;
    source->markend = 0xFFFFFFFF;
    source->usage.all = 0;
    VgmSourceStream* vss = vgmh2_malloc(sizeof(VgmSourceStream));
    source->data = vss;
    vss->datalen = 0;
    vss->vgmdatastartaddr = 0;
    vss->block = NULL;
    vss->blocklen = 0;
    return source;
}
void VGM_SourceStream_Delete(void* sourcestream){
    VgmSourceStream* vss = (VgmSourceStream*)sourcestream;
    if(vss->block != NULL){
        free(vss->block);
    }
    vgmh2_free(vss);
}


static void BufferRead(u8* cmdbuf, u32 bytes, u32* a, u32* bufstart, u8* buf){
    u32 b = 0;
    while(1){
        while(*a - *bufstart < VGM_SOURCESTREAM_BUFSIZE){
            cmdbuf[b++] = buf[(*a)++ - *bufstart];
            if(b >= bytes) return;
        }
        FILE_ReadSeek(*a);
        FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
        *bufstart = *a;
    }
}
static void BufferSkip(u32 bytes, u32* a, u32* bufstart, u8* buf){
    *a += bytes;
    if(*a - *bufstart >= VGM_SOURCESTREAM_BUFSIZE){
        FILE_ReadSeek(*a);
        FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE); 
        *bufstart = *a;
    }
}
static inline u32 ReadLittleEndianU32(u8* buf, u32 addr){
    return (u32)buf[addr+0] 
        | ((u32)buf[addr+1] << 8) 
        | ((u32)buf[addr+2] << 16) 
        | ((u32)buf[addr+3] << 24);
}
s32 VGM_ScanFile(char* filename, VgmFileMetadata* md){
    //Init
    md->filesize = 0;
    md->numcmds = 0;
    md->numcmdsram = 0;
    md->numblocks = 0;
    md->totalblocksize = 0;
    md->usage.all = 0;
    md->vgmdatastartaddr = 0;
    md->loopaddr = 0;
    md->loopsamples = 0;
    md->psgclock = 0;
    md->psgfreq0to1 = 1;
    md->opn2clock = 0;
    //Open file
    MUTEX_SDCARD_TAKE;
    s32 res = FILE_ReadOpen(&md->file, filename);
    if(res < 0) goto Error_Opening;
    md->filesize = FILE_ReadGetCurrentSize();
    if(md->filesize <= 0x40){
        DBG("File too small to have VGM header!");
        goto Error_Filesize;
    }
    u8* buf = malloc(0x40); //DMA target, have to use normal malloc
    res = FILE_ReadBuffer(buf, 0x40);
    if(res < 0) goto Error_withBufferOpen;
    if(buf[0] != 'V' || buf[1] != 'g' || buf[2] != 'm' || buf[3] != ' '){
        DBG("File doesn't have magic \"Vgm \" tag!");
        goto Error_withBufferOpen;
    }
    //Read header data
    u8 ver_lo = buf[8];
    u8 ver_hi = buf[9];
    md->psgclock = ReadLittleEndianU32(buf, 0x0C);
    md->loopaddr = ReadLittleEndianU32(buf, 0x1C) + 0x1C;
    md->loopsamples = ReadLittleEndianU32(buf, 0x20);
    md->opn2clock = ReadLittleEndianU32(buf, 0x2C);
    md->psgfreq0to1 = (~buf[0x2B]) & 1;
    if(md->opn2clock == 0) md->psgfreq0to1 = 0; //If there's no OPN2, it's probably a SMS
    u32 a;
    if(ver_hi < 1 || (ver_hi == 1 && ver_lo < 0x50)){
        a = 0x40;
    }else{
        a = ReadLittleEndianU32(buf, 0x34) + 0x34;
    }
    md->vgmdatastartaddr = a;
    free(buf);
    //Scan entire file for data blocks and usage
    u32 bufstart = a;
    u8 type, len;
    u32 thisblocksize;
    u8 cmdbuf[4];
    VgmChipWriteCmd cmd;
    buf = malloc(VGM_SOURCESTREAM_BUFSIZE); //DMA target, have to use normal malloc
    FILE_ReadSeek(a); FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
    while(a < md->filesize){
        BufferRead(&type, 1, &a, &bufstart, buf);
        ++md->numcmds;
        if(type == 0x67){
            //Data block
            ++md->numblocks;
            BufferSkip(2, &a, &bufstart, buf); //Skip 0x66 0x00
            BufferRead(cmdbuf, 4, &a, &bufstart, buf);
            thisblocksize = ReadLittleEndianU32(cmdbuf, 0);
            //DBG("Block size is %02X %02X %02X %02X or %d", cmdbuf[0], cmdbuf[1], cmdbuf[2], cmdbuf[3], thisblocksize);
            md->totalblocksize += thisblocksize;
            BufferSkip(thisblocksize, &a, &bufstart, buf);
        }else if(type == 0x66){
            //End of stream
            break;
        }else if(type == 0x50){
            //PSG write
            BufferRead(cmdbuf, 1, &a, &bufstart, buf);
            cmd.cmd = type;
            cmd.data = cmdbuf[0];
            VGM_Cmd_UpdateUsage(&md->usage, cmd);
            if(cmd.data & 0x80) ++md->numcmdsram; //Don't count second halves of freq writes
        }else if((type & 0xFE) == 0x52){
            //OPN2 write
            BufferRead(cmdbuf, 2, &a, &bufstart, buf);
            cmd.cmd = type;
            cmd.addr = cmdbuf[0];
            cmd.data = cmdbuf[1];
            VGM_Cmd_UpdateUsage(&md->usage, cmd);
            if((cmd.addr & 0xF4) != 0xA0) ++md->numcmdsram; //Don't count second halves of freq writes
        }else if(type >= 0x80 && type <= 0x8F){
            //DAC and wait
            cmd.cmd = type;
            VGM_Cmd_UpdateUsage(&md->usage, cmd);
            ++md->numcmdsram;
        }else if((type >= 0x70 && type <= 0x7F) || type == 0x62 || type == 0x63){
            //Short wait, 60 Hz wait, or 50 Hz wait
            ++md->numcmdsram;
        }else if(type == 0x61){
            //Long wait
            BufferSkip(2, &a, &bufstart, buf);
            ++md->numcmdsram;
        }else{
            len = VGM_HeadStream_getCommandLen(type);
            BufferSkip(len, &a, &bufstart, buf);
        }
    }
    if(type != 0x66){
        DBG("VGM_ScanFile ran off end of file!");
        res = -51;
    }
Error_withBufferOpen:
    free(buf);
Error_Filesize:
    FILE_ReadClose(&md->file);
Error_Opening:
    MUTEX_SDCARD_GIVE;
    if(res < 0){
        DBG("VGM_ScanFile error %d on %s", res, filename);
    }else{
        DBG("VGM_ScanFile successful on %s (%d bytes):", filename, md->filesize);
        DBG("--%d cmds, %d cmds for RAM, %d blocks, %d total block size", 
            md->numcmds, md->numcmdsram, md->numblocks, md->totalblocksize);
        DBG("--VGM data starts at 0x%X, loop at 0x%X, loop %d samples",
            md->vgmdatastartaddr, md->loopaddr, md->loopsamples);
        DBG("--PSG clock %d, OPN2 clock %d", md->psgclock, md->opn2clock);
        VGM_Cmd_DebugPrintUsage(md->usage);
    }
    return res;
}
s32 VGM_SourceStream_Start(VgmSource* source, VgmFileMetadata* md){
    VgmSourceStream* vss = (VgmSourceStream*)source->data;
    //Copy data from metadata to source/sourcestream
    source->psgclock = md->psgclock;
    source->opn2clock = md->opn2clock;
    source->psgfreq0to1 = md->psgfreq0to1;
    source->loopaddr = md->loopaddr;
    source->loopsamples = md->loopsamples;
    source->usage.all = md->usage.all;
    vss->file = md->file;
    vss->datalen = md->filesize;
    vss->vgmdatastartaddr = md->vgmdatastartaddr;
    vss->blocklen = md->totalblocksize;
    if(!vss->blocklen) return 0; //No blocks, done!
    //Allocate memory for block
    vss->block = malloc(vss->blocklen);
    if(vss->block == NULL) return -50;
    //Open file
    MUTEX_SDCARD_TAKE;
    s32 res = FILE_ReadReOpen(&vss->file);
    if(res < 0) goto Error_Opening;
    //Load data blocks from file
    u32 blockaddr = 0;
    u32 a = vss->vgmdatastartaddr;
    u32 bufstart = a;
    u16 blockcount = 0;
    u8 type, len;
    u32 thisblocksize;
    u8 cmdbuf[4];
    u8* buf = malloc(VGM_SOURCESTREAM_BUFSIZE); //DMA target, have to use normal malloc
    FILE_ReadSeek(a); FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
    while(a < md->filesize){
        BufferRead(&type, 1, &a, &bufstart, buf);
        if(type == 0x67){
            //Data block
            BufferSkip(2, &a, &bufstart, buf); //Skip 0x66 0x00
            BufferRead(cmdbuf, 4, &a, &bufstart, buf);
            thisblocksize = ReadLittleEndianU32(cmdbuf, 0);
            //Load actual block
            FILE_ReadSeek(a);
            FILE_ReadBuffer((u8*)(vss->block + blockaddr), thisblocksize);
            blockaddr += thisblocksize;
            //See if we're done
            ++blockcount;
            if(blockcount >= md->numblocks) break;
            //Restore our buffer
            a += thisblocksize;
            FILE_ReadSeek(a);
            FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
            bufstart = a;
        }else if(type == 0x66){
            //End of stream
            break;
        }else{
            len = VGM_HeadStream_getCommandLen(type);
            BufferSkip(len, &a, &bufstart, buf);
        }
    }
    if(blockcount < md->numblocks){
        DBG("VGM_SourceStream_Start error: couldn't find enough blocks!");
        res = -51;
    }else if(blockaddr < md->totalblocksize){
        DBG("VGM_SourceStream_Start error: loaded blocks not long enough!");
        res = -52;
    }
    DBG("Loaded %d of %d block bytes", blockaddr, vss->blocklen);
    free(buf);
    FILE_ReadClose(&vss->file);
Error_Opening:
    MUTEX_SDCARD_GIVE;
    return res;
}

#if 0
s32 VGM_SourceStream_Start(VgmSource* source, char* filename){
    VgmSourceStream* vss = (VgmSourceStream*)source->data;
    MUTEX_SDCARD_TAKE;
    s32 res = FILE_ReadOpen(&vss->file, filename);
    if(res < 0) { MUTEX_SDCARD_GIVE; return res; }
    vss->datalen = FILE_ReadGetCurrentSize();
    //Read header
    u8 flag = 0;
    if(vss->datalen > 0x40){
        u8* hdrdata = malloc(0x40); //DMA target, have to use normal malloc
        res = FILE_ReadBuffer(hdrdata, 0x40);
        if(res < 0) { free(hdrdata); vss->datalen = 0; MUTEX_SDCARD_GIVE; return -2; }
        //Check if has VGM header or raw data
        if(hdrdata[0] == 'V' && hdrdata[1] == 'g' && hdrdata[2] == 'm' && hdrdata[3] == ' '){
            flag = 1;
            //Get version
            u8 ver_lo = hdrdata[8];
            u8 ver_hi = hdrdata[9];
            source->psgclock = ((u32)hdrdata[0x0C]) 
                    | ((u32)hdrdata[0x0D] << 8)
                    | ((u32)hdrdata[0x0E] << 16) 
                    | ((u32)hdrdata[0x0F] << 24);
            source->loopaddr = (((u32)hdrdata[0x1C]) 
                    | ((u32)hdrdata[0x1D] << 8)
                    | ((u32)hdrdata[0x1E] << 16) 
                    | ((u32)hdrdata[0x1F] << 24))
                    + 0x1C;
            source->loopsamples = ((u32)hdrdata[0x20]) 
                    | ((u32)hdrdata[0x21] << 8)
                    | ((u32)hdrdata[0x22] << 16) 
                    | ((u32)hdrdata[0x23] << 24);
            source->opn2clock = ((u32)hdrdata[0x2C]) 
                    | ((u32)hdrdata[0x2D] << 8)
                    | ((u32)hdrdata[0x2E] << 16) 
                    | ((u32)hdrdata[0x2F] << 24);
            if(ver_hi < 1 || (ver_hi == 1 && ver_lo < 0x50)){
                vss->vgmdatastartaddr = 0x40;
            }else{
                vss->vgmdatastartaddr = (((u32)hdrdata[0x34]) 
                        | ((u32)hdrdata[0x35] << 8)
                        | ((u32)hdrdata[0x36] << 16) 
                        | ((u32)hdrdata[0x37] << 24))
                        + 0x34;
            }
            DBG("VGM data starts at 0x%X", vss->vgmdatastartaddr);
        }
        free(hdrdata);
    }
    if(!flag){
        DBG("VGM data starts at 0");
        vss->vgmdatastartaddr = 0;
        source->opn2clock = 7670454;
        source->psgclock = 3579545;
        source->loopaddr = 0xFFFFFFFF;
        source->loopsamples = 0;
    }
    //Scan entire file for data blocks
    u32 totalblocksize = 0;
    u32 a = vss->vgmdatastartaddr;
    u32 bufstart = a;
    u8 type;
    u8 blockcount = 0;
    u32 thisblocksize;
    u8* buf = malloc(VGM_SOURCESTREAM_BUFSIZE); //DMA target, have to use normal malloc
    FILE_ReadSeek(a); FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
    while(1){
        type = buf[a - bufstart];
        if(type == 0x67){
            //Data block
            ++blockcount;
            a += 3; /*Skip 0x66 0x00*/ CheckAdvanceBuffer(a, &bufstart, buf);
            thisblocksize = (u32)buf[a-bufstart];
            ++a; CheckAdvanceBuffer(a, &bufstart, buf);
            thisblocksize |= (u32)buf[a-bufstart] << 8;
            ++a; CheckAdvanceBuffer(a, &bufstart, buf);
            thisblocksize |= (u32)buf[a-bufstart] << 16;
            ++a; CheckAdvanceBuffer(a, &bufstart, buf);
            thisblocksize |= (u32)buf[a-bufstart] << 24;
            totalblocksize += thisblocksize;
            #ifndef VGM_STREAM_SUPPORTMULTIBLOCK
                vss->block = malloc(totalblocksize); //DMA target, have to use normal malloc
                vss->blocklen = totalblocksize;
                FILE_ReadSeek(a+1);
                FILE_ReadBuffer(vss->block, totalblocksize);
                DBG("Loaded block 0x%X bytes", totalblocksize);
                break;
            #else
                a += 1 + thisblocksize;
                CheckAdvanceBuffer(a, &bufstart, buf);
            #endif
        }else if(type == 0x66){
            //End of stream
            break;
        }else{
            a += 1 + VGM_HeadStream_getCommandLen(type);
            CheckAdvanceBuffer(a, &bufstart, buf);
        }
    }
    #ifdef VGM_STREAM_SUPPORTMULTIBLOCK
        DBG("Found %d blocks, total size 0x%X", blockcount, totalblocksize);
        //Load blocks
        vss->block = malloc(totalblocksize); //DMA target, have to use normal malloc
        vss->blocklen = totalblocksize;
        u32 blockaddr = 0;
        a = vss->vgmdatastartaddr;
        bufstart = a;
        FILE_ReadSeek(a); FILE_ReadBuffer(buf, VGM_SOURCESTREAM_BUFSIZE);
        while(1){
            type = buf[a - bufstart];
            if(type == 0x67){
                //Data block
                a += 3; /*Skip 0x66 0x00*/ CheckAdvanceBuffer(a, &bufstart, buf);
                thisblocksize = (u32)buf[a-bufstart];
                ++a; CheckAdvanceBuffer(a, &bufstart, buf);
                thisblocksize |= (u32)buf[a-bufstart] << 8;
                ++a; CheckAdvanceBuffer(a, &bufstart, buf);
                thisblocksize |= (u32)buf[a-bufstart] << 16;
                ++a; CheckAdvanceBuffer(a, &bufstart, buf);
                thisblocksize |= (u32)buf[a-bufstart] << 24;
                FILE_ReadSeek(a+1);
                FILE_ReadBuffer((u8*)(vss->block + blockaddr), thisblocksize);
                blockaddr += thisblocksize;
                a += 1 + thisblocksize;
                CheckAdvanceBuffer(a, &bufstart, buf);
            }else if(type == 0x66){
                //End of stream
                break;
            }else{
                a += 1 + VGM_HeadStream_getCommandLen(type);
                CheckAdvanceBuffer(a, &bufstart, buf);
            }
        }
        DBG("Loaded 0x%X block bytes", blockaddr);
    #endif
    //Clean up
    FILE_ReadClose(&vss->file);
    free(buf);
    MUTEX_SDCARD_GIVE;
    return 0;
}
#endif

void VGM_SourceStream_UpdateUsage(VgmSource* source){
    //This doesn't rescan the file--we assume if we haven't changed the file,
    //the usage hasn't changed!
}



