// $Id$
/*
 * Header file for MBSID Patch Structure
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_PATCH_H
#define _SID_PATCH_H

#include <sid.h>
#include "sid_se.h"
#include "sid_se_l.h"

/////////////////////////////////////////////////////////////////////////////
// Global Defines
/////////////////////////////////////////////////////////////////////////////

#define SID_PATCH_NUM (SID_NUM) // SID_NUN is defined in sid.h -- note: in distance to MBSID V2, each SID can have it's own patch!


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////
// Combined Patch Structure
////////////////////////////////////////
typedef union {
  struct {
    u8 ALL[512];
  };

  struct {
    u8 name[16]; // 16 chars w/o null termination

    u8 engine; // sid_se_engine_t
    u8 hw_flags; // sid_se_hw_flags_t
    u16 opt_flags; // sid_se_opt_flags_t

    u8 custom_sw; // 8 switches, which can be optionally forwarded to J5 or used inside engine for private enhancements
    u8 reserved1[3]; // for future extensions

    u8 knob[8][5]; // sid_knob_t, Knob #1..5, K#V, K#P and K#A

    u16 ext_par[8]; // external parameters, which can be optionally forwarded to AOUTs
  };

  struct {
    u8 header[0x50]; // the same for all engines

    // 0x50..0x53
    u8 v_flags; // sid_se_v_flags_t 
    u8 osc_detune; // detune left/right oscillators
    u8 volume; // 7bit value, only 4 bit used
    u8 osc_phase; // oscillator phase offset -- not used by bassline engine!

    // 0x54..0x5f
    u8 filter[2][6]; // L/R sid_se_filter_patch_t

    // 0x60..0xbf
    u8 voice[6][16]; // Voice 1..6, sid_se_voice_patch_t.L

    // 0xc0..0xdd
    u8 lfo[6][5]; // LFO 1..6, sid_se_lfo_patch_t.L

    // 0xde..0xdf
    u8 reserved_lfo[2];

    // 0xe0..0xff
    u8 env[2][16]; // ENV 1..2, sid_se_env_patch_t.L

    // 0x100..0x13f
    u8 mod[8][8]; // MOD 1..8, sid_se_mod_patch_t

    // 0x140..0x169
    u8 trg_matrix[14][3]; // 14 * sid_se_trg_t

    // 0x16a..0x16b
    u8 reserved_trg[2];

    // 0x16c..0x17f
    u8 wt[4][5]; // WT 1..4, sid_se_wt_par_patch_t
 
    // 0x180..0x1ff
    u8 wt_memory[128];
  } L;

  struct {
    u8 header[0x50]; // the same for all engines

    // 0x50..0x53
    u8 flags; // not used - flags are available for each individual instrument
    u8 osc_detune; // detune left/right oscillators
    u8 volume; // 7bit value, only 4 bit used
    u8 osc_phase; // oscillator phase offset

    // 0x54..0x5f
    u8 filter[2][6]; // L/R sid_se_filter_patch_t

    // 0x60..0xaf/0xb0..0xff
    u8 voice[2][80]; // sid_se_voice_patch_t.B

    // 0x100..0x1ff
    u8 seq_memory[256];
  } B;

  struct {
    u8 header[0x50]; // the same for all engines

    // 0x50..0x53
    u8 seq_speed; // sid_se_seq_speed_par_t
    u8 seq_num; // sequence number (0-8, 8=disabled)
    u8 volume; // 7bit value, only 4 bit used
    u8 seq_length; // steps (0-15)

    // 0x54..0x5f
    u8 filter[2][6]; // L/R sid_se_filter_patch_t

    // 0x60..0xaf/0xb0..0xff
    u8 voice[16][10]; // sid_se_voice_patch_t.D

    // 0x100..0x1ff
    u8 seq_memory[256];
  } D;

  struct {
    u8 header[0x50]; // the same for all engines

    // 0x50..0x53
    u8 flags; // not used - flags are available for each individual instrument
    u8 osc_detune; // detune left/right oscillators -- not used by multi engine!
    u8 volume; // 7bit value, only 4 bit used
    u8 osc_phase; // oscillator phase offset -- not used by multi engine!

    // 0x54..0x5f
    u8 filter[2][6]; // L/R sid_se_filter_patch_t

    // 0x060/0x090/0x0c0/0x0f0/0x120/0x150..0x17f
    u8 voice[6][48]; // sid_se_voice_patch_t.M

    // 0x180..0x1ff
    u8 wt_memory[128];
  } M;

} sid_patch_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_PATCH_Init(u32 mode);
extern s32 SID_PATCH_Changed(u8 sid);
extern s32 SID_PATCH_Preset(sid_patch_t *patch, sid_se_engine_t engine);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern sid_patch_t sid_patch[SID_PATCH_NUM];
extern sid_patch_t sid_patch_shadow[SID_PATCH_NUM];

#endif /* _SID_PATCH_H */
