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

#ifndef _VGMPLAYERLL_H
#define _VGMPLAYERLL_H

#ifdef __cplusplus
extern "C" {
#endif


// There are 1905 hr_ticks per VGM sample.
// Check all VGMs again at least each 10 samples, i.e. 19048 hr_ticks.
#define VGMP_HRTICKSPERSAMPLE 1905
#define VGMP_MAXDELAY 1000 //19048
//#define VGMP_CHIPBUSYDELAY 850
#define VGMP_PSGBUSYDELAY 672
#define VGMP_OPN2BUSYDELAY 2100 //1512 or 2016?


extern void VgmPlayerLL_Init();
extern void VgmPlayerLL_RegisterCallback(u16 (*_vgm_work_callback)(u32 hr_time, u32 vgm_time));

extern u32 VgmPlayerLL_GetHRTime();
extern u32 VgmPlayerLL_GetVGMTime();


#ifdef __cplusplus
}
#endif


#endif /* _VGMPLAYERLL_H */
