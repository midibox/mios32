/*
 * MIDIbox Genesis Tracker: VGM Playback Head Class
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
#include "vgmhead.h"


VgmHead::VgmHead(VgmSourceStream* src){
    source = src;
    srcaddr = 0;
    isdone = false;
    delay62 = 735;
    delay63 = 882;
    opn2mult = ((7670454 << 8) / 500000); //0x1000; //TODO adjust in real time
    subbufferlen = 0;
}


void VgmHead::restart(u32 vgm_time){
    srcaddr = source->vgmdatastartaddr;
    srcblockaddr = 0;
    ticks = vgm_time;
}

u8 VgmHead::getCommandLen(u8 type){
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

bool VgmHead::bufferNextCommand(){
    if(subbufferlen == SUBBUFFER_MAXLEN) return false;
    u8 type = source->getByte(srcaddr);
    u8 len = getCommandLen(type);
    if(len + 1 + subbufferlen <= SUBBUFFER_MAXLEN){
        ++srcaddr;
        subbuffer[subbufferlen++] = type;
        for(u8 i=0; i<len; i++){
            subbuffer[subbufferlen++] = source->getByte(srcaddr++);
        }
        return true;
    }else{
        return false;
    }
}

void VgmHead::unBuffer(u8 len){
    if(len > subbufferlen) len = subbufferlen;
    subbufferlen -= len;
    for(u8 i=0; i<subbufferlen; i++){
        subbuffer[i] = subbuffer[i+len];
    }
}

void VgmHead::fixOPN2Frequency(ChipWriteCmd* writecmd, u32 opn2mult){
    u8 block; u32 freq;
    /*
    if(!(writecmd->addr & 0x04)){
        //The VGM wrote the LSB first, swap them
        writecmd->addr |= 0x04;
        block = writecmd->data;
        writecmd->data = writecmd->data2;
        writecmd->data2 = block;
    }
    */
    block = (writecmd->data >> 3) & 0x07;
    freq = ((u32)(writecmd->data & 0x07) << 8) | writecmd->data2; //Up to 11 bits set
    freq <<= block; //Up to 18 bits set
    freq *= opn2mult; //If unity (0x1000), up to 30 bits set (29 downto 0)
    //Check for overflow
    if(freq & 0xC0000000){
        freq = 0x3FFFFFFF;
    }
    //To floating point: find most-significant 1
    for(block=8; block>1; --block){
        if(freq & 0x20000000) break;
        freq <<= 1;
    }
    --block;
    freq >>= 19; //Previously up to 30 bits set, now up to 11 bits set
    writecmd->data = (freq >> 8) | (block << 3);
    writecmd->data2 = (freq & 0xFF);
}

void VgmHead::cmdNext(u32 vgm_time){
    u8 type, cmdlen;
    iswait = iswrite = false;
    bool dontunbuffer;
    while(!(iswait || iswrite)){
        if(subbufferlen == 0){
            bufferNextCommand(); //Should never return false
        }
        type = subbuffer[0];
        cmdlen = getCommandLen(type);
        dontunbuffer = false;
        if(type == 0x50){
            //PSG write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.data = subbuffer[1];
            //TODO also adjust frequency on PSG write
        }else if((type & 0xFE) == 0x52){
            //OPN2 write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.addr = subbuffer[1];
            writecmd.data = subbuffer[2];
            if((writecmd.addr & 0xF4) == 0xA4){
                //Frequency MSB write, read to find frequency LSB write command
                u8 bufferpos, newtype;
                bufferpos = 3;
                while(bufferNextCommand()){
                    newtype = subbuffer[bufferpos];
                    if(newtype == type){
                        //Next command is another OPN2 write to the same addrhi
                        if((writecmd.addr & 0xFB) == (subbuffer[bufferpos+1] & 0xFB)){
                            //Second command is a frequency write to same channel
                            if(!(subbuffer[bufferpos+1] & 0x04)){
                                //It's a frequency LSB write
                                writecmd.data2 = subbuffer[bufferpos+2];
                                fixOPN2Frequency(&writecmd, opn2mult);
                                //Reconstruct next command
                                subbuffer[bufferpos+2] = writecmd.data2;
                            }
                            //If it's a frequency MSB command, don't modify anything, and stop
                            break;
                        }
                    }
                    bufferpos = subbufferlen;
                }
            }
        }else if(type >= 0x80 && type <= 0x8F){
            //OPN2 DAC write
            iswrite = true;
            writecmd.cmd = 0x52;
            writecmd.addr = 0x2A;
            writecmd.data = source->getBlockByte(srcblockaddr++);
            if(type != 0x80){
                //Replace the command with the equivalent wait command
                subbuffer[0] = type - 0x11;
                dontunbuffer = true;
            }
        }else if(type >= 0x70 && type <= 0x7F){
            //Short wait
            iswait = true;
            ticks += type - 0x6F;
        }else if(type == 0x61){
            //Long wait
            iswait = true;
            ticks += subbuffer[1] | ((u32)subbuffer[2] << 8);
        }else if(type == 0x62){
            //60 Hz wait
            iswait = true;
            ticks += delay62;
        }else if(type == 0x63){
            //50 Hz wait
            iswait = true;
            ticks += delay63;
        }else if(type == 0x64){
            //Override wait lengths
            u8 tooverride = subbuffer[1];
            u32 newdelay = subbuffer[2] | ((u32)subbuffer[3] << 8);
            if(tooverride == 0x62){
                delay62 = newdelay;
            }else if(tooverride == 0x63){
                delay63 = newdelay;
            }
        }else if(type == 0x65){
            //Nop [unofficial]
        }else if(type == 0x66){
            //End of data
            //Behaves like endless stream of 65535-tick waits
            isdone = true;
        }else if(type == 0x67){
            //Data block
            //Skip 0x66 in subbuffer[1]
            u32 a = subbuffer[2];
            if(a != 0){
                //Format other than uncompressed YM2612 PCM not supported
                isdone = true;
            }else{
                a = subbuffer[3] | ((u32)subbuffer[4] << 8)| ((u32)subbuffer[5] << 16) 
                    | ((u32)subbuffer[6] << 24);
                DBG("Loading data block from %x size %x", srcaddr, a);
                source->loadBlock(srcaddr, a);
                DBG("Block loaded!");
                srcaddr += a;
            }
        }else if(type == 0xE0){
            //Seek in data block
            srcblockaddr = subbuffer[1] | ((u32)subbuffer[2] << 8) | ((u32)subbuffer[3] << 16) 
                | ((u32)subbuffer[4] << 24);
        }
        if(!dontunbuffer){
            unBuffer(cmdlen+1);
        }
    }
}

