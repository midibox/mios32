/*
 * MIDIbox Genesis Tracker: VGM Player Low-Level (C) functions
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
#include "vgmplayer_ll.h"

// timers clocked at CPU/2 clock
#define TIM_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/2)

static u16 (*_vgm_work)(u32 hr_time, u32 vgm_time);

u32 VgmPlayerLL_GetHRTime(){
    return TIM2->CNT;
}
u32 VgmPlayerLL_GetVGMTime(){
    return TIM5->CNT;
}

void TIM3_IRQHandler(void){
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM_Cmd(TIM3, DISABLE);
        u16 nextdelay = _vgm_work(TIM2->CNT, TIM5->CNT);
        TIM3->CNT = 0;
        TIM3->ARR = nextdelay;
        TIM_Cmd(TIM3, ENABLE);
    }
}

void VgmPlayerLL_RegisterCallback(u16 (*_vgm_work_callback)(u32 hr_time, u32 vgm_time)){
    _vgm_work = _vgm_work_callback;
}

void VgmPlayerLL_Init(){
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
    MIOS32_IRQ_Install(TIM3_IRQn, MIOS32_IRQ_PRIO_LOW); //MIOS32_IRQ_PRIO_INSANE);
    TIM_Cmd(TIM3, ENABLE); //Start counting!
}


