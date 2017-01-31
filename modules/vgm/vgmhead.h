/*
 * VGM Data and Playback Driver: VGM Playback Head header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _VGMHEAD_H
#define _VGMHEAD_H

#include <mios32.h>
#include "vgmsource.h"


#define VGM_DELAY62 735
#define VGM_DELAY63 882


/*
Channels are:
0 OPN2 globals
1-6 OPN2 channels 0-2 on bank 0, 0-2 on bank 1
7 DAC (and OPN2 DAC Enable)
8-B PSG channels
*/
typedef union {
    u8 ALL;
    struct{
        u8 nodata:1; //Source has no data on this channel (used by allocator, VGM engine treats it like mute, though that shouldn't ever matter)
        u8 mute:1;
        u8 map_chip:2;
        u8 map_voice:3; //Either 0-5 or 0-2; channels 0, 7, and B ignore
        u8 option:1; //For OPN2 voices, flag for using LFO; for PSG Noise, flag for using SQ3
    };
} VgmHead_Channel;

typedef union {
    u8 ALL[44];
    struct{
        VgmSource* source;
        void* data;
        u32 ticks;
        VgmChipWriteCmd writecmd;
        VgmHead_Channel channel[12]; //0 OPN2, 1-6 voices, 7 DAC, 8-A sq, B noise
        u32 opn2mult;
        u32 psgmult;
        u32 tempomult;
        u8 playing:1;
        u8 iswait:1;
        u8 iswrite:1;
        u8 isdone:1;
        u8 firstoftwo:1;
        u8 psgfreq0to1:1;
        u8 psglastchannel:2;
        u32 dummy:24;
    };
} VgmHead;


extern void VGM_Head_Init();

extern VgmHead* VGM_Head_Create(VgmSource* source, u32 freqmult, u32 tempomult);
extern s32 VGM_Head_Delete(VgmHead* head); //Remove from queue and free

extern void VGM_Head_Restart(VgmHead* head, u32 vgm_time);
extern void VGM_Head_cmdNext(VgmHead* head, u32 vgm_time);

static inline u8 VGM_Head_cmdIsWait(VgmHead* head) {return head->iswait || head->isdone;}
static inline s32 VGM_Head_cmdGetWaitRemaining(VgmHead* head, u32 vgm_time) {return (head->isdone ? 65535 : (s32)(head->ticks - vgm_time));}
static inline u8 VGM_Head_cmdIsChipWrite(VgmHead* head) {return head->iswrite && !head->isdone;}
//static inline VgmChipWriteCmd VGM_Head_cmdGetChipWrite() {return head->writecmd;}
extern void VGM_Head_doMapping(VgmHead* head, VgmChipWriteCmd* cmd);

#define VGM_HEAD_MAXNUM 64
extern VgmHead* vgm_heads[VGM_HEAD_MAXNUM];
extern u32 vgm_numheads;

#endif /* _VGMHEAD_H */
