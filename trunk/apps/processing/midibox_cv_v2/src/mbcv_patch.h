// $Id$
/*
 * Patch Layer for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_PATCH_H
#define _MBCV_PATCH_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

#define MBCV_PATCH_CV_CURVE_V_OCTAVE    0
#define MBCV_PATCH_CV_CURVE_HZ_V        1
#define MBCV_PATCH_CV_CURVE_INVERTED    2
#define MBCV_PATCH_CV_CURVE_EXPONENTIAL 3



/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

extern s32 MBCV_PATCH_Init(u32 mode);

extern u16 MBCV_PATCH_ReadPar(u16 addr);
extern s32 MBCV_PATCH_WritePar(u16 addr, u16 value);

extern s32 MBCV_PATCH_Load(u8 bank, u8 patch);
extern s32 MBCV_PATCH_Store(u8 bank, u8 patch);

extern s32 MBCV_PATCH_LoadGlobal(char *filename);
extern s32 MBCV_PATCH_StoreGlobal(char *filename);

extern s32 MBCV_PATCH_Copy(u8 channel, u16 *buffer);
extern s32 MBCV_PATCH_Paste(u8 channel, u16 *buffer);
extern s32 MBCV_PATCH_Clear(u8 channel);
extern u16* MBCV_PATCH_CopyBufferGet(void);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////

extern u8 mbcv_patch_name[21];

extern u8 mbcv_patch_gateclr_cycles;

#ifdef __cplusplus
}
#endif


#endif /* _MBCV_PATCH_H */
