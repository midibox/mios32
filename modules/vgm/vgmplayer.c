/*
 * VGM Data and Playback Driver: VGM Player 
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "vgmplayer.h"
#include <mios32.h>
#include "vgmhead.h"
#include <genesis.h>
#include "vgmperfmon.h"


typedef struct {
    u32 opn2_lastwritetime;
    u32 psg_lastwritetime;
} vgmp_chipdata;

static vgmp_chipdata chipdata[GENESIS_COUNT];


u16 VgmPlayer_WorkCallback(u32 hr_time, u32 vgm_time){
    ////////////////////////////////////////////////////////////////////////
    // PLAY VGMS
    ////////////////////////////////////////////////////////////////////////
    u8 leds = MIOS32_BOARD_LED_Get();
    MIOS32_BOARD_LED_Set(0b1111, 0b0010);
    VgmHead* h;
    u32 minwait = 0xFFFFFFFF; s32 s; u32 u;
    VgmChipWriteCmd cmd;
    u8 wrotetochip;
    //Scan all VGMs for delays
    u8 i;
    for(i=0; i<vgm_numheads; ++i){
        h = vgm_heads[i];
        if(h != NULL && h->playing){
            //Check for delay
            if(VGM_Head_cmdIsWait(h)){
                s = VGM_Head_cmdGetWaitRemaining(h, vgm_time);
                if(s <= 0){
                    //Advance to next command
                    VGM_Head_cmdNext(h, vgm_time);
                }else{
                    u = s * VGMP_HRTICKSPERSAMPLE;
                    if(u < minwait){
                        //Store it as the shortest remaining time
                        minwait = u;
                    }
                }
            }
        }
    }
    //Scan all VGMs for pending chip write commands
    wrotetochip = 1;
    while(wrotetochip){
        wrotetochip = 0;
        for(i=0; i<vgm_numheads; ++i){
            h = vgm_heads[i];
            if(h != NULL && h->playing){
                //Check for command
                if(VGM_Head_cmdIsChipWrite(h)){
                    cmd = h->writecmd;
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
                            VGM_Head_cmdNext(h, vgm_time);
                            chipdata[USE_GENESIS].psg_lastwritetime = hr_time;
                            wrotetochip = 1;
                        }
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
                            VGM_Head_cmdNext(h, vgm_time);
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


void TIM3_IRQHandler(void){
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM_Cmd(TIM3, DISABLE);
        VGM_PerfMon_ClockIn(VGM_PERFMON_TASK_CHIP);
        u16 nextdelay = VgmPlayer_WorkCallback(TIM2->CNT, TIM5->CNT);
        VGM_PerfMon_ClockOut(VGM_PERFMON_TASK_CHIP);
        TIM3->CNT = 0;
        TIM3->ARR = nextdelay;
        TIM_Cmd(TIM3, ENABLE);
    }
}

// timers clocked at CPU/2 clock
//#define TIM_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/2)

void VGM_Player_Init(){
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 2: hr_time, max resolution (168 MHz / 2 = 84 MHz)
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); //Disable interrupts
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF; //maximum length
    TIM_TimeBaseStructure.TIM_Prescaler = 1; //maximum speed
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM2, ENABLE); //Start counting!
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 5: VGM Sample Clock 44.1 kHz
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM5, TIM_IT_Update, DISABLE); //Disable interrupts
    TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF; //maximum length
    TIM_TimeBaseStructure.TIM_Prescaler = (VGMP_HRTICKSPERSAMPLE-1); //(TIM_PERIPHERAL_FRQ/44100)-1; //44.1 kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM5, ENABLE); //Start counting!
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 3: Work Timer, hr_ticks also, interrupts, variable length
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE); //Disable interrupts for now
    TIM_TimeBaseStructure.TIM_Period = (VGMP_MAXDELAY-1); //Will be changed
    TIM_TimeBaseStructure.TIM_Prescaler = 1; //maximum speed
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_ARRPreloadConfig(TIM3, DISABLE);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); //Enable interrupts
    MIOS32_IRQ_Install(TIM3_IRQn, MIOS32_IRQ_PRIO_INSANE); //highest priority!
    TIM_Cmd(TIM3, ENABLE); //Start counting!
}




