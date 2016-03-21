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

#define VGM_DELAY62 735
#define VGM_DELAY63 882

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
u8 VGM_HeadStream_bufferNextCommand(VgmHeadStream* vhs, VgmSourceStream* vss){
    if(vhs->subbufferlen == VGM_HEADSTREAM_SUBBUFFER_MAXLEN) return 0;
    u8 type = VGM_SourceStream_getByte(vss, vhs->srcaddr);
    u8 len = VGM_HeadStream_getCommandLen(type);
    if(len + 1 + vhs->subbufferlen <= VGM_HEADSTREAM_SUBBUFFER_MAXLEN){
        ++vhs->srcaddr;
        vhs->subbuffer[vhs->subbufferlen++] = type;
        u8 i;
        for(i=0; i<len; ++i){
            vhs->subbuffer[vhs->subbufferlen++] = VGM_SourceStream_getByte(vss, vhs->srcaddr++);
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
    VgmHeadStream* vhs = malloc(sizeof(VgmHeadStream));
    vhs->srcaddr = 0;
    vhs->srcblockaddr = 0;
    vhs->subbufferlen = 0;
    return vhs;
}
void VGM_HeadStream_Delete(void* headstream){
    VgmHeadStream* vhs = (VgmHeadStream*)headstream;
    free(vhs);
}
void VGM_HeadStream_Restart(VgmHead* head){
    VgmHeadStream* vhs = (VgmHeadStream*)head->data;
    VgmSourceStream* vss = (VgmSourceStream*)head->source->data;
    vhs->srcaddr = vss->vgmdatastartaddr;
    vhs->srcblockaddr = 0;
}
void VGM_HeadStream_cmdNext(VgmHead* head, u32 vgm_time){
    VgmHeadStream* vhs = (VgmHeadStream*)head->data;
    VgmSourceStream* vss = (VgmSourceStream*)head->source->data;
    u8 type, cmdlen;
    head->iswait = head->iswrite = 0;
    u8 dontunbuffer;
    while(!(head->iswait || head->iswrite)){
        if(vhs->subbufferlen == 0){
            VGM_HeadStream_bufferNextCommand(vhs, vss); //Should never return 0
        }
        type = vhs->subbuffer[0];
        cmdlen = VGM_HeadStream_getCommandLen(type);
        dontunbuffer = 0;
        if(type == 0x50){
            //PSG write
            head->iswrite = 1;
            head->writecmd.cmd = type;
            head->writecmd.data = vhs->subbuffer[1];
            if((head->writecmd.data & 0x80) && !(head->writecmd.data & 0x10) && (head->writecmd.data < 0xE0)){
                //It's a main write, not attenuation, and not noise
                u8 bufferpos, newtype;
                bufferpos = vhs->subbufferlen;
                while(VGM_HeadStream_bufferNextCommand(vhs, vss)){
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
        }else if((type & 0xFE) == 0x52){
            //OPN2 write
            head->iswrite = 1;
            head->writecmd.cmd = type;
            head->writecmd.addr = vhs->subbuffer[1];
            head->writecmd.data = vhs->subbuffer[2];
            if((head->writecmd.addr & 0xF4) == 0xA4){
                //Frequency MSB write, read to find frequency LSB write command
                u8 bufferpos, newtype;
                bufferpos = vhs->subbufferlen;
                while(VGM_HeadStream_bufferNextCommand(vhs, vss)){
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
        }else if(type >= 0x80 && type <= 0x8F){
            //OPN2 DAC write
            head->iswrite = 1;
            head->writecmd.cmd = 0x52;
            head->writecmd.addr = 0x2A;
            head->writecmd.data = VGM_SourceStream_getBlockByte(vss, vhs->srcblockaddr++);
            if(type != 0x80){
                //Replace the command with the equivalent wait command
                vhs->subbuffer[0] = type - 0x11;
                dontunbuffer = 1;
            }
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
            //Behaves like endless stream of 65535-tick waits
            head->isdone = 1;
        }else if(type == 0x67){
            //Data block
            //Skip 0x66 in subbuffer[1]
            u32 a = vhs->subbuffer[2];
            if(a != 0){
                //Format other than uncompressed YM2612 PCM not supported
                head->isdone = 1;
            }else{
                a = vhs->subbuffer[3] | ((u32)vhs->subbuffer[4] << 8) 
                    | ((u32)vhs->subbuffer[5] << 16) | ((u32)vhs->subbuffer[6] << 24);
                DBG("Loading data block from %x size %x", vhs->srcaddr, a);
                VGM_SourceStream_loadBlock(vss, vhs->srcaddr, a);
                DBG("Block loaded!");
                vhs->srcaddr += a;
            }
        }else if(type == 0xE0){
            //Seek in data block
            vhs->srcblockaddr = vhs->subbuffer[1] | ((u32)vhs->subbuffer[2] << 8) 
                | ((u32)vhs->subbuffer[3] << 16) | ((u32)vhs->subbuffer[4] << 24);
        }
        if(!dontunbuffer){
            VGM_HeadStream_unBuffer(vhs, cmdlen+1);
        }
    }
}

VgmSource* VGM_SourceStream_Create(){
    VgmSource* source = malloc(sizeof(VgmSource));
    source->type = VGM_SOURCE_TYPE_STREAM;
    source->opn2clock = 0;
    source->psgclock = 0;
    source->loopaddr = 0;
    source->loopsamples = 0xFFFFFFFF;
    VgmSourceStream* vss = malloc(sizeof(VgmSourceStream));
    source->data = vss;
    vss->datalen = 0;
    vss->vgmdatastartaddr = 0;
    vss->buffer1 = malloc(VGM_SOURCESTREAM_BUFSIZE);
    vss->buffer2 = malloc(VGM_SOURCESTREAM_BUFSIZE);
    vss->buffer1addr = 0;
    vss->buffer2addr = 0;
    vss->wantbuffer = 0;
    vss->wantbufferaddr = 0;
    vss->block = NULL;
    vss->blocklen = 0;
    vss->blockorigaddr = 0xFFFFFFFF;
    return source;
}
void VGM_SourceStream_Delete(void* sourcestream){
    VgmSourceStream* vss = (VgmSourceStream*)sourcestream;
    free(vss->buffer1);
    free(vss->buffer2);
    if(vss->block != NULL){
        free(vss->block);
    }
}
extern s32 VGM_SourceStream_Start(VgmSource* source, char* filename){
    VgmSourceStream* vss = (VgmSourceStream*)source->data;
    s32 res = FILE_ReadOpen(&(vss->file), filename);
    if(res < 0) return res;
    vss->datalen = FILE_ReadGetCurrentSize();
    //Fill both buffers
    vss->buffer1addr = 0;
    res = FILE_ReadBuffer(vss->buffer1, VGM_SOURCESTREAM_BUFSIZE);
    if(res < 0){vss->datalen = 0; return -2;}
    vss->buffer2addr = VGM_SOURCESTREAM_BUFSIZE;
    res = FILE_ReadBuffer(vss->buffer2, VGM_SOURCESTREAM_BUFSIZE);
    if(res < 0){vss->datalen = 0; return -3;}
    //Close file and save for reopening
    res = FILE_ReadClose(&(vss->file));
    if(res < 0){vss->datalen = 0; return -4;}
    //Read header
    if(        VGM_SourceStream_getByte(vss, 0) == 'V' 
            && VGM_SourceStream_getByte(vss, 1) == 'g' 
            && VGM_SourceStream_getByte(vss, 2) == 'm' 
            && VGM_SourceStream_getByte(vss, 3) == ' '){
        //File has header
        //Get version
        u8 ver_lo = VGM_SourceStream_getByte(vss, 8);
        u8 ver_hi = VGM_SourceStream_getByte(vss, 9);
        source->psgclock = ((u32)VGM_SourceStream_getByte(vss, 0x0C)) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x0D) << 8)
                | ((u32)VGM_SourceStream_getByte(vss, 0x0E) << 16) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x0F) << 24);
        source->loopaddr = (((u32)VGM_SourceStream_getByte(vss, 0x1C)) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x1D) << 8)
                | ((u32)VGM_SourceStream_getByte(vss, 0x1E) << 16) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x1F) << 24))
                + 0x1C;
        source->loopsamples = ((u32)VGM_SourceStream_getByte(vss, 0x20)) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x21) << 8)
                | ((u32)VGM_SourceStream_getByte(vss, 0x22) << 16) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x23) << 24);
        source->opn2clock = ((u32)VGM_SourceStream_getByte(vss, 0x2C)) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x2D) << 8)
                | ((u32)VGM_SourceStream_getByte(vss, 0x2E) << 16) 
                | ((u32)VGM_SourceStream_getByte(vss, 0x2F) << 24);
        if(ver_hi < 1 || (ver_hi == 1 && ver_lo < 0x50)){
            vss->vgmdatastartaddr = 0x40;
        }else{
            vss->vgmdatastartaddr = (((u32)VGM_SourceStream_getByte(vss, 0x34)) 
                    | ((u32)VGM_SourceStream_getByte(vss, 0x35) << 8)
                    | ((u32)VGM_SourceStream_getByte(vss, 0x36) << 16) 
                    | ((u32)VGM_SourceStream_getByte(vss, 0x37) << 24))
                    + 0x34;
        }
    }else{
        vss->vgmdatastartaddr = 0;
        source->opn2clock = 0;
        source->psgclock = 0;
        source->loopaddr = 0;
        source->loopsamples = 0xFFFFFFFF;
    }
    return 0;
}
u8 VGM_SourceStream_getByte(VgmSourceStream* vss, u32 addr){
    if(addr >= vss->datalen) return 0;
    if(addr >= vss->buffer1addr && addr < (vss->buffer1addr + VGM_SOURCESTREAM_BUFSIZE)){
        if(vss->buffer2addr != vss->buffer1addr + VGM_SOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            vss->wantbuffer = 2;
            vss->wantbufferaddr = vss->buffer1addr + VGM_SOURCESTREAM_BUFSIZE;
        }
        return vss->buffer1[addr - vss->buffer1addr];
    }
    if(addr >= vss->buffer2addr && addr < (vss->buffer2addr + VGM_SOURCESTREAM_BUFSIZE)){
        if(vss->buffer1addr != vss->buffer2addr + VGM_SOURCESTREAM_BUFSIZE){
            //Set up to background buffer next
            vss->wantbuffer = 1;
            vss->wantbufferaddr = vss->buffer2addr + VGM_SOURCESTREAM_BUFSIZE;
        }
        return vss->buffer2[addr - vss->buffer2addr];
    }
    //Have to load something right now
    u8 leds = MIOS32_BOARD_LED_Get();
    MIOS32_BOARD_LED_Set(0b1111, 0b0100);
    s32 res = FILE_ReadReOpen(&(vss->file));
    if(res < 0) return 0;
    res = FILE_ReadSeek(addr);
    if(res < 0) return 0;
    res = FILE_ReadBuffer(vss->buffer1, VGM_SOURCESTREAM_BUFSIZE);
    if(res < 0) return 0;
    vss->buffer1addr = addr;
    FILE_ReadClose(&(vss->file));
    //Set up to load the next buffer next
    vss->wantbuffer = 2;
    vss->wantbufferaddr = addr + VGM_SOURCESTREAM_BUFSIZE;
    //Done
    MIOS32_BOARD_LED_Set(0b1111, leds);
    return vss->buffer1[0];
}
void VGM_SourceStream_loadBlock(VgmSourceStream* vss, u32 startaddr, u32 len){
    if(startaddr == vss->blockorigaddr && len == vss->blocklen) return; //Don't reload existing block
    if(vss->block != NULL){
        free(vss->block);
    }
    vss->block = malloc(len);
    s32 res = FILE_ReadReOpen(&(vss->file));
    if(res < 0) return;
    res = FILE_ReadSeek(startaddr);
    if(res < 0) return;
    res = FILE_ReadBuffer(vss->block, len);
    if(res < 0) return;
    FILE_ReadClose(&(vss->file));
    vss->blocklen = len;
}
