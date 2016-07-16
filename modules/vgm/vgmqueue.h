/*
 * VGM Data and Playback Driver: VGM Data Queue header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMQUEUE_H
#define _VGMQUEUE_H

#include <mios32.h>
#include "vgmsource.h"
#include "vgmhead.h"

//TODO allow queues for different purposes to have different lengths, memory
//allocated at runtime rather than in the struct
#define VGM_QUEUE_LENGTH 256

typedef union {
    u8 ALL[4+(VGM_QUEUE_LENGTH*sizeof(VgmChipWriteCmd))];
    struct{
        u8 busy;
        u8 dummy;
        u8 start;
        u8 depth;
        VgmChipWriteCmd queue[VGM_QUEUE_LENGTH];
    };
} VgmHeadQueue;

extern VgmHeadQueue* VGM_HeadQueue_Create(VgmSource* source);
extern void VGM_HeadQueue_Delete(void* headqueue);
extern void VGM_HeadQueue_Restart(VgmHead* head);
extern void VGM_HeadQueue_cmdNext(VgmHead* head, u32 vgm_time);

extern void VGM_HeadQueue_Enqueue(VgmHead* head, VgmChipWriteCmd cmd, u8 fixfreq);

extern VgmSource* VGM_SourceQueue_Create();
extern void VGM_SourceQueue_Delete(void* sourcequeue);

#endif /* _VGMQUEUE_H */
