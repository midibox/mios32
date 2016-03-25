/*
 * VGM Data and Playback Driver: Performance Monitor header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMPERFMON_H
#define _VGMPERFMON_H

#include <mios32.h>

#define VGM_PERFMON_NUM_TASKS 2
#define VGM_PERFMON_TASK_CHIP 0
#define VGM_PERFMON_TASK_CARD 1


extern void VGM_PerfMon_ClockIn(u8 task);
extern void VGM_PerfMon_ClockOut(u8 task);

extern void VGM_PerfMon_Second();
extern u8 VGM_PerfMon_GetTaskCPU(u8 task);


#endif /* _VGMPERFMON_H */
