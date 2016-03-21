/*
 * VGM Data and Playback Driver: VGM Streaming Data header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMSTREAM_H
#define _VGMSTREAM_H

#include <mios32.h>
#include "vgmsource.h"
#include "vgmhead.h"
#include <file.h>

#ifndef VGM_SOURCESTREAM_BUFSIZE
#define VGM_SOURCESTREAM_BUFSIZE 512
#endif

#define VGM_HEADSTREAM_SUBBUFFER_MAXLEN 16

typedef union {
    u8 ALL[9+VGM_HEADSTREAM_SUBBUFFER_MAXLEN];
    struct{
        u32 srcaddr;
        u32 srcblockaddr;
        u8 subbuffer[VGM_HEADSTREAM_SUBBUFFER_MAXLEN];
        u8 subbufferlen;
    };
} VgmHeadStream;

extern VgmHeadStream* VGM_HeadStream_Create(VgmSource* source);
extern void VGM_HeadStream_Delete(void* headstream);
extern void VGM_HeadStream_Restart(VgmHead* head);
extern void VGM_HeadStream_cmdNext(VgmHead* head, u32 vgm_time);

typedef union {
    u8 ALL[44+sizeof(file_t)];
    struct{
        file_t file;
        u32 datalen;
        u32 vgmdatastartaddr;
        
        u8* buffer1;
        u8* buffer2;
        u32 buffer1addr;
        u32 buffer2addr;
        
        u8 wantbuffer;
        u8 dummy1;
        u16 dummy2;
        u32 wantbufferaddr;

        u8* block;
        u32 blocklen;
        u32 blockorigaddr;
    };
} VgmSourceStream;

extern VgmSource* VGM_SourceStream_Create();
extern void VGM_SourceStream_Delete(void* sourcestream);
extern s32 VGM_SourceStream_Start(VgmSource* source, char* filename);
extern u8 VGM_SourceStream_getByte(VgmSourceStream* vss, u32 addr);
static inline u8 VGM_SourceStream_getBlockByte(VgmSourceStream* vss, u32 blockaddr){ return ((blockaddr < vss->blocklen) ? (vss->block[blockaddr]) : 0); }
extern void VGM_SourceStream_loadBlock(VgmSourceStream* vss, u32 startaddr, u32 len);

#endif /* _VGMSTREAM_H */
