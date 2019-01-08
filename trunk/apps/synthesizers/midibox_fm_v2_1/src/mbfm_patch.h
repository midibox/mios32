// $Id: mbfm_patch.h $
/*
 * Header file for MIDIbox FM V2.0 patch file loading/saving
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBFM_PATCH_H
#define _MBFM_PATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MBFM_FILES_PATH
#define MBFM_FILES_PATH "/mbfm"
#endif

#ifndef MBFM_PATCHNAME_LEN
#define MBFM_PATCHNAME_LEN 11 //10 characters plus '\0'
#endif

#define MBFM_PATCHLOADERR_DRUMLOADTOVOICE -200
#define MBFM_PATCHLOADERR_VOICELOADTODRUM -201
#define MBFM_PATCHLOADERR_WRONGDRUM       -202
#define MBFM_PATCHLOADERR_INSUFFICIENT2OP -203
#define MBFM_PATCHLOADERR_INSUFFICIENT4OP -204
#define MBFM_PATCHLOADERR_NONMULTIPLE2OP  -205
#define MBFM_PATCHLOADERR_NONMULTIPLE4OP  -206

typedef union{
  u8 data;
  struct{
    u8 command:2;
    u8 dummy:1;
    u8 success:1;
    u8 drum:4;
  };
} prgstate_t;



extern char patchnames[16][MBFM_PATCHNAME_LEN];
extern char drumnames[2*OPL3_COUNT][MBFM_PATCHNAME_LEN];
extern u8 midibankmsb[16];
extern u8 midibanklsb[16];
extern u8 midiprogram[16];

extern prgstate_t prgstate[16];
extern char loadpatchname[MBFM_PATCHNAME_LEN];
extern u8 patchloadquery;
extern u32 lastdatawheelchange;

extern void MBFM_Patch_Init();
extern void MBFM_Patch_Tick(u32 time);

//drum: 0 BD(OPL3_0), 1 SD/HH/TT/CY(OPL3_0), 2 BD(OPL3_1), ...
extern s32 MBFM_PatchFile_Load(u8 chan, s8 drum, u8 bankmsb, u8 banklsb, u8 program);
extern s32 MBFM_PatchFile_Exists(u8 bankmsb, u8 banklsb, u8 program);
extern s32 MBFM_PatchFile_Save(u8 chan, s8 drum, u8 bankmsb, u8 banklsb, u8 program);



#ifdef __cplusplus
}
#endif

#endif /* _MBFM_PATCH_H */
