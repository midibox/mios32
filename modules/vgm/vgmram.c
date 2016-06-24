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

VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source){
    //VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    VgmHeadRAM* vhr = malloc(sizeof(VgmHeadRAM));
    vhr->srcaddr = 0;
    return vhr;
}
void VGM_HeadRAM_Delete(void* headram){
    VgmHeadRAM* vhr = (VgmHeadRAM*)headram;
    free(vhr);
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
    VgmSource* source = malloc(sizeof(VgmSource));
    source->type = VGM_SOURCE_TYPE_RAM;
    source->opn2clock = 7670454;
    source->psgclock = 3579545;
    source->loopaddr = 0xFFFFFFFF;
    source->loopsamples = 0;
    VgmSourceRAM* vsr = malloc(sizeof(VgmSourceRAM));
    source->data = vsr;
    vsr->cmds = NULL;
    vsr->numcmds = 0;
    return source;
}
void VGM_SourceRAM_Delete(void* sourceram){
    VgmSourceRAM* vsr = (VgmSourceRAM*)sourceram;
    if(vsr->cmds != NULL){
        free(vsr->cmds);
    }
    free(vsr);
}
