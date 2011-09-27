// $Id$
/*
 * Patch Layer for MIDIbox CV V2
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"

#include <app.h>
#include <MbCvEnvironment.h>


#include "mbcv_map.h"
#include "mbcv_patch.h"


// quick&dirty to simplify re-use of C modules without changing header files
extern "C" {
#include "mbcv_file.h"
#include "mbcv_file_p.h"
}


/////////////////////////////////////////////////////////////////////////////
// Preset patch
/////////////////////////////////////////////////////////////////////////////

mbcv_patch_router_entry_t mbcv_patch_router[MBCV_PATCH_NUM_ROUTER] = {
  // src chn   dst chn
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
  { 0x10,  0, 0x20, 17 },
};

u32 mbcv_patch_router_mclk_in;
u32 mbcv_patch_router_mclk_out;

mbcv_patch_cfg_t mbcv_patch_cfg;

u8 mbcv_patch_gate_inverted[MBCV_PATCH_NUM_CV/8];

u8 mbcv_patch_gateclr_cycles = 3; // 3 mS


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // init remaining config values
  mbcv_patch_cfg.flags.ALL = 0;
  mbcv_patch_cfg.ext_clk_divider = 16; // 24 ppqn
  mbcv_patch_cfg.ext_clk_pulsewidth = 1;

  //                           USB0 only     UART0..3       IIC0..3      OSC0..3
  mbcv_patch_router_mclk_in = (0x01 << 0) | (0x0f << 8) | (0x0f << 16) | (0x01 << 24);
  //                            all ports
  mbcv_patch_router_mclk_out = 0xffffffff;

  int i;
  for(i=0; i<(MBCV_PATCH_NUM_CV/8); ++i)
    mbcv_patch_gate_inverted[i] = 0x00;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns a byte from patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
u8 MBCV_PATCH_ReadByte(u16 addr)
{
  if( addr >= MBCV_PATCH_SIZE )
    return 0;

  // Note: this patch structure only exists for compatibility reasons with MBCV V1
  // Don't enhance it! Meanwhile the complete patch can only be edited via SD Card!
  if( addr < 8 ) {
    switch( addr ) {
    case 0x00: return mbcv_patch_cfg.flags.MERGER_MODE;
    case 0x02: return mbcv_patch_gate_inverted[0];
    case 0x03: return mbcv_patch_cfg.ext_clk_divider; // TODO: convert from old format!
    }
  } else {
    u8 cv = addr & 0x7;
    MbCvEnvironment* env = APP_GetEnv();
    MbCvVoice *v = &env->mbCv[cv].mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    switch( addr >> 3 ) {
    case 0x01: return mv->midivoiceChannel ? (mv->midivoiceChannel - 1) : 16; // normaly 0..16 (0 disables channel) - for patch compatibility we take 16 to disable channel
    case 0x02: return (v->voiceEventMode & 0xf) | (v->voiceLegato << 4) | (v->voicePoly << 5);
    case 0x03: return MBCV_MAP_PitchRangeGet(cv);
    case 0x04: return mv->midivoiceSplitLower;
    case 0x05: return mv->midivoiceSplitUpper;
    case 0x06: return (v->voiceTransposeOctave >= 0) ? v->voiceTransposeOctave : (16+v->voiceTransposeOctave);
    case 0x07: return (v->voiceTransposeSemitone >= 0) ? v->voiceTransposeSemitone : (16+v->voiceTransposeSemitone);
    case 0x08: return mv->midivoiceCCNumber;
    case 0x09: return MBCV_MAP_CurveGet(cv);
    case 0x0a: return MBCV_MAP_SlewRateGet(cv); // TODO: conversion to old format
    }
  }

  return 0x00;
}


/////////////////////////////////////////////////////////////////////////////
// This function writes a byte into patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_WriteByte(u16 addr, u8 byte)
{
  if( addr >= MBCV_PATCH_SIZE )
    return -1; // invalid address

  if( addr >= MBCV_PATCH_SIZE )
    return 0;

  // Note: this patch structure only exists for compatibility reasons with MBCV V1
  // Don't enhance it! Meanwhile the complete patch can only be edited via SD Card!
  if( addr < 8 ) {
    switch( addr ) {
    case 0x00: mbcv_patch_cfg.flags.MERGER_MODE = byte; return 0;
    case 0x02: mbcv_patch_gate_inverted[0] = byte; return 0;
    case 0x03: mbcv_patch_cfg.ext_clk_divider = byte; return 0; // TODO: convert from old format!
    }
    return 0x00;
  } else if( addr < 0x100 ) {
    u8 cv = addr & 0x7;
    MbCvEnvironment* env = APP_GetEnv();
    MbCvVoice *v = &env->mbCv[cv].mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    switch( addr >> 3 ) {
    case 0x01: mv->midivoiceChannel = (byte < 16) ? (byte+1) : 0; return 0;
    case 0x02:
      v->voiceEventMode = (mbcv_midi_event_mode_t)(byte & 0xf);
      v->voiceLegato = (byte & 0x10) ? 1 : 0;
      v->voicePoly = (byte & 0x20) ? 1 : 0;
      return 0;
    case 0x03: MBCV_MAP_PitchRangeSet(cv, byte); return 0;
    case 0x04: mv->midivoiceSplitLower = byte; return 0;
    case 0x05: mv->midivoiceSplitUpper = byte; return 0;
    case 0x06: if( byte < 8 ) v->voiceTransposeOctave = byte; else v->voiceTransposeOctave = 7 - (int)byte; return 0;
    case 0x07: if( byte < 8 ) v->voiceTransposeSemitone = byte; else v->voiceTransposeSemitone = 7 - (int)byte; return 0;
    case 0x08: mv->midivoiceCCNumber = byte; return 0;
    case 0x09: MBCV_MAP_CurveSet(cv, byte); return 0;
    case 0x0a: MBCV_MAP_SlewRateSet(cv, byte); return 0; // TODO: conversion to old format
    }
    return 0x00;
  }

  return -2; // value not mapped
}


/////////////////////////////////////////////////////////////////////////////
// This function loads the patch from SD Card
// Returns != 0 if Load failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Load(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Read(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch on SD Card
// Returns != 0 if Store failed
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_PATCH_Store(char *filename)
{
  MUTEX_SDCARD_TAKE;
  s32 status = MBCV_FILE_P_Write(filename);
  MUTEX_SDCARD_GIVE;

  return status;
}
