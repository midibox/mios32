/*
 * VGM Data and Playback Driver: Performance Monitor
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmperfmon.h"
#include "vgmplayer.h"

static u32 timers[VGM_PERFMON_NUM_TASKS];
static u8 percents[VGM_PERFMON_NUM_TASKS];

void VGM_PerfMon_ClockIn(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return;
    timers[task] -= VGM_Player_GetVGMTime();
}
void VGM_PerfMon_ClockOut(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return;
    timers[task] += VGM_Player_GetVGMTime();
}

void VGM_PerfMon_Second(){
    u8 i;
    for(i=0; i<VGM_PERFMON_NUM_TASKS; ++i){
        percents[i] = timers[i] / 441; // time * 100 / 44100;
        timers[i] = 0;
    }
}
u8 VGM_PerfMon_GetTaskCPU(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return 0;
    return percents[task];
}
