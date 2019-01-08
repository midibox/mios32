/*
 * VGM Data and Playback Driver: VGM Player header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMPLAYER_H
#define _VGMPLAYER_H

#include <mios32.h>

#define USE_GENESIS 3 //TODO

// There are 1905 hr_ticks per VGM sample.
// Check all VGMs again at least each 10 samples, i.e. 19048 hr_ticks.
#define VGMP_HRTICKSPERSAMPLE 1905
#define VGMP_MAXDELAY 1000 //19048
//#define VGMP_CHIPBUSYDELAY 850
#define VGMP_PSGBUSYDELAY 672
#define VGMP_OPN2BUSYDELAY 2100 //1512 or 2016?


static inline u32 VGM_Player_GetHRTime() { return TIM2->CNT; }
static inline u32 VGM_Player_GetVGMTime() { return TIM5->CNT; }

// Call at startup
extern void VGM_Player_Init();

extern u8 VGM_Player_docapture;


#endif /* _VGMPLAYER_H */
