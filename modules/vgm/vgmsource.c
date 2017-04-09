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

void VGM_Cmd_UpdateUsage(VgmUsageBits* usage, VgmChipWriteCmd cmd){
    if(cmd.cmd == 0x50){
        //PSG write
        usage->all |= (1 << (24 + ((cmd.data >> 5) & 3)));
        if((cmd.data & 0x63) == 0x63){
            usage->noisefreqsq3 = 1;
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, addrhi = (cmd.cmd & 1)*3;
        if(cmd.addr <= 0x2F){
            if(cmd.addr < 0x20 || addrhi) return;
            //OPN2 global register
            if(cmd.addr == 0x28){
                //Key On register
                chan = cmd.data & 0x07;
                if((chan & 0x03) == 0x03) return; //Make sure not writing to 0x03 or 0x07
                if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                usage->all |= 1 << (chan);
            }else if(cmd.addr == 0x24 || cmd.addr == 0x25 || cmd.addr == 0x27){
                usage->fm3 = 1;
                usage->fm3_special = 1;
            }else if(cmd.addr == 0x2A || cmd.addr == 0x2B){
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
                usage->opn2_globals = 1;
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) return; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            usage->all |= 1 << (chan);
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            usage->fm3 = 1;
            usage->fm3_special = 1;
        }else if(cmd.addr <= 0xB6){
            //Channel command
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) return; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            usage->all |= 1 << (chan);
            if((cmd.addr & 0xFC) == 0xB4){
                if((cmd.data & 0x37) != 0){
                    //LFO-Frq or LFO-Amp enabled
                    usage->all |= 1 << (chan+6);
                }
            }
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //DAC write and wait
        usage->dac = 1;
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

void VGM_Source_UpdateUsage(VgmSource* source){
    if(source->type == VGM_SOURCE_TYPE_RAM){
        VGM_SourceRAM_UpdateUsage(source);
    }else if(source->type == VGM_SOURCE_TYPE_STREAM){
        VGM_SourceStream_UpdateUsage(source);
    }else if(source->type == VGM_SOURCE_TYPE_QUEUE){
        VGM_SourceQueue_UpdateUsage(source);
    }
}

void VGM_Cmd_DebugPrintUsage(VgmUsageBits usage){
    DBG("[Key:  ---QN321---SpdLf-037654321654321]");
    DBG("Usage: %32b", usage.all);
}
