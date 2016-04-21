/*
 * VGM Data and Playback Driver: VGM Data Source header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMSOURCE_H
#define _VGMSOURCE_H

#include <mios32.h>


#define VGM_SOURCE_TYPE_RAM 1
#define VGM_SOURCE_TYPE_STREAM 2
#define VGM_SOURCE_TYPE_QUEUE 3

typedef union {
    u8 ALL[24];
    struct{
        u8 type;
        u8 dummy1;
        u8 dummy2;
        u8 dummy3;
        u32 opn2clock;
        u32 psgclock;
        u32 loopaddr;
        u32 loopsamples;
        void* data;
    };
} VgmSource;

extern s32 VGM_Source_Delete(VgmSource* source);

#endif /* _VGMSOURCE_H */
