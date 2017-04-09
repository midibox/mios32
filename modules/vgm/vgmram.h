/*
 * VGM Data and Playback Driver: VGM RAM Block Data header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2017 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 *
 * Each command is a fixed-size (4-byte) VgmChipWriteCmd. The style used in
 * the array is the original VGM style (PSG is 0x50, OPN2 is 0x52/0x53).
 * Supported commands are OPN2 write (if frequency write, LSB is in data2),
 * PSG write (same for frequency command), sample+wait (0x80-0x8F, except the
 * actual command for the sample write is in addr and data), and all the timing
 * comamnds. All other commands are ignored.
 */

#ifndef _VGMRAM_H
#define _VGMRAM_H

#include <mios32.h>
#include "vgmsource.h"
#include "vgmhead.h"

typedef union {
    u8 ALL[4];
    struct{
        VgmChipWriteCmd bufferedcmd;
    };
} VgmHeadRAM;

extern VgmHeadRAM* VGM_HeadRAM_Create(VgmSource* source);
extern void VGM_HeadRAM_Delete(void* headram);
extern void VGM_HeadRAM_Restart(VgmHead* head);
extern void VGM_HeadRAM_cmdNext(VgmHead* head, u32 vgm_time);

extern s32 VGM_HeadRAM_Forward1(VgmHead* head);
extern s32 VGM_HeadRAM_Backward1(VgmHead* head);
extern s32 VGM_HeadRAM_SeekTo(VgmHead* head, u32 newaddr);
extern s32 VGM_HeadRAM_ForwardState(VgmHead* head, u32 maxt, u32 maxdt, u8 allowstay);
extern s32 VGM_HeadRAM_BackwardState(VgmHead* head, u32 maxt, u32 maxdt);

typedef union {
    u8 ALL[8];
    struct{
        VgmChipWriteCmd* cmds;
        u32 numcmds;
    };
} VgmSourceRAM;

extern VgmSource* VGM_SourceRAM_Create();
extern void VGM_SourceRAM_Delete(void* sourceram);

extern void VGM_SourceRAM_UpdateUsage(VgmSource* source);

extern void VGM_SourceRAM_InsertCmd(VgmSource* source, u32 addr, VgmChipWriteCmd newcmd);
extern void VGM_SourceRAM_DeleteCmd(VgmSource* source, u32 addr);

#endif /* _VGMRAM_H */
