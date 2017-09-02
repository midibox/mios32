/*
 * VGM Data and Playback Driver: VGM File Load/Save header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2017 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMFILE_H
#define _VGMFILE_H

#include <mios32.h>
#include "vgmhead.h"
#include "vgmperfmon.h"
#include "vgmram.h"
#include "vgmsource.h"
#include "vgmstream.h"
#include <file.h>


typedef union {
    u8 ALL[sizeof(file_t)+44];
    struct{
        file_t file;
        u32 filesize;
        
        u32 numcmds;
        u32 numcmdsram;
        u32 numblocks;
        u32 totalblocksize;
        
        VgmUsageBits usage;
        
        u32 vgmdatastartaddr;
        u32 loopaddr;
        u32 loopsamples;
        
        u32 psgclock:31;
        u8 psgfreq0to1:1;
        u32 opn2clock;
    };
} VgmFileMetadata;

//resultMsg can be NULL if you don't care about the message
extern s32 VGM_File_Load(char* filename, VgmSource** ss, char* resultMsg);

extern s32 VGM_File_ScanFile(char* filename, VgmFileMetadata* md);

extern s32 VGM_File_StartStream(VgmSource* sourcestream, char* filepath, VgmFileMetadata* md);

extern s32 VGM_File_LoadRAM(VgmSource* sourceram, VgmFileMetadata* md);
extern s32 VGM_File_SaveRAM(VgmSource* sourceram, char* filename);



#endif /* _VGMFILE_H */
