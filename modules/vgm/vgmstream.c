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


u8 VGM_HeadStream_bufferNextCommand(VgmHead* head, VgmHeadStream* vhs, VgmSourceStream* vss){
    if(vhs->subbufferlen == VGM_HEADSTREAM_SUBBUFFER_MAXLEN) return 0;
    u8 type = VGM_HeadStream_getByte(vss, vhs, head->srcaddr);
    u8 len = VGM_Cmd_GetCmdLen(type);
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
        cmdlen = VGM_Cmd_GetCmdLen(type);
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
    vss->filepath = NULL;
    vss->datalen = 0;
    vss->vgmdatastartaddr = 0;
    vss->block = NULL;
    vss->blocklen = 0;
    return source;
}
void VGM_SourceStream_Delete(void* sourcestream){
    VgmSourceStream* vss = (VgmSourceStream*)sourcestream;
    if(vss->filepath != NULL){
        vgmh2_free(vss->filepath);
    }
    if(vss->block != NULL){
        free(vss->block);
    }
    vgmh2_free(vss);
}

void VGM_SourceStream_UpdateUsage(VgmSource* source){
    //This doesn't rescan the file--we assume if we haven't changed the file,
    //the usage hasn't changed!
}



