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
#include "vgm_heap2.h"
#include "FreeRTOS.h"

static u32 timers[VGM_PERFMON_NUM_TASKS];
static u8 percents[VGM_PERFMON_NUM_TASKS];
static u32 last_time;

void VGM_PerfMon_ClockIn(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return;
    timers[task] -= VGM_Player_GetVGMTime();
}
void VGM_PerfMon_ClockOut(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return;
    timers[task] += VGM_Player_GetVGMTime();
}

void VGM_PerfMon_Periodic(){
    u8 i;
    u32 time = VGM_Player_GetVGMTime();
    for(i=0; i<VGM_PERFMON_NUM_TASKS; ++i){
        percents[i] = timers[i] * 100 / (time - last_time);
        timers[i] = 0;
    }
    last_time = time;
}
u8 VGM_PerfMon_GetTaskCPU(u8 task){
    if(task >= VGM_PERFMON_NUM_TASKS) return 0;
    return percents[task];
}

vgm_meminfo_t VGM_PerfMon_GetMemInfo(){
    vgm_meminfo_t ret;
    ret.main_total = configTOTAL_HEAP_SIZE >> 3;
    ret.main_used = xPortGetFreeHeapSize() >> 3;
    ret.vgmh2_total = VGMH2_NUMBLOCKS;
    ret.vgmh2_used = vgmh2_numusedblocks;
    return ret;
}
