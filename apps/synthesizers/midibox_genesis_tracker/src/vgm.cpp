/*
 * MIDIbox Genesis Tracker: Parent VGM Class
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
#include "vgm.h"

Vgm::Vgm(){
    isdone = false;
    delay62 = 735;
    delay63 = 882;
}

void Vgm::cmdNext(u32 curtime){
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
        type = nextByte();
        if(type == 0x50){
            //PSG write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.data = nextByte();
        }else if(type == 0x52 || type == 0x53){
            //OPN2 write
            iswrite = true;
            writecmd.cmd = type;
            writecmd.addr = nextByte();
            writecmd.data = nextByte();
        }else if(type == 0xDE){
            //OPN2 Frequency write [unofficial]
            //Format:
            //-- Address of first byte to write (e.g. 0xA0)
            //-- Data to write to first byte
            //-- Data to write to second byte (e.g. 0xA4)
            isfreqwrite = true;
            iswrite = true;
            writecmd.addr = nextByte();
            writecmd.data = nextByte();
            writecmd.data2 = nextByte();
        }else if(type >= 0x80 && type <= 0x8F){
            //OPN2 DAC write
            isdacwrite = true;
            iswrite = true;
            waitduration = type - 0x80;
            waitstarttime = curtime;
            writecmd.cmd = 0x52;
            writecmd.addr = 0x2A;
            writecmd.data = nextBlockByte();
        }else if(type >= 0x70 && type <= 0x7F){
            //Short wait
            iswait = true;
            waitduration = type - 0x7F;
        }else if(type == 0x61){
            //Long wait
            iswait = true;
            waitduration = nextByte() | ((u32)nextByte() << 8);
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
            u8 tooverride = nextByte();
            u32 newdelay = nextByte() | ((u32)nextByte() << 8);
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
            //Data block TODO
            u8 todo;
        }else if(type == 0xE0){
            //Seek in data block TODO
            u8 todo;
        }else if(type == 0xDF){
            //Loop within VGM [unofficial] TODO
            u8 todo;
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x90){
            //Setup Stream Control not supported
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x91){
            //Set Stream Data not supported
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x92){
            //Set Stream Frequency not supported
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x93){
            //Start Stream not supported
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x94){
            //Stop Stream not supported
            nextByte();
        }else if(type == 0x95){
            //Start Stream fast not supported
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type == 0x68){
            //PCM RAM write, not supported; skip 11 bytes
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else if(type >= 0x30 && type <= 0x3F){
            //Unsupported single-byte command
            nextByte();
        }else if((type >= 0x40 && type <= 0x4E) || (type >= 0xA0 && type <= 0xBF)){
            //Unsupported double-byte command
            nextByte();
            nextByte();
        }else if(type >= 0xC0 && type <= 0xDD){
            //Unsupported triple-byte command
            nextByte();
            nextByte();
            nextByte();
        }else if(type >= 0xE1 && type <= 0xFF){
            //Unsupported quadruple-byte command
            nextByte();
            nextByte();
            nextByte();
            nextByte();
        }else{
            //Unsupported, do nothing (assume 0 byte command)
        }
    }
}

