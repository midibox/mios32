/*
 * MIDIbox Genesis Tracker: VGM Player
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
#include "vgmplayer.h"

#include <genesis.h>

#include "vgmplayer_ll.h"


//vgmp_chipdata chipdata[GENESIS_COUNT];

// For this test software, there's only one head.
VgmHead* vgmp_head;

void VgmPlayer_AddHead(VgmHead* vgmh){
    vgmp_head = vgmh;
    //Start it!
    vgmp_head->cmdNext(VgmPlayerLL_GetVGMTime());
}
void VgmPlayer_RemoveHead(VgmHead* vgmh){
    vgmp_head = NULL;
}

// Where all the work gets done.
u16 VgmPlayer_WorkCallback(u32 hr_time, u32 vgm_time){
    ////////////////////////////////////////////////////////////////////////
    // PLAY VGMS
    ////////////////////////////////////////////////////////////////////////
    if(hr_time % 10000 == 0){
        DBG("WorkCallback at hr_time=%d, vgm_time=%d", hr_time, vgm_time);
    }
    VgmHead* h;
    s32 minvgmwaitremaining = 65535; s32 s;
    ChipWriteCmd cmd;
    u8 chipwasbusy = 0, wrotetochip;
    u16 nextdelay;
    //Scan all VGMs for delays
    if(vgmp_head != NULL){
        h = vgmp_head;
        if(!h->isPaused()){
            //Check for delay
            if(h->cmdIsWait()){
                s = h->cmdGetWaitRemaining(vgm_time);
                if(s < 0){
                    //Advance to next command
                    h->cmdNext(vgm_time);
                }else if(s < minvgmwaitremaining){
                    //Store it as the shortest remaining time
                    minvgmwaitremaining = s;
                }
            }
        }
    }
    //Scan all VGMs for pending chip write commands
    wrotetochip = 1;
    while(wrotetochip){
        wrotetochip = 0;
        if(vgmp_head != NULL){
            h = vgmp_head;
            if(!h->isPaused()){
                //Check for command
                if(h->cmdIsChipWrite()){
                    cmd = h->cmdGetChipWrite();
                    if(cmd.cmd == 0x50){
                        //PSG write
                        //TODO better timing scheme, multiple chips
                        if(Genesis_CheckPSGBusy(0)){
                            chipwasbusy = 1;
                        }else{
                            Genesis_PSGWrite(0, cmd.data);
                            h->cmdNext(vgm_time);
                            wrotetochip = 1;
                        }
                    }else if((cmd.cmd & 0xFE) == 0x52){
                        //OPN2 write
                        //TODO better timing scheme, multiple chips
                        if(Genesis_CheckOPN2Busy(0)){
                            chipwasbusy = 1;
                        }else{
                            Genesis_OPN2Write(0, (cmd.cmd & 0x01), cmd.addr, cmd.data);
                            h->cmdNext(vgm_time);
                            wrotetochip = 1;
                        }
                    }
                }
            }
        }
    }
    //Set up next delay
    if(chipwasbusy){
        nextdelay = VGMP_CHIPBUSYDELAY;
    }else{
        if(minvgmwaitremaining < 10){
            nextdelay = minvgmwaitremaining * VGMP_HRTICKSPERSAMPLE;
        }else{
            nextdelay = VGMP_MAXDELAY;
        }
    }
    return nextdelay;
}


void VgmPlayer_Init(){
    vgmp_head = NULL;
    VgmPlayerLL_RegisterCallback(VgmPlayer_WorkCallback);
    VgmPlayerLL_Init();
}


