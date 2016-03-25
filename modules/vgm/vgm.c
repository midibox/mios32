/*
 * VGM Data and Playback Driver: Main
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "vgm.h"
#include "vgmhead.h"
#include "vgmplayer.h"
#include "vgmsdtask.h"

void VGM_Init(){
    VGM_Head_Init();
    VGM_Player_Init();
    VGM_SDTask_Init();
}

