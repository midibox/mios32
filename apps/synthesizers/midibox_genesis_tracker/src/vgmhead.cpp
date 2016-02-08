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

void fixOPN2Frequency(ChipWriteCmd* writecmd, u32 opn2mult){
    u8 block; u32 freq;
    if(!(writecmd->addr & 0x04)){
        //The VGM wrote the LSB first, swap them
        writecmd->addr |= 0x04;
        block = writecmd->data;
        writecmd->data = writecmd->data2;
        writecmd->data2 = block;
    }
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
    u8 type;
    iswait = iswrite = false;
    while(!(iswait || iswrite)){
        if(subbufferlen > 0){
            type = subbuffer[0];
            subbuffer[0] = subbuffer[1];
            subbuffer[1] = subbuffer[2];
            subbuffer[2] = subbuffer[3];
            --subbufferlen;
        }else{
            type = source->getByte(srcaddr++);
        }
        if(type == 0x50){
            //PSG write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.data = source->getByte(srcaddr++);
            //TODO also adjust frequency on PSG write
        }else if((type & 0xFE) == 0x52){
            //OPN2 write
            iswrite = true;
            writecmd.cmd = type;
            if(subbufferlen > 0){
                writecmd.addr = subbuffer[0];
                writecmd.data = subbuffer[1];
                subbufferlen = 0;
            }else{
                writecmd.addr = source->getByte(srcaddr++);
                writecmd.data = source->getByte(srcaddr++);
            }
            if((writecmd.addr & 0xF0) == 0xA0){
                //Frequency write, read next command
                subbuffer[0] = source->getByte(srcaddr++);
                subbufferlen = 1;
                if((subbuffer[0] & 0xFE) == 0x52){
                    //The next command is another frequency write
                    subbuffer[1] = source->getByte(srcaddr++); //Address
                    subbuffer[2] = source->getByte(srcaddr++); //Data
                    subbufferlen = 3;
                    if((writecmd.addr & 0xFB) == (subbuffer[1] & 0xFB)){
                        //Both writes to the same channel
                        writecmd.data2 = subbuffer[2];
                        fixOPN2Frequency(&writecmd, opn2mult);
                        //Reconstruct next command
                        subbuffer[2] = writecmd.data2;
                    }
                }else if((subbuffer[0] >= 0x70) && (subbuffer[0] <= 0x8F)){
                    //The next command is a short wait or DAC write, read the one after that
                    subbuffer[1] = source->getByte(srcaddr++);
                    subbufferlen = 2;
                    if((subbuffer[1] & 0xFE) == 0x52){
                        //The command after that is a frequency write
                        subbuffer[2] = source->getByte(srcaddr++); //Address
                        subbuffer[3] = source->getByte(srcaddr++); //Data
                        subbufferlen = 4;
                        if((writecmd.addr & 0xFB) == (subbuffer[2] & 0xFB)){
                            //Both writes to the same channel
                            writecmd.data2 = subbuffer[3];
                            fixOPN2Frequency(&writecmd, opn2mult);
                            //Reconstruct next command
                            subbuffer[3] = writecmd.data2;
                        }
                    }
                }
            }
        }else if(type == 0xDE){
            //OPN2 Frequency write [unofficial]
            /*
            FIXME this doesn't work because of different addr hi
            iswrite = true;
            writecmd.addr = source->getByte(srcaddr++);
            writecmd.data = source->getByte(srcaddr++);
            writecmd.data2 = source->getByte(srcaddr++);
            fixOPN2Frequency(&writecmd, opn2mult);
            subbuffer[0] = 
            */
        }else if(type >= 0x80 && type <= 0x8F){
            //iswait = true;
            //ticks += type - 0x80;
            //OPN2 DAC write
            iswrite = true;
            writecmd.cmd = 0x52;
            writecmd.addr = 0x2A;
            writecmd.data = source->getBlockByte(srcblockaddr++);
            if(type != 0x80){
                if(subbufferlen != 0){
                    //There's still a write command in there
                    subbuffer[3] = subbuffer[2];
                    subbuffer[2] = subbuffer[1];
                    subbuffer[1] = subbuffer[0];
                }
                subbuffer[0] = type - 0x11;
                ++subbufferlen;
            } //else just do nothing afterwards
        }else if(type >= 0x70 && type <= 0x7F){
            //Short wait
            iswait = true;
            ticks += type - 0x6F;
        }else if(type == 0x61){
            //Long wait
            iswait = true;
            ticks += source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8);
            srcaddr += 2;
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
            u8 tooverride = source->getByte(srcaddr++);
            u32 newdelay = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8);
            srcaddr += 2;
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
            ++srcaddr; //Skip 0x66
            u32 a = source->getByte(srcaddr++);
            if(a != 0){
                isdone = true;
            }else{
                a = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8)
                    | ((u32)source->getByte(srcaddr+2) << 16) | ((u32)source->getByte(srcaddr+3) << 24);
                DBG("Loading data block from %x size %x", srcaddr, a);
                source->loadBlock(srcaddr, a);
                DBG("Block loaded!");
                srcaddr += a;
            }
        }else if(type == 0xE0){
            //Seek in data block
            srcblockaddr = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8)
                | ((u32)source->getByte(srcaddr+2) << 16) | ((u32)source->getByte(srcaddr+3) << 24);
            srcaddr += 4;
        }else if(type == 0xDF){
            //Loop within VGM [unofficial] FIXME scrap this there's loop info in the header
            //srcaddr = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8)
            //    | ((u32)source->getByte(srcaddr+2) << 16);
        }else if(type == 0x90){
            //Setup Stream Control not supported
            srcaddr += 4;
        }else if(type == 0x91){
            //Set Stream Data not supported
            srcaddr += 4;
        }else if(type == 0x92){
            //Set Stream Frequency not supported
            srcaddr += 5;
        }else if(type == 0x93){
            //Start Stream not supported
            srcaddr += 10;
        }else if(type == 0x94){
            //Stop Stream not supported
            srcaddr++;
        }else if(type == 0x95){
            //Start Stream fast not supported
            srcaddr += 4;
        }else if(type == 0x68){
            //PCM RAM write, not supported
            srcaddr += 11;
        }else if(type >= 0x30 && type <= 0x3F){
            //Unsupported single-byte command
            srcaddr++;
        }else if((type >= 0x40 && type <= 0x4E) || (type >= 0xA0 && type <= 0xBF)){
            //Unsupported double-byte command
            srcaddr += 2;
        }else if(type >= 0xC0 && type <= 0xDD){
            //Unsupported triple-byte command
            srcaddr += 3;
        }else if(type >= 0xE1 && type <= 0xFF){
            //Unsupported quadruple-byte command
            srcaddr += 4;
        }else{
            //Unsupported, do nothing (assume 0 byte command)
        }
    }
}

void VgmHead::restart(u32 vgm_time){
    srcaddr = source->vgmdatastartaddr;
    srcblockaddr = 0;
    ticks = vgm_time;
}

