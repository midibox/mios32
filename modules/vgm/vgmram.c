/*
 * VGM Data and Playback Driver: VGM RAM Block Data
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmram.h"
#include "vgmhead.h"
#include "vgmtuning.h"
#include "vgm_heap2.h"

VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source){
    //VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    VgmHeadRAM* vhr = vgmh2_malloc(sizeof(VgmHeadRAM));
    vhr->srcaddr = 0;
    return vhr;
}
void VGM_HeadRAM_Delete(void* headram){
    VgmHeadRAM* vhr = (VgmHeadRAM*)headram;
    vgmh2_free(vhr);
}
void VGM_HeadRAM_Restart(VgmHead* head){
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    vhr->srcaddr = 0;
}
void VGM_HeadRAM_cmdNext(VgmHead* head, u32 vgm_time){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    head->iswait = 0;
    head->iswrite = 0;
    head->isdone = 0;
    if(head->firstoftwo){
        head->firstoftwo = 0;
        if(vhr->bufferedcmd.cmd == 0x50){
            //Second PSG frequency write
            head->writecmd.cmd = 0x00;
            head->writecmd.data = vhr->bufferedcmd.data2;
            head->iswrite = 1;
            VGM_Head_doMapping(head, &(head->writecmd));
            return;
        }else if((vhr->bufferedcmd.cmd & 0xFE) == 0x52){
            //Second OPN2 frequency write
            head->writecmd.cmd = 0x02 | (vhr->bufferedcmd.cmd & 0x01);
            head->writecmd.addr = vhr->bufferedcmd.addr & 0xFB; //A4 -> A0
            head->writecmd.data = vhr->bufferedcmd.data2;
            head->iswrite = 1;            
            VGM_Head_doMapping(head, &(head->writecmd));
            return;
        }else if((vhr->bufferedcmd.cmd & 0xF0) == 0x80){
            //Wait after a sample
            head->iswait = 1;
            head->ticks += vhr->bufferedcmd.cmd - 0x80;
            return;
        }
        //Otherwise there wasn't actually a second command...?
    }
    //Read new command
    if(vhr->srcaddr >= vsr->numcmds){
        //End of data
        if(head->source->loopaddr < vsr->numcmds){
            //Jump to loop point
            vhr->srcaddr = head->source->loopaddr;
        }else{
            //Behaves like endless stream of 65535-tick waits
            head->isdone = 1;
            return;
        }
    }
    vhr->bufferedcmd = vsr->cmds[vhr->srcaddr];
    ++vhr->srcaddr;
    //Parse command
    u8 type = vhr->bufferedcmd.cmd;
    if(type == 0x50){
        //PSG write
        head->iswrite = 1;
        if((vhr->bufferedcmd.data & 0x80) && !(vhr->bufferedcmd.data & 0x10) && (vhr->bufferedcmd.data < 0xE0)){
            //It's a main write, not attenuation, and not noise--i.e. frequency
            VGM_fixPSGFrequency(&(vhr->bufferedcmd), head->psgmult, head->psgfreq0to1);
            head->firstoftwo = 1;
        }
        head->writecmd = vhr->bufferedcmd;
        head->writecmd.cmd = 0x00;
        VGM_Head_doMapping(head, &(head->writecmd));
    }else if((type & 0xFE) == 0x52){
        //OPN2 write
        head->iswrite = 1;
        if((vhr->bufferedcmd.addr & 0xF4) == 0xA4){
            //Frequency MSB write
            VGM_fixOPN2Frequency(&(vhr->bufferedcmd), head->opn2mult);
            head->firstoftwo = 1;
        }
        head->writecmd = vhr->bufferedcmd;
        head->writecmd.cmd = (type & 0x01) | 0x02;
        VGM_Head_doMapping(head, &(head->writecmd));
    }else if(type >= 0x80 && type <= 0x8F){
        //OPN2 DAC write
        head->iswrite = 1;
        head->writecmd.cmd = 0x02;
        head->writecmd.addr = 0x2A;
        head->writecmd.data = vhr->bufferedcmd.data;
        if(type != 0x80){
            //Wait next
            head->firstoftwo = 1;
        }
        VGM_Head_doMapping(head, &(head->writecmd));
    }else if(type >= 0x70 && type <= 0x7F){
        //Short wait
        head->iswait = 1;
        head->ticks += type - 0x6F;
    }else if(type == 0x61){
        //Long wait
        head->iswait = 1;
        head->ticks += vhr->bufferedcmd.data | ((u32)vhr->bufferedcmd.data2 << 8);
    }else if(type == 0x62){
        //60 Hz wait
        head->iswait = 1;
        head->ticks += VGM_DELAY62;
    }else if(type == 0x63){
        //50 Hz wait
        head->iswait = 1;
        head->ticks += VGM_DELAY63;
    }else{
        //Unsupported command, should not be here
        head->iswait = 1;
    }
}

VgmSource* VGM_SourceRAM_Create(){
    VgmSource* source = vgmh2_malloc(sizeof(VgmSource));
    source->type = VGM_SOURCE_TYPE_RAM;
    source->mutes = 0;
    source->opn2clock = 7670454;
    source->psgclock = 3579545;
    source->loopaddr = 0xFFFFFFFF;
    source->loopsamples = 0;
    source->usage.all = 0;
    VgmSourceRAM* vsr = vgmh2_malloc(sizeof(VgmSourceRAM));
    source->data = vsr;
    vsr->cmds = NULL;
    vsr->numcmds = 0;
    return source;
}
void VGM_SourceRAM_Delete(void* sourceram){
    VgmSourceRAM* vsr = (VgmSourceRAM*)sourceram;
    if(vsr->cmds != NULL){
        vgmh2_free(vsr->cmds);
    }
    vgmh2_free(vsr);
}

void VGM_SourceRAM_UpdateUsage(VgmSource* source){
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    VgmUsageBits usage = (VgmUsageBits){.all = 0};
    u32 a;
    VgmChipWriteCmd cmd;
    u8 lfomode = 0, lfospeed = 0;
    for(a=0; a<vsr->numcmds; ++a){
        cmd = vsr->cmds[a];
        if(cmd.cmd == 0x50){
            //PSG write
            usage.all |= (1 << (24 + ((cmd.data >> 5) & 3)));
            if((cmd.data & 0x63) == 0x63){
                usage.noisefreqsq3 = 1;
            }
        }else if((cmd.cmd & 0xFE) == 0x52){
            //OPN2 write
            u8 chan, addrhi = (cmd.cmd & 1)*3;
            if(cmd.addr <= 0x2F){
                if(cmd.addr < 0x20 || addrhi) continue;
                //OPN2 global register
                if(cmd.addr == 0x28){
                    //Key On register
                    chan = cmd.data & 0x07;
                    if((chan & 0x03) == 0x03) continue; //Make sure not writing to 0x03 or 0x07
                    if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                    usage.all |= 1 << (chan);
                }else if(cmd.addr == 0x24 || cmd.addr == 0x25 || cmd.addr == 0x27){
                    usage.fm3 = 1;
                    usage.fm3_special = 1;
                }else if(cmd.addr == 0x2A || cmd.addr == 0x2B){
                    usage.dac = 1;
                }else if(cmd.addr == 0x22){
                    if(lfomode == 0){
                        if(cmd.data & 0x08){ //Command is to turn on the LFO; ignore commands to turn it off, unless it was already turned on
                            lfomode = 1;
                            lfospeed = cmd.data & 0x07;
                        }
                    }else if(lfomode == 1){
                        if(cmd.data != (0x08 | lfospeed)){ //It was turned off or changed speed
                            lfomode = 2;
                        }
                    }
                }else if(cmd.addr == 0x21 || cmd.addr == 0x2C){
                    usage.opn2_globals = 1;
                }
            }else if(cmd.addr <= 0x9F){
                //Operator command
                chan = (cmd.addr & 0x03);
                if(chan == 0x03) continue; //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                usage.all |= 1 << (chan);
            }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
                //Channel 3 extra frequency
                usage.fm3 = 1;
                usage.fm3_special = 1;
            }else if(cmd.addr <= 0xB6){
                //Channel command
                chan = (cmd.addr & 0x03);
                if(chan == 0x03) continue; //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                usage.all |= 1 << (chan);
                if((cmd.addr & 0xFC) == 0xB4){
                    if((cmd.data & 0x37) != 0){
                        //LFO-Frq or LFO-Amp enabled
                        usage.all |= 1 << (chan+6);
                    }
                }
            }
        }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
            //DAC write and wait
            usage.dac = 1;
        }
    }
    if(lfomode == 1 || ((usage.all & 0x00000FC0) && lfomode == 0)){
        usage.lfofixed = 1;
        usage.lfofixedspeed = lfospeed;
    }else if(lfomode == 2){
        usage.lfofixed = 0;
        //Make sure all used channels are marked as using LFO, so the chip thinks it's using LFO
        usage.all |= (usage.all & 0x0000003F) << 6;
    }
    source->usage.all = usage.all;
    DBG("Key:   ---QN321-----SpdX037654321654321");
    DBG("Usage: %32b", source->usage.all);
}

void VGM_SourceRAM_InsertCmd(VgmSource* source, u32 addr, VgmChipWriteCmd newcmd){
    MIOS32_IRQ_Disable();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    //Allocate additional memory
    vsr->cmds = vgmh2_realloc(vsr->cmds, (vsr->numcmds+1)*sizeof(VgmChipWriteCmd));
    if(vsr->cmds == NULL){
        DBG("Out of memory trying to enlarge VgmSourceRAM! Crashing soon!");
        vsr->numcmds = 0;
        MIOS32_IRQ_Enable();
        return;
    }
    //Move later data
    u32 a;
    for(a=vsr->numcmds; a>addr; --a){
        vsr->cmds[a].all = vsr->cmds[a-1].all;
    }
    //Insert new data
    vsr->cmds[addr] = newcmd;
    //Change length
    ++vsr->numcmds;
    //Move any heads playing this forward by one command
    VgmHead* head;
    VgmHeadRAM* vhr;
    for(a=0; a<VGM_HEAD_MAXNUM; ++a){
        head = vgm_heads[a];
        if(head != NULL && head->source == source){
            vhr = (VgmHeadRAM*)head->data;
            if(vhr->srcaddr >= addr) ++vhr->srcaddr;
        }
    }
    MIOS32_IRQ_Enable();
}
void VGM_SourceRAM_DeleteCmd(VgmSource* source, u32 addr){
    MIOS32_IRQ_Disable();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    //Move later data
    u32 a;
    for(a=addr; a<vsr->numcmds-1; ++a){
        vsr->cmds[a].all = vsr->cmds[a+1].all;
    }
    //Change length
    --vsr->numcmds;
    //Deallocate extra memory
    vsr->cmds = vgmh2_realloc(vsr->cmds, vsr->numcmds*sizeof(VgmChipWriteCmd));
    //Move any heads playing this backward by one command
    VgmHead* head;
    VgmHeadRAM* vhr;
    for(a=0; a<VGM_HEAD_MAXNUM; ++a){
        head = vgm_heads[a];
        if(head != NULL && head->source == source){
            vhr = (VgmHeadRAM*)head->data;
            if(vhr->srcaddr > addr) --vhr->srcaddr;
        }
    }
    MIOS32_IRQ_Enable();
}

static void PlayCommandNow(VgmHead* head, VgmSourceRAM* vsr, VgmHeadRAM* vhr, VgmChipWriteCmd cmd){
    u8 type = cmd.cmd;
    if(type == 0x50){
        //PSG write
        if((cmd.data & 0x80) && !(cmd.data & 0x10) && (cmd.data < 0xE0)){
            //It's a main write, not attenuation, and not noise--i.e. frequency
            VGM_fixPSGFrequency(&cmd, head->psgmult, head->psgfreq0to1);
        }
        cmd.cmd = 0x00;
        VGM_Head_doMapping(head, &cmd);
    }else if((type & 0xFE) == 0x52){
        //OPN2 write
        if((cmd.addr & 0xF4) == 0xA4){
            //Frequency MSB write
            VGM_fixOPN2Frequency(&(cmd), head->opn2mult);
        }
        cmd.cmd = (type & 0x01) | 0x02;
        VGM_Head_doMapping(head, &cmd);
    }else if(type >= 0x80 && type <= 0x8F){
        //OPN2 DAC write
        cmd.cmd = 0x02;
        cmd.addr = 0x2A;
        cmd.data = cmd.data;
        VGM_Head_doMapping(head, &cmd);
    }else{
        return;
    }
    VGM_Tracker_Enqueue(cmd, 0);
}

void VGM_HeadRAM_Forward1(VgmHead* head){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    //Drop the second half of a command, if buffered
    head->firstoftwo = 0;
    if(vhr->srcaddr >= vsr->numcmds){
        //Don't loop back
        head->isdone = 1;
        return;
    }
    //Play the command now
    PlayCommandNow(vsr->cmds[vhr->srcaddr]);
    //Prepare the next command
    VGM_HeadRAM_cmdNext(head, VGM_Player_GetVGMTime());
}
void VGM_HeadRAM_Backward1(VgmHead* head){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    //Drop the second half of a command, if buffered
    head->firstoftwo = 0;
    if(vhr->srcaddr <= 1){
        //Don't go backwards from the beginning
        return;
    }
    //Go back two commands
    vhr->srcaddr -= 2;
    VgmChipWriteCmd curcmd = vsr->cmds[vhr->srcaddr];
    //Find the most recent command before this one, which this one overwrote the state of
    s32 a; u8 flag = 0;
    VgmChipWriteCmd oldcmd;
    if(curcmd.cmd >= 0x80 && curcmd.cmd <= 0x8F) curcmd.cmd = 0x52; //Get rid of wait part of DAC write
    if(curcmd.cmd == 0x50){
        for(a=vhr->srcaddr-1; a>=0; --a){
            oldcmd = vsr->cmds[a];
            //Has to be PSG Write command
            if(oldcmd.cmd != 0x50) continue;
            //Has to be the same address
            if((oldcmd.data & 0xF0) != (curcmd.data & 0xF0)) continue;
            //Got it
            PlayCommandNow(oldcmd);
            flag = 1;
            break;
        }
        if(!flag){
            //This was the first command to modify this, reset to initial state
            if(curcmd.data & 0x10){
                //Volume, set to 1111
                curcmd.data |= 0x0F;
            }else if((curcmd.data & 0x60) == 0x60){
                //PSG control, set to 0000
                curcmd.data &= 0xF0;
            }else{
                //Frequency, set to 0
                curcmd.data &= 0xF0;
                curcmd.data2 = 0x00;
            }
            PlayCommandNow(curcmd);
        }
    }else if((curcmd.cmd & 0xFE) == 0x52){
        for(a=vhr->srcaddr-1; a>=0; --a){
            oldcmd = vsr->cmds[a];
            if(oldcmd.cmd >= 0x80 && oldcmd.cmd <= 0x8F) oldcmd.cmd = 0x52; //Get rid of wait part of DAC write
            //Has to be OPN2 Write command with the same addrhi
            if(oldcmd.cmd != curcmd.cmd) continue;
            //Has to be the same address
            if(oldcmd.addr != curcmd.addr) continue;
            //Got it
            PlayCommandNow(oldcmd);
            flag = 1;
            break;
        }
        if(!flag){
            //This was the first command to modify this, reset to initial state
            if(curcmd.addr == 0x28){
                //Key ons, turn off
                curcmd.data &= 0x0F;
            }else if((curcmd.addr & 0xFC) == 0xB4){
                //Output bits turn on
                curcmd.data = 0xC0;
            }else{
                //All other registers initialized to 0
                curcmd.data = 0x00;
                curcmd.data2 = 0x00;
            }
            PlayCommandNow(curcmd);
        }
    }
    //Prepare the next command
    VGM_HeadRAM_cmdNext(head, VGM_Player_GetVGMTime());
}
void VGM_HeadRAM_SeekTo(VgmHead* head, u32 newaddr){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    while(newaddr < (vhr->srcaddr-1)){
        VGM_HeadRAM_Backward1(head);
    }
    while((newaddr > (vhr->srcaddr-1)) && !head->isdone){
        VGM_HeadRAM_Forward1(head);
    }
}

static u32 GetWaitTime(VgmChipWriteCmd cmd){
    if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        return cmd.cmd - 0x80;
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        return cmd.cmd - 0x6F;
    }else if(cmd.cmd == 0x61){
        return (cmd.data | ((u32)cmd.data2 << 8));
    }else if(cmd.cmd == 0x62){
        return VGM_DELAY62;
    }else if(cmd.cmd == 0x63){
        return VGM_DELAY63;
    }else{
        return 0;
    }
}

void VGM_HeadRAM_ForwardState(VgmHead* head, u32 maxt, u32 maxdt, u8 allowstay){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    VgmChipWriteCmd cmd;
    if(vhr->srcaddr >= vsr->numcmds) return;
    if(allowstay)
    u32 totalt = 0, thist;
    while(vhr->srcaddr < vsr->numcmds){
        cmd = vsr->cmds[vhr->srcaddr];
        thist = GetWaitTime(cmd);
        totalt += thist;
        if(thist >= maxdt || totalt >= maxt) return;
        ++vhr->srcaddr;
    }
}

void VGM_HeadRAM_BackwardState(VgmHead* head, u32 maxt, u32 maxdt){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    VgmChipWriteCmd cmd;
    if(vhr->srcaddr == 0) return;
    --vhr->srcaddr;
    u32 totalt = 0, thist;
    while(vhr->srcaddr > 0){
        cmd = vsr->cmds[vhr->srcaddr];
        thist = GetWaitTime(cmd);
        totalt += thist;
        if(thist >= maxdt || totalt >= maxt) return;
        --vhr->srcaddr;
    }
}


