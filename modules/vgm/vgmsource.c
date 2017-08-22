/*
 * VGM Data and Playback Driver: VGM Data Source
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmsource.h"
#include "vgmhead.h"
#include "vgmqueue.h"
#include "vgmram.h"
#include "vgmstream.h"
#include "vgm_heap2.h"


u8 VGM_Cmd_GetCmdLen(u8 type){
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
u32 VGM_Cmd_GetWaitValue(VgmChipWriteCmd cmd){
    if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        return cmd.cmd - 0x80;
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        //Short wait
        return cmd.cmd - 0x6F;
    }else if(cmd.cmd == 0x61){
        //Long wait
        return cmd.data | ((u32)cmd.data2 << 8);
    }else if(cmd.cmd == 0x62){
        //60 Hz wait
        return VGM_DELAY62;
    }else if(cmd.cmd == 0x63){
        //50 Hz wait
        return VGM_DELAY63;

    }else{
        return 0;
    }
}
u8 VGM_Cmd_IsWrite(VgmChipWriteCmd cmd){
    if(cmd.cmd == 0x50){
        //PSG write
        return 1;
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        return 1;
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        return 1;
    }else{
        return 0;
    }
}
u8 VGM_Cmd_IsTwoByte(VgmChipWriteCmd cmd){
    if(cmd.cmd == 0x50){
        //PSG write
        if((cmd.data & 0x80) && !(cmd.data & 0x10) && (cmd.data < 0xE0)){
            //It's a main write, not attenuation, and not noise--i.e. frequency
            return 1;
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        if((cmd.addr & 0xF4) == 0xA4){
            //Frequency MSB write
            return 1;
        }else if(cmd.cmd == 0x52 && cmd.addr == 0x24){
            //Timer A MSB
            return 1;
        }
    }
    return 0;
}
u8 VGM_Cmd_IsSecondHalfTwoByte(VgmChipWriteCmd cmd){
    if(cmd.cmd == 0x50){
        //PSG write
        if(!(cmd.data & 0x80)){
            //Frequency MSB
            return 1;
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        if((cmd.addr & 0xF4) == 0xA0){
            //Frequency LSB write
            return 1;
        }else if(cmd.cmd == 0x52 && cmd.addr == 0x25){
            //Timer A LSB
            return 1;
        }
    }
    return 0;
}
VgmChipWriteCmd VGM_Cmd_TryMakeTwoByte(VgmChipWriteCmd first, VgmChipWriteCmd second){
    VgmChipWriteCmd ret = {.all = first.all};
    if(first.cmd == 0x50){
        //PSG write
        if((first.data & 0x80) && !(first.data & 0x10) && (first.data < 0xE0)){
            //It's a main write, not attenuation, and not noise--i.e. frequency
            if(second.cmd == 0x50){
                //PSG write
                if(!(second.data & 0x80)){
                    //Frequency second byte
                    ret.data2 = second.data;
                    return ret;
                }
            }
        }
    }else if((first.cmd & 0xFE) == 0x52){
        //OPN2 write
        if((first.addr & 0xF4) == 0xA4){
            //Frequency MSB write
            if(second.cmd == first.cmd){
                //OPN2 write to same bank
                if((first.addr & 0xFB) == (second.addr & 0xFB) && !(second.addr & 0x04)){
                    //Same channel's frequency command, lower address
                    ret.data2 = second.data;
                    return ret;
                }
            }
        }else if(first.cmd == 0x52 && first.addr == 0x24){
            //Timer A MSB
            if(second.cmd == 0x52 && second.addr == 0x25){
                ret.data2 = second.data;
                return ret;
            }
        }
    }
    ret.all = 0xFFFFFFFF;
    return ret;
}
u8 VGM_Cmd_UnpackTwoByte(VgmChipWriteCmd in, VgmChipWriteCmd* out1, VgmChipWriteCmd* out2){
    //Common for all
    out1->all = in.all;
    out2->all = in.all;
    out2->data = in.data2;
    //
    if((in.cmd & 0x0F) == 0){
        //PSG write
        if((in.data & 0x80) && !(in.data & 0x10) && (in.data < 0xE0)){
            //It's a main write, not attenuation, and not noise--i.e. frequency
            return 1;
        }
    }else if((in.cmd & 0x0E) == 2){
        //OPN2 write
        if((in.addr & 0xF4) == 0xA4){
            //Frequency MSB write
            out2->addr = in.addr & 0xFB;
            return 1;
        }else if((in.cmd & 0x0F) == 2 && in.addr == 0x24){
            //Timer A MSB
            out2->addr = 0x25;
            return 1;
        }
    }else if(in.cmd >= 0x80 && in.cmd <= 0x8F){
        out1->cmd = 0x52;
        out1->addr = 0x2A;
        if(in.cmd != 0x80){
            //Wait afterwards is not 0
            out2->cmd = in.cmd - 0x81 + 0x70;
            return 1;
        }
    }
    return 0;
}

void VGM_Cmd_UpdateUsage(VgmUsageBits* usage, VgmChipWriteCmd cmd){
    if(cmd.cmd == 0x50){
        //PSG write
        if(!(cmd.data & 0x80)) return; //Second half of freq write, shouldn't be here
        //Main write
        if(cmd.data & 0x10){
            //Volume write
            if((cmd.data & 0x0F) == 0x0F) return; //Init to off doesn't count as usage
        }else{
            if(((cmd.data & 0x0F) == 0x00 && (cmd.data2 & 0x3F) == 0) 
                || ((cmd.data & 0xF) == 0x0F && (cmd.data2 & 0x3F) == 0x3F)) return; 
                //Init to freq 0 or 0x3FF doesn't count as usage
            if((cmd.data & 0x63) == 0x63){
                usage->noisefreqsq3 = 1;
                usage->sq3 = 1;
            }
            return; //Other than sq3/noise, only volume commands set usage
        }
        usage->all |= (1 << (24 + ((cmd.data >> 5) & 3)));
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, addrhi = (cmd.cmd & 1)*3;
        if(cmd.addr <= 0x2F){
            if(cmd.addr < 0x20 || addrhi) return;
            //OPN2 global register
            if(cmd.addr == 0x28){
                //Key On register
                if((cmd.data & 0xF0) == 0) return; //Don't count key offs
                chan = cmd.data & 0x07;
                if((chan & 0x03) == 0x03) return; //Make sure not writing to 0x03 or 0x07
                if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                usage->all |= 1 << (chan);
            }else if(cmd.addr == 0x27){
                //Only consider writes to FM3 bits, not other timer stuff
                if((cmd.data & 0xC0) == 0) return; //Don't count setting it to 0
                usage->fm3 = 1;
                usage->fm3_special = 1;
            }else if(cmd.addr == 0x2A){
                if(cmd.data == 0x00 || cmd.data == 0x80) return; //Don't count 0
                usage->dac = 1;
            }else if(cmd.addr == 0x2B){
                if(!(cmd.data & 0x80)) return; //Don't count turning off DAC
                usage->dac = 1;
            }else if(cmd.addr == 0x22){
                if(usage->lfomode == 0){
                    if(cmd.data & 0x08){ //Command is to turn on the LFO; ignore commands to turn it off, unless it was already turned on
                        usage->lfomode = 1;
                        usage->lfofixedspeed = cmd.data & 0x07;
                    }
                }else if(usage->lfomode == 1){
                    if(cmd.data != (0x08 | usage->lfofixedspeed)){ //It was turned off or changed speed
                        usage->lfomode = 2;
                    }
                }
            }else if(cmd.addr == 0x21 || cmd.addr == 0x2C){
                if(cmd.data == 0x00) return; //Don't count disabling everything
                usage->opn2_globals = 1;
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            if(cmd.data == 0x00) return; //Don't count inits to 0
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) return; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            usage->all |= 1 << (chan);
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Don't count these at all
            /*
            //Channel 3 extra frequency
            usage->fm3 = 1;
            usage->fm3_special = 1;
            */
        }else if(cmd.addr <= 0xB6){
            //Channel command
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) return; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            if(cmd.data == 0x00) return; //Don't count resetting
            if((cmd.addr & 0xFC) == 0xB4){
                if(cmd.data == 0xC0) return; //Don't count resetting
                if((cmd.data & 0x37) != 0){
                    //LFO-Frq or LFO-Amp enabled
                    usage->all |= 1 << (chan+6);
                }
            }
            usage->all |= 1 << (chan);
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //DAC write and wait
        usage->dac = 1;
    }
}

void VGM_Cmd_DebugPrintUsage(VgmUsageBits usage){
    DBG("[Key:  ---QN321---SpdLf-037654321654321]");
    DBG("Usage: %32b", usage.all);
}

void VGM_Source_UpdateUsage(VgmSource* source){
    if(source->type == VGM_SOURCE_TYPE_RAM){
        VGM_SourceRAM_UpdateUsage(source);
    }else if(source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_SourceStream_UpdateUsage(source);
    }else if(source->type == VGM_SOURCE_TYPE_QUEUE){
        VGM_SourceQueue_UpdateUsage(source);
    }
}

s32 VGM_Source_Delete(VgmSource* source){
    if(source == NULL) return -1;
    s32 i;
    for(i=0; i<vgm_numheads; ++i){
        if(vgm_heads[i] != NULL && vgm_heads[i]->source == source){
            VGM_Head_Delete(vgm_heads[i]);
            --i;
        }
    }
    if(source->type == VGM_SOURCE_TYPE_RAM){

        VGM_SourceRAM_Delete(source->data);
    }else if(source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_SourceStream_Delete(source->data);
    }else if(source->type == VGM_SOURCE_TYPE_QUEUE){
        VGM_SourceQueue_Delete(source->data);
    }
    vgmh2_free(source);
    return 0;
}
