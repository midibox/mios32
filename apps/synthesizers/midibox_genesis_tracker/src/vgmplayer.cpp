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

// timers clocked at CPU/2 clock
#define TIM_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/2)

//vgmp_chipdata chipdata[GENESIS_COUNT];

// For this test software, there's only one head.
VgmHead* head;

void VgmPlayer_AddHead(VgmHead* vgmh){
    head = vgmh;
}
void VgmPlayer_RemoveHead(VgmHead* vgmh){
    head = NULL;
}

// There are 1905 hr_ticks per VGM sample.
// Check all VGMs again at least each 10 samples, i.e. 19048 hr_ticks.
#define VGMP_HRTICKSPERSAMPLE 1905
#define VGMP_MAXDELAY 19048
#define VGMP_CHIPBUSYDELAY 850

// Where all the work gets done.
void TIM5_IRQHandler(void){
    if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
        TIM_Cmd(TIM5, DISABLE);
        ////////////////////////////////////////////////////////////////////////
        // PLAY VGMS
        ////////////////////////////////////////////////////////////////////////
        VgmHead* h;
        u16 vgm_time = TIM3->CNT;
        s32 minvgmwaitremaining = 65535; s32 s;
        ChipWriteCmd cmd;
        u8 chipwasbusy = 0, wrotetochip;
        u16 nextdelay;
        //Scan all VGMs for delays
        if(head != NULL){
            h = head;
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
            if(head != NULL){
                h = head;
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
        TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
        TIM_TimeBaseStructure.TIM_Period = (nextdelay-1);
        TIM_TimeBaseStructure.TIM_Prescaler = 0; //maximum speed
        TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
        TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
        TIM5->CNT = 0; //just to make sure
        TIM_Cmd(TIM5, ENABLE);
    }
}


void VgmPlayer_Init(){
    head = NULL;
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 2: hr_time, max resolution (168 MHz / 2 = 84 MHz)
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE); //Disable interrupts
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 65535; //maximum length
    TIM_TimeBaseStructure.TIM_Prescaler = 0; //maximum speed
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM2, ENABLE); //Start counting!
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 3: VGM Sample Clock 44.1 kHz
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE); //Disable interrupts
    TIM_TimeBaseStructure.TIM_Period = 65535; //maximum length
    TIM_TimeBaseStructure.TIM_Prescaler = (VGMP_HRTICKSPERSAMPLE-1); //(TIM_PERIPHERAL_FRQ/44100)-1; //44.1 kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM3, ENABLE); //Start counting!
    ////////////////////////////////////////////////////////////////////////////
    // Setup Timer 5: Work Timer, hr_ticks also, interrupts, variable length
    ////////////////////////////////////////////////////////////////////////////
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //Enable timer clock
    TIM_ITConfig(TIM5, TIM_IT_Update, DISABLE); //Disable interrupts for now
    TIM_TimeBaseStructure.TIM_Period = (VGMP_MAXDELAY-1); //Will be changed
    TIM_TimeBaseStructure.TIM_Prescaler = 0; //maximum speed
    TIM_TimeBaseStructure.TIM_ClockDivision = 0; //maximum accuracy
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //normal
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE); //Enable interrupts
    MIOS32_IRQ_Install(TIM5_IRQn, MIOS32_IRQ_PRIO_LOW); //MIOS32_IRQ_PRIO_INSANE);
    TIM_Cmd(TIM5, ENABLE); //Start counting!
}


