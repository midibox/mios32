/*
 * VGM Data and Playback Driver: VGM RAM Block Data
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#include "vgmram.h"

//TODO ALL
VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source) {return NULL;}
void VGM_HeadRAM_Delete(void* headram) {}
void VGM_HeadRAM_Restart(VgmHead* head) {}
void VGM_HeadRAM_cmdNext(VgmHead* head, u32 vgm_time) {}
VgmSource* VGM_SourceRAM_Create() {return NULL;}
void VGM_SourceRAM_Delete(void* sourceram) {}
