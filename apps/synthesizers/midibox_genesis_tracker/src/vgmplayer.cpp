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

#define USE_GENESIS 0

vgmp_chipdata chipdata[GENESIS_COUNT];

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
    /*
    if(hr_time % 10000 == 0){
        DBG("WorkCallback at hr_time=%d, vgm_time=%d", hr_time, vgm_time);
    }
    */
    u8 leds = MIOS32_BOARD_LED_Get();
    MIOS32_BOARD_LED_Set(0b1111, 0b0010);
    VgmHead* h;
    u32 minwait = 0xFFFFFFFF; s32 s; u32 u;
    ChipWriteCmd cmd;
    u8 wrotetochip;
    //Scan all VGMs for delays
    if(vgmp_head != NULL){
        h = vgmp_head;
        //Check for delay
        if(h->cmdIsWait()){
            s = h->cmdGetWaitRemaining(vgm_time);
            if(s <= 0){
                //Advance to next command
                h->cmdNext(vgm_time);
            }else{
                u = s * VGMP_HRTICKSPERSAMPLE;
                if(u < minwait){
                    //Store it as the shortest remaining time
                    minwait = u;
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
            //Check for command
            if(h->cmdIsChipWrite()){
                cmd = h->cmdGetChipWrite();
                if(cmd.cmd == 0x50){
                    //PSG write
                    u = hr_time - chipdata[USE_GENESIS].psg_lastwritetime;
                    if(u < VGMP_PSGBUSYDELAY){
                        u = VGMP_PSGBUSYDELAY - u;
                        if(u < minwait){
                            minwait = u;
                        }
                    }else{
                        Genesis_PSGWrite(USE_GENESIS, cmd.data);
                        h->cmdNext(vgm_time);
                        chipdata[USE_GENESIS].psg_lastwritetime = hr_time;
                        wrotetochip = 1;
                    }
                    /*
                    if(Genesis_CheckPSGBusy(0)){
                        if(VGMP_PSGBUSYDELAY < minwait){
                            minwait = VGMP_PSGBUSYDELAY;
                        }
                    }else{
                        Genesis_PSGWrite(0, cmd.data);
                        h->cmdNext(vgm_time);
                        wrotetochip = 1;
                    }
                    */
                }else if((cmd.cmd & 0xFE) == 0x52){
                    //OPN2 write
                    u = hr_time - chipdata[USE_GENESIS].opn2_lastwritetime;
                    if(u < VGMP_OPN2BUSYDELAY){
                        u = VGMP_OPN2BUSYDELAY - u;
                        if(u < minwait){
                            minwait = u;
                        }
                    }else{
                        Genesis_OPN2Write(USE_GENESIS, (cmd.cmd & 0x01), cmd.addr, cmd.data);
                        h->cmdNext(vgm_time);
                        //Don't delay after 0x2x commands
                        if(cmd.addr >= 0x20 && cmd.addr < 0x2F && cmd.addr != 0x28){
                            chipdata[USE_GENESIS].opn2_lastwritetime = hr_time - VGMP_OPN2BUSYDELAY;
                        }else{
                            chipdata[USE_GENESIS].opn2_lastwritetime = hr_time;
                        }
                        wrotetochip = 1;
                    }
                }
            }
        }
    }
    //Set up next delay
    if(minwait < 100){
        //TODO loop instead of returning and re-timing
        minwait = 100;
    }else if(minwait > VGMP_MAXDELAY){
        minwait = VGMP_MAXDELAY;
    }
    //minwait = 1000;
    MIOS32_BOARD_LED_Set(0b1111, leds);
    return minwait;
}


void VgmPlayer_Init(){
    vgmp_head = NULL;
    VgmPlayerLL_RegisterCallback(VgmPlayer_WorkCallback);
    VgmPlayerLL_Init();
}


