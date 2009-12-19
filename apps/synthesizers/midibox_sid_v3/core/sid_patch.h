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
// General Substructures
////////////////////////////////////////

typedef union {
  struct {
    u8 ALL[5];
  };
  struct {
    u8 assign1;
    u8 assign2;
    u8 value;
    u8 min;
    u8 max;
  };
} sid_patch_knob_t;


typedef union {
  struct {
    u8 ALL[6];
  };
  struct {
    u8 chn_mode;
    u16 cutoff; // 12bit, 11 bits used - bit 7 controls FIP (interpolation option)
    u8 resonance; // 8 bit, 4 bits used)
    u8 keytrack;
    u8 reserved; // for future extensions
  };
} sid_patch_filter_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned PORTA_MODE:2; // portamento (0), constant time glide (1), glissando (2)
    unsigned GAE:1; // gate always enabled
  };
} sid_patch_voice_flags_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned WAVEFORM:4;
    unsigned VOICE_OFF:1;
    unsigned SYNC:1;
    unsigned RING:1;
    unsigned RESERVED:1;
  };
} sid_patch_voice_waveform_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned DECAY:4;
    unsigned ATTACK:4;
  };
} sid_patch_voice_ad_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned SUSTAIN:4;
    unsigned RELEASE:4;
  };
} sid_patch_voice_sr_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned ENABLE:1;
    unsigned DIR:2; // up/down/U&D/D&U
    unsigned RANDOM:1;
    unsigned SORTED:1;
    unsigned HOLD:1;
    unsigned SYNC:1;
    unsigned CAC:1;
  };
} sid_patch_voice_arp_mode_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned DIV:6;
    unsigned EASY_CHORD:1;
    unsigned ONESHOT:1;
  };
} sid_patch_voice_arp_speed_div_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned GATELENGTH:5;
    unsigned OCTAVE_RANGE:3;
  };
} sid_patch_voice_arp_gl_rng_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned WAVEFORM_SECOND:4;
    unsigned ENABLE_SECOND:1;
    unsigned PITCHX2:1;
    unsigned REVERSED_WAVEFORM:1;
  };
} sid_patch_voice_swinsid_mode_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned ENABLE:1;
    unsigned RESERVED1:1;
    unsigned CLKSYNC:1;
    unsigned ONESHOT:1;
    unsigned WAVEFORM:4;
  };
} sid_patch_lfo_mode_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned LOOP_BEGIN:3;
    unsigned RESERVED1:1;
    unsigned LOOP_END:3;
    unsigned RESERVED2:1;
  };
} sid_patch_env_mode_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned OP:4;
    unsigned RESERVED:2;
    unsigned INVERT_TARGET_2;
    unsigned INVERT_TARGET_1;
  };
} sid_patch_mod_op_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned PITCH_1:1;
    unsigned PITCH_2:1;
    unsigned PITCH_3:1;
    unsigned PW_1:1;
    unsigned PW_2:1;
    unsigned PW_3:1;
    unsigned FILTER:1;
    unsigned VOLUME:1;
  };
} sid_patch_mod_direct_targets_t;


////////////////////////////////////////
// Lead Engine Substructures
////////////////////////////////////////

typedef union {
  struct {
    u8 ALL[16];
  };
  struct {
    sid_patch_voice_flags_t flags;
    sid_patch_voice_waveform_t waveform;
    sid_patch_voice_ad_t ad;
    sid_patch_voice_sr_t sr;
    u16 pulsewidth; // [11:0] only
    u8 accent; // not used by lead engine, could be replaced by something else
    u8 swinsid_phase; // used by lead engine if SwinSID option enabled
    u8 delay; // 8bit
    u8 transpose; // 7bit
    u8 finetune; // 8bit
    u8 pitchrange; // 7bit
    u8 portamento; // 8bit
    sid_patch_voice_arp_mode_t arp_mode;
    sid_patch_voice_arp_speed_div_t arp_speed_div;
    sid_patch_voice_arp_gl_rng_t arp_gl_rng;
    sid_patch_voice_swinsid_mode_t swinsid_mode;
  };
} sid_patch_l_voice_t;

typedef union {
  struct {
    u8 ALL[5];
  };
  struct {
    sid_patch_lfo_mode_t mode;
    s8 depth;
    u8 rate;
    u8 delay;
    u8 phase;
  };
} sid_patch_l_lfo_t;

typedef union {
  struct {
    u8 ALL[16];
  };
  struct {
    sid_patch_env_mode_t mode;
    s8 depth;
    u8 delay;
    u8 attack1;
    u8 attlvl;
    u8 attack2;
    u8 decay1;
    u8 declvl;
    u8 decay2;
    u8 sustain;
    u8 release1;
    u8 rellvl;
    u8 release2;
    s8 att_curve;
    s8 dec_curve;
    s8 rel_curve;
  };
} sid_patch_l_env_t;

typedef union {
  struct {
    u8 ALL[8];
  };
  struct {
    u8 src1;
    u8 src2;
    sid_patch_mod_op_t op;
    s8 depth;
    sid_patch_mod_direct_targets_t direct_target[2]; // L/R
    u8 x_target[2];
  };
} sid_patch_l_mod_t;

typedef union {
  struct {
    u8 ALL[3];
  };
  struct {
    unsigned O1L:1; // OSC1 Left
    unsigned O2L:1; // OSC2 Left
    unsigned O3L:1; // OSC3 Left
    unsigned O1R:1; // OSC1 Right
    unsigned O2R:1; // OSC2 Right
    unsigned O3R:1; // OSC3 Right
    unsigned E1A:1; // ENV1 Attack
    unsigned E2A:1; // ENV2 Attack
    unsigned E1R:1; // ENV1 Release
    unsigned E2R:1; // ENV2 Release
    unsigned L1:1; // LFO1 Sync
    unsigned L2:1; // LFO2 Sync
    unsigned L3:1; // LFO3 Sync
    unsigned L4:1; // LFO4 Sync
    unsigned L5:1; // LFO5 Sync
    unsigned L6:1; // LFO6 Sync
    unsigned W1R:1; // WT1 reset
    unsigned W2R:1; // WT2 reset
    unsigned W3R:1; // WT3 reset
    unsigned W4R:1; // WT4 reset
    unsigned W1S:1; // WT1 step
    unsigned W2S:1; // WT2 step
    unsigned W3S:1; // WT3 step
    unsigned W4S:1; // WT4 step
  };
} sid_patch_l_trg_t;

typedef union {
  struct {
    u8 ALL[5];
  };
  struct {
    u8 speed;
    u8 assign;
    unsigned begin:7;
    unsigned POS_MOD_CTRL:1;
    unsigned end:7;
    unsigned POS_KEY_CTRL:1;
    unsigned loop:7;
    unsigned ONE_SHOT:1;
  };
} sid_patch_l_wt_t;



////////////////////////////////////////
// Combined Patch Structure
////////////////////////////////////////
typedef union {
  struct {
    u8 ALL[512];
  };

  struct {
    u8 engine; // sid_se_engine_t
    u8 hw_flags; // sid_se_hw_flags_t
    u8 opt_flags; // sid_se_opt_flags_t

    u8 custom_sw; // 8 switches, which can be optionally forwarded to J5 or used inside engine for private enhancements
    u8 reserved1[3]; // for future extensions

    u8 knob[8][5]; // sid_knob_t, Knob #1..5, K#V, K#P and K#A

    u16 ext_par[8]; // external parameters, which can be optionally forwarded to AOUTs
  };

  struct {
    u8 header[0x50]; // the same for all engines

    // 0x50..0x53
    u8 flags; // sid_se_l_flags_t 
    u8 osc_detune; // detune left/right oscillators
    u8 volume; // 7bit value, only 4 bit used
    u8 osc_phase; // oscillator phase offset

    // 0x54..0x5f
    u8 filter[2][6]; // L/R sid_se_filter_patch_t

    // 0x60..0xbf
    u8 voice[6][16]; // Voice 1..6, sid_se_voice_patch_t

    // 0xc0..0xdd
    u8 lfo[6][5]; // LFO 1..6, sid_se_lfo_patch_t

    // 0xde..0xdf
    u8 reserved_lfo[2];

    // 0xe0..0xff
    u8 env[2][16]; // ENV 1..2, sid_se_env_patch_t

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

} sid_patch_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_PATCH_Init(u32 mode);
extern s32 SID_PATCH_Preset(sid_patch_t *patch, sid_se_engine_t engine);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern sid_patch_t sid_patch[SID_PATCH_NUM];
extern sid_patch_t sid_patch_shadow[SID_PATCH_NUM];

#endif /* _SID_PATCH_H */
