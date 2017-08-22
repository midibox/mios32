/*
 * VGM Data and Playback Driver: VGM RAM Block Data
 *
 * ==========================================================================
 *
 *  Copyright (C) 2017 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmram.h"
#include "vgmhead.h"
#include "vgmplayer.h"
#include "vgmtracker.h"
#include "vgmtuning.h"
#include "vgm_heap2.h"
#include <genesis.h>

VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source){
    //VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    VgmHeadRAM* vhr = vgmh2_malloc(sizeof(VgmHeadRAM));
    vhr->bufferedcmd.all = 0;
    return vhr;
}
void VGM_HeadRAM_Delete(void* headram){
    VgmHeadRAM* vhr = (VgmHeadRAM*)headram;
    vgmh2_free(vhr);
}

void VGM_HeadRAM_InternalCmdNext(VgmHead* head, VgmSourceRAM* vsr, VgmHeadRAM* vhr){
    VgmChipWriteCmd cmd = vsr->cmds[head->srcaddr];
    VGM_Head_doTransformations(head, &cmd);
    head->firstoftwo = VGM_Cmd_UnpackTwoByte(cmd, &cmd, &(vhr->bufferedcmd));
    VGM_Head_setWritecmd(head, cmd);
    VGM_Head_doMapping(head, &(head->writecmd));
}
void VGM_HeadRAM_Restart(VgmHead* head){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    head->srcaddr = head->source->markstart;
    head->isdone = 0;
    VGM_HeadRAM_InternalCmdNext(head, vsr, vhr);
}
void VGM_HeadRAM_cmdNext(VgmHead* head, u32 vgm_time){
    if(head->isdone) return;
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    head->iswait = 0;
    head->iswrite = 0;
    if(head->firstoftwo){
        head->firstoftwo = 0;
        VGM_Head_setWritecmd(head, vhr->bufferedcmd);
        VGM_Head_doMapping(head, &(head->writecmd));
        return;
    }
    //Read new command
    ++head->srcaddr;
    if(head->srcaddr > head->source->markend){
        //Done with marked region
        head->isdone = 1;
        head->srcaddr = head->source->markend+1;
        return;
    }else if(head->srcaddr >= vsr->numcmds){
        //End of data
        if(head->source->loopaddr < vsr->numcmds){
            //Jump to loop point
            head->srcaddr = head->source->loopaddr;
        }else{
            //Behaves like endless stream of 65535-tick waits
            head->isdone = 1;
            head->srcaddr = vsr->numcmds;
            return;
        }
    }
    VGM_HeadRAM_InternalCmdNext(head, vsr, vhr);
}

VgmSource* VGM_SourceRAM_Create(){
    VgmSource* source = vgmh2_malloc(sizeof(VgmSource));
    source->type = VGM_SOURCE_TYPE_RAM;
    source->mutes = 0;
    source->opn2clock = genesis_clock_opn2;
    source->psgclock = genesis_clock_psg;
    source->psgfreq0to1 = 1;
    source->loopaddr = 0xFFFFFFFF;
    source->loopsamples = 0;
    source->markstart = 0;
    source->markend = 0xFFFFFFFF;
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
    source->usage.all = 0;
    u32 a;
    for(a=0; a<vsr->numcmds; ++a){
        VGM_Cmd_UpdateUsage(&source->usage, vsr->cmds[a]);
    }
    VGM_Cmd_DebugPrintUsage(source->usage);
}

void VGM_SourceRAM_InsertCmd(VgmSource* source, u32 addr, VgmChipWriteCmd newcmd){
    MIOS32_IRQ_Disable();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    if(addr > vsr->numcmds) addr = vsr->numcmds;
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
    if(source->markstart >= addr && addr > 0) source->markstart++;
    if(source->markend >= addr && source->markend < 0xFFFFFFFF) source->markend++;
    //Move any heads playing this forward by one command
    VgmHead* head;
    for(a=0; a<VGM_HEAD_MAXNUM; ++a){
        head = vgm_heads[a];
        if(head != NULL && head->source == source){
            if(head->srcaddr >= addr && addr > 0) ++head->srcaddr;
        }
    }
    MIOS32_IRQ_Enable();
}
void VGM_SourceRAM_DeleteCmd(VgmSource* source, u32 addr){
    MIOS32_IRQ_Disable();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    if(addr >= vsr->numcmds) return;
    //Move later data
    u32 a;
    for(a=addr; a<vsr->numcmds-1; ++a){
        vsr->cmds[a].all = vsr->cmds[a+1].all;
    }
    //Change length
    --vsr->numcmds;
    if(source->markstart > addr) source->markstart--;
    if(source->markend > addr && source->markend < 0xFFFFFFFF) source->markend--;
    //Deallocate extra memory
    vsr->cmds = vgmh2_realloc(vsr->cmds, vsr->numcmds*sizeof(VgmChipWriteCmd));
    //Move any heads playing this backward by one command
    VgmHead* head;
    for(a=0; a<VGM_HEAD_MAXNUM; ++a){
        head = vgm_heads[a];
        if(head != NULL && head->source == source){
            if(head->srcaddr > addr) --head->srcaddr;
        }
    }
    MIOS32_IRQ_Enable();
}

//Turn DAC and Wait command into regular OPN2 chip write to DAC
#define FixDACWrite(ARG) do{if((ARG).cmd >= 0x80 && (ARG).cmd <= 0x8F) {(ARG).cmd = 0x52; (ARG).addr = 0x2A;} } while(0)

static void PlayCommandNow(VgmHead* head, VgmSourceRAM* vsr, VgmHeadRAM* vhr, VgmChipWriteCmd cmd){
    if(!VGM_Cmd_IsWrite(cmd)) return;
    VGM_Head_doTransformations(head, &cmd);
    FixDACWrite(cmd);
    VGM_Head_doMapping(head, &cmd);
    VGM_Tracker_Enqueue(cmd, 0);
}

s32 VGM_HeadRAM_Forward1(VgmHead* head){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    //Are we already off the end?
    if(head->srcaddr >= vsr->numcmds){
        DBG("VGM_HeadRAM_Forward1 tried to go forward off the end (%d)!", vsr->numcmds);
        return -1;
    }
    //Play the current command, which should be buffered in head->writecmd
    PlayCommandNow(head, vsr, vhr, vsr->cmds[head->srcaddr]);
    //Forward one command
    ++head->srcaddr;
    //If we would now be going off the end, don't loop back
    if(head->srcaddr == vsr->numcmds){
        //Don't loop back
        head->isdone = 1;
        return 0;
    }
    //Otherwise, prepare the next command
    VGM_HeadRAM_InternalCmdNext(head, vsr, vhr);
    return 0;
}
s32 VGM_HeadRAM_Backward1(VgmHead* head){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)head->data;
    if(head->srcaddr == 0){
        //Don't go backwards from the beginning
        DBG("VGM_HeadRAM_Backward1 tried to go backward from 0!");
        return -1;
    }
    //Back one command
    --head->srcaddr;
    head->isdone = 0;
    VgmChipWriteCmd curcmd;
    curcmd.all = vsr->cmds[head->srcaddr].all;
    //Find the most recent command before this one, which this one overwrote the state of
    s32 a; u8 flag = 0;
    VgmChipWriteCmd oldcmd;
    FixDACWrite(curcmd);
    if(curcmd.cmd == 0x50){
        for(a=(s32)head->srcaddr-1; a>=0; --a){
            oldcmd = vsr->cmds[a];
            //Has to be PSG Write command
            if(oldcmd.cmd != 0x50) continue;
            //Has to be the same address
            if((oldcmd.data & 0xF0) != (curcmd.data & 0xF0)) continue;
            //Got it
            PlayCommandNow(head, vsr, vhr, oldcmd);
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
            PlayCommandNow(head, vsr, vhr, curcmd);
        }
    }else if((curcmd.cmd & 0xFE) == 0x52){
        for(a=(s32)head->srcaddr-1; a>=0; --a){
            oldcmd = vsr->cmds[a];
            FixDACWrite(oldcmd);
            //Has to be OPN2 Write command with the same addrhi
            if(oldcmd.cmd != curcmd.cmd) continue;
            //Has to be the same address
            if(oldcmd.addr != curcmd.addr) continue;
            //Got it
            PlayCommandNow(head, vsr, vhr, oldcmd);
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
            PlayCommandNow(head, vsr, vhr, curcmd);
        }
    }
    //Prepare the next command
    VGM_HeadRAM_InternalCmdNext(head, vsr, vhr);
    return 0;
}
s32 VGM_HeadRAM_SeekTo(VgmHead* head, u32 newaddr){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    if(newaddr > vsr->numcmds){
        DBG("VGM_HeadRAM: Trying to seek to 0x%X but sequence is only 0x%X long!", newaddr, vsr->numcmds);
        return -1;
    }
    while(head->srcaddr > newaddr){
        VGM_HeadRAM_Backward1(head);
    }
    while(head->srcaddr < newaddr){
        VGM_HeadRAM_Forward1(head);
    }
    return 0;
}


s32 VGM_HeadRAM_ForwardState(VgmHead* head, u32 maxt, u32 maxdt, u8 allowstay){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    if(head->srcaddr >= vsr->numcmds){
        return -1;
    }
    VgmChipWriteCmd cmd;
    u8 state = 0;
    /*
    State 0: skipping time commands we were pointing to
    State 1: skipping chip write commands and short time commands
    Stop when long time command reached
    */
    if(allowstay) state = 1;
    u32 totalt = 0, thist;
    u32 origsrcaddr = head->srcaddr;
    while(head->srcaddr < vsr->numcmds){
        cmd = vsr->cmds[head->srcaddr];
        thist = VGM_Cmd_GetWaitValue(cmd);
        if(state == 0){
            if(thist == 0 || (cmd.cmd >= 0x80 && cmd.cmd <= 0x8F)){
                //It's a chip write command
                state = 1;
                totalt = 0;
            }
        }else{
            totalt += thist;
            if(thist >= maxdt || totalt >= maxt) break;
        }
        VGM_HeadRAM_Forward1(head);
    }
    if(origsrcaddr == head->srcaddr){
        return -2;
    }else{
        return 0;
    }
}

s32 VGM_HeadRAM_BackwardState(VgmHead* head, u32 maxt, u32 maxdt){
    VgmSourceRAM* vsr = (VgmSourceRAM*)head->source->data;
    if(head->srcaddr == 0){
        return -1;
    }
    VgmChipWriteCmd cmd;
    u8 state = 0;
    /*
    State 0: skipping chip write commands and short time commands
    State 1: skipping long time commands
    */
    u32 totalt = 0, thist;
    u32 origsrcaddr = head->srcaddr;
    while(head->srcaddr > 0){
        cmd = vsr->cmds[head->srcaddr-1];
        thist = VGM_Cmd_GetWaitValue(cmd);
        if(state == 0){
            totalt += thist;
            if(thist >= maxdt || totalt >= maxt){
                state = 1;
            }
        }else{
            if(thist == 0 || (cmd.cmd >= 0x80 && cmd.cmd <= 0x8F)){
                //It's a chip write command
                break;
            }
        }
        VGM_HeadRAM_Backward1(head);
    }
    if(origsrcaddr == head->srcaddr){
        return -2;
    }else{
        return 0;
    }
}


