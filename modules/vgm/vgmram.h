/*
 * VGM Data and Playback Driver: VGM RAM Block Data header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMRAM_H
#define _VGMRAM_H

#include <mios32.h>
#include "vgmsource.h"
#include "vgmhead.h"

typedef union {
    u8 ALL[1];
    struct{
        u8 todo;
    };
} VgmHeadRAM;

extern VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source);
extern void VGM_HeadRAM_Delete(void* headram);
extern void VGM_HeadRAM_Restart(VgmHead* head);
extern void VGM_HeadRAM_cmdNext(VgmHead* head, u32 vgm_time);

typedef union {
    u8 ALL[1];
    struct{
        u8 todo;
    };
} VgmSourceRAM;

extern VgmSource* VGM_SourceRAM_Create();
extern void VGM_SourceRAM_Delete(void* sourceram);

#endif /* _VGMRAM_H */
