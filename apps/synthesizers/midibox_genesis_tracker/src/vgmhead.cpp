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
    opn2mult = 0x1000;
    paused = false;
    restart();
}

void VgmHead::cmdNext(u32 curtime){
    if(isfreqwrite){
        //Second virtual command is the second byte
        isfreqwrite = false;
        writecmd.data = writecmd.data2;
        writecmd.addr ^= 0x04; //Switch from frequency MSB to LSB or vice versa
        return;
    }else if(isdacwrite){
        //Second virtual command is the wait by itself
        isdacwrite = false;
        iswait = true;
        iswrite = false;
        return;
    }
    u8 type;
    iswait = iswrite = false;
    while(!(iswait || iswrite)){
        type = source->getByte(srcaddr++);
        if(type == 0x50){
            //PSG write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.data = source->getByte(srcaddr++);
        }else if(type == 0x52 || type == 0x53){
            //OPN2 write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.addr = source->getByte(srcaddr++);
            writecmd.data = source->getByte(srcaddr++);
        }else if(type == 0xDE){
            //OPN2 Frequency write [unofficial]
            //Format:
            //-- Address of first byte to write (e.g. 0xA4)
            //-- Data to write to first byte
            //-- Data to write to second byte (e.g. 0xA0)
            isfreqwrite = true;
            iswrite = true;
            writecmd.addr = source->getByte(srcaddr++);
            writecmd.data = source->getByte(srcaddr++);
            writecmd.data2 = source->getByte(srcaddr++);
            u8 block; u32 freq;
            if(!(writecmd.addr & 0x04)){
                //The VGM wrote the LSB first, swap them
                writecmd.addr |= 0x04;
                block = writecmd.data;
                writecmd.data = writecmd.data2;
                writecmd.data2 = block;
            }
            block = (writecmd.data >> 3) & 0x07;
            freq = ((u32)(writecmd.data & 0x07) << 8) | writecmd.data2; //Up to 11 bits set
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
            writecmd.data = (freq >> 8) | (block << 3);
            writecmd.data2 = (freq & 0xFF);
        }else if(type >= 0x80 && type <= 0x8F){
            //OPN2 DAC write
            isdacwrite = true;
            iswrite = true;
            waitduration = type - 0x80;
            waitstarttime = curtime;
            writecmd.cmd = 0x52;
            writecmd.addr = 0x2A;
            writecmd.data = source->getBlockByte(srcblockaddr++);
        }else if(type >= 0x70 && type <= 0x7F){
            //Short wait
            iswait = true;
            waitduration = type - 0x7F;
        }else if(type == 0x61){
            //Long wait
            iswait = true;
            waitduration = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8);
            srcaddr += 2;
        }else if(type == 0x62){
            //60 Hz wait
            iswait = true;
            waitduration = delay62;
        }else if(type == 0x63){
            //50 Hz wait
            iswait = true;
            waitduration = delay63;
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
            //Loop within VGM [unofficial] TODO scrap this there's loop info in the header
            srcaddr = source->getByte(srcaddr) | ((u32)source->getByte(srcaddr+1) << 8)
                | ((u32)source->getByte(srcaddr+2) << 16);
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

void VgmHead::restart(){
    srcaddr = source->vgmdatastartaddr;
    srcblockaddr = 0;
}

