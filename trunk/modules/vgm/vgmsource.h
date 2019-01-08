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


typedef union {
    u32 all;
    struct {
        u8 cmd; //0x00 PSG write, data in data only; 0x02, 0x03 OPN2 writes port 0, 1
        //Plus 0x10 per board
        //0xFF for null command
        u8 addr;
        u8 data;
        u8 data2;
    };
} VgmChipWriteCmd;

typedef union {
    u32 all;
    struct {
        //Don't change the order of these bits, they're accessed by bit mask operations
        u8 fm1:1;
        u8 fm2:1;
        u8 fm3:1;
        u8 fm4:1;
        u8 fm5:1;
        u8 fm6:1;
        
        u8 fm1_lfo:1;
        u8 fm2_lfo:1;
        u8 fm3_lfo:1;
        u8 fm4_lfo:1;
        u8 fm5_lfo:1;
        u8 fm6_lfo:1;
        
        u8 dac:1;
        u8 fm3_special:1;
        u8 opn2_globals:1;
        u8 dummy:1;
        
        u8 lfomode:2; //0: untouched | 1: set to constant lfofixedspeed | 2 or 3: messed with
        u8 lfofixedspeed:3; //Must be 0 if lfomode is 0 (untouched)
        u8 dummy2:3;
        
        u8 sq1:1;
        u8 sq2:1;
        u8 sq3:1;
        u8 noise:1;
        u8 noisefreqsq3:1;
        u8 dummy3:3;
    };
} VgmUsageBits;


#define VGM_SOURCE_TYPE_RAM 1
#define VGM_SOURCE_TYPE_STREAM 2
#define VGM_SOURCE_TYPE_QUEUE 3

typedef union {
    u8 ALL[36];
    struct{
        u8 type;
        u8 psgfreq0to1:1;
        u8 dummy1:7;
        u16 mutes;
        u32 opn2clock;
        u32 psgclock;
        u32 loopaddr;
        u32 loopsamples;
        u32 markstart; //First command to play
        u32 markend; //Last command to play, inclusive
        VgmUsageBits usage;
        void* data;
    };
} VgmSource;



extern u8 VGM_Cmd_GetCmdLen(u8 type); //Number of bytes past the type
extern u32 VGM_Cmd_GetWaitValue(VgmChipWriteCmd cmd);
extern u8 VGM_Cmd_IsWrite(VgmChipWriteCmd cmd);
extern u8 VGM_Cmd_IsTwoByte(VgmChipWriteCmd cmd);
extern u8 VGM_Cmd_IsSecondHalfTwoByte(VgmChipWriteCmd cmd);

//Returns .all=0xFFFFFFFF if second is not the corresponding command to first
extern VgmChipWriteCmd VGM_Cmd_TryMakeTwoByte(VgmChipWriteCmd first, VgmChipWriteCmd second);

//Returns 1 if it is indeed a two write command, 0 if it is a normal command. Works on both RAM type and Board type commands.
extern u8 VGM_Cmd_UnpackTwoByte(VgmChipWriteCmd in, VgmChipWriteCmd* out1, VgmChipWriteCmd* out2);


extern void VGM_Cmd_UpdateUsage(VgmUsageBits* usage, VgmChipWriteCmd cmd);
extern void VGM_Cmd_DebugPrintUsage(VgmUsageBits usage);
extern void VGM_Source_UpdateUsage(VgmSource* source);

extern s32 VGM_Source_Delete(VgmSource* source);

#endif /* _VGMSOURCE_H */
