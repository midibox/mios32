/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * This file defines all structures used by MIDIbox SID
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBSID_STRUCTS_H
#define _MBSID_STRUCTS_H

#include <mios32.h>
#include <sid.h>
#include <notestack.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SID_SE_NUM             (SID_NUM/2) // should match with number of SIDs / 2 (stereo configuration)
#define SID_SE_NUM_VOICES      6
#define SID_SE_NUM_MIDI_VOICES 6
#define SID_SE_NUM_FILTERS     2 // must be at least 2 for all engines
#define SID_SE_NUM_LFO         (2*SID_SE_NUM_VOICES) // must be at least SID_SE_NUM_VOICES for lead engine, and 2*SID_SE_NUM_VOICES for multi engine
#define SID_SE_NUM_ENV         (2*SID_SE_NUM_VOICES) // must be at least 2 for lead engine, and 2*SID_SE_NUM_VOICES for multi engine
#define SID_SE_NUM_WT          (SID_SE_NUM_VOICES) // must be at least 4 for lead engine, and SID_SE_NUM_VOICES for multi/drum engine
#define SID_SE_NUM_SEQ         2 // must be two for bassline engine

#define SID_SE_NOTESTACK_SIZE 10

#define SID_SE_DRUM_MODEL_NUM 20

#define SID_SE_KNOB_NUM 8


// Trigger Matrix assignments (since each entry allocates 3 bytes, a struct cannot be used to define it)
#define SID_SE_TRG_NOn   0 // Note On
#define SID_SE_TRG_NOff  1 // Note Off
#define SID_SE_TRG_E1S   2 // ENV1 Sustain Phase
#define SID_SE_TRG_E2S   3 // ENV2 Sustain Phase
#define SID_SE_TRG_L1P   4 // LFO1 Period
#define SID_SE_TRG_L2P   5 // LFO2 Period
#define SID_SE_TRG_L3P   6 // LFO3 Period
#define SID_SE_TRG_L4P   7 // LFO4 Period
#define SID_SE_TRG_L5P   8 // LFO5 Period
#define SID_SE_TRG_L6P   9 // LFO6 Period
#define SID_SE_TRG_CLK  10 // Clock
#define SID_SE_TRG_CL6  11 // Clock/6
#define SID_SE_TRG_C24  12 // Clock/24
#define SID_SE_TRG_MST  13 // MIDI Clock Start


// Modulation source assignments
#define SID_SE_MOD_SRC_ENV1      0
#define SID_SE_MOD_SRC_ENV2      1
#define SID_SE_MOD_SRC_LFO1      2
#define SID_SE_MOD_SRC_LFO2      3
#define SID_SE_MOD_SRC_LFO3      4
#define SID_SE_MOD_SRC_LFO4      5
#define SID_SE_MOD_SRC_LFO5      6
#define SID_SE_MOD_SRC_LFO6      7
#define SID_SE_MOD_SRC_MOD1      8
#define SID_SE_MOD_SRC_MOD2      9
#define SID_SE_MOD_SRC_MOD3     10
#define SID_SE_MOD_SRC_MOD4     11
#define SID_SE_MOD_SRC_MOD5     12
#define SID_SE_MOD_SRC_MOD6     13
#define SID_SE_MOD_SRC_MOD7     14
#define SID_SE_MOD_SRC_MOD8     15
#define SID_SE_MOD_SRC_MDW      16
#define SID_SE_MOD_SRC_KEY      17
#define SID_SE_MOD_SRC_KNOB1    18
#define SID_SE_MOD_SRC_KNOB2    19
#define SID_SE_MOD_SRC_KNOB3    20
#define SID_SE_MOD_SRC_KNOB4    21
#define SID_SE_MOD_SRC_KNOB5    22
#define SID_SE_MOD_SRC_VEL      23
#define SID_SE_MOD_SRC_PBN      24
#define SID_SE_MOD_SRC_ATH      25
#define SID_SE_MOD_SRC_WT1      26
#define SID_SE_MOD_SRC_WT2      27
#define SID_SE_MOD_SRC_WT3      28
#define SID_SE_MOD_SRC_WT4      29

#define SID_SE_NUM_MOD_SRC      30


// Modulation destination assignments
#define SID_SE_MOD_DST_PITCH1    0
#define SID_SE_MOD_DST_PITCH2    1
#define SID_SE_MOD_DST_PITCH3    2
#define SID_SE_MOD_DST_PITCH4    3
#define SID_SE_MOD_DST_PITCH5    4
#define SID_SE_MOD_DST_PITCH6    5
#define SID_SE_MOD_DST_PW1       6
#define SID_SE_MOD_DST_PW2       7
#define SID_SE_MOD_DST_PW3       8
#define SID_SE_MOD_DST_PW4       9
#define SID_SE_MOD_DST_PW5      10
#define SID_SE_MOD_DST_PW6      11
#define SID_SE_MOD_DST_FIL1     12
#define SID_SE_MOD_DST_FIL2     13
#define SID_SE_MOD_DST_VOL1     14
#define SID_SE_MOD_DST_VOL2     15
#define SID_SE_MOD_DST_LD1      16
#define SID_SE_MOD_DST_LD2      17
#define SID_SE_MOD_DST_LD3      18
#define SID_SE_MOD_DST_LD4      19
#define SID_SE_MOD_DST_LD5      20
#define SID_SE_MOD_DST_LD6      21
#define SID_SE_MOD_DST_LR1      22
#define SID_SE_MOD_DST_LR2      23
#define SID_SE_MOD_DST_LR3      24
#define SID_SE_MOD_DST_LR4      25
#define SID_SE_MOD_DST_LR5      26
#define SID_SE_MOD_DST_LR6      27
#define SID_SE_MOD_DST_EXT1     28
#define SID_SE_MOD_DST_EXT2     29
#define SID_SE_MOD_DST_EXT3     30
#define SID_SE_MOD_DST_EXT4     31
#define SID_SE_MOD_DST_EXT5     32
#define SID_SE_MOD_DST_EXT6     33
#define SID_SE_MOD_DST_EXT7     34
#define SID_SE_MOD_DST_EXT8     35
#define SID_SE_MOD_DST_WT1      36
#define SID_SE_MOD_DST_WT2      37
#define SID_SE_MOD_DST_WT3      38
#define SID_SE_MOD_DST_WT4      39

#define SID_SE_NUM_MOD_DST      40

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


#define SID_SE_NUM_ENGINES 4
typedef enum sid_se_engine_t {
    SID_SE_LEAD = 0,
    SID_SE_BASSLINE,
    SID_SE_DRUM,
    SID_SE_MULTI
} sid_se_engine_t;

typedef union {
    u8 ALL;
    struct {
        u8 sid_type:2; // 0=no SID restriction, 1=6581, 2=6582/8580, 3=SwinSID
        u8 reserved:1;
        u8 stereo:1;
        u8 caps:4; // 0=470pF, 1=1nF, 2=2.2nF, 3=4.7nF, 4=6.8nF, 5=10nF, 6=22nF, 7=47nF, 8=100nF
    };
} sid_se_hw_flags_t;

typedef union {
    u16 ALL;
    struct {
        u8 ABW:1; // ADSR Bug Workaround
    };
} sid_se_opt_flags_t;


////////////////////////////////////////
// Clock
////////////////////////////////////////

typedef union {
    u8 ALL;
    struct {
        u8 SLAVE:1;
    };
} sid_se_clk_state_t;

typedef union {
    u8 ALL;
    struct {
        u8 CLK:1;
        u8 MIDI_START:1;
        u8 MIDI_CONTINUE:1;
        u8 MIDI_STOP:1;

        u8 MIDI_START_REQ:1;
        u8 MIDI_CONTINUE_REQ:1;
        u8 MIDI_STOP_REQ:1;
    };

    struct {
        u8 ALL_SE:4;
        u8 ALL_MIDI:3;
    };
} sid_se_clk_event_t;

typedef struct sid_se_clk_t {
    sid_se_clk_state_t state;
    sid_se_clk_event_t event;
    u16 incoming_clk_ctr;
    u16 incoming_clk_delay;
    u16 sent_clk_delay;
    u8 sent_clk_ctr;
    u8 clk_req_ctr;
    u8 global_clk_ctr6;
    u8 global_clk_ctr24;
    u32 tmp_bpm_ctr; // tmp.
    u32 tmp_bpm_ctr_mod; // tmp.
} sid_se_clk_t;


////////////////////////////////////////
// Trigger Matrix
////////////////////////////////////////
typedef union {
    //u32 ALL32;
    u8 ALL[3];
    struct {
        u8 O1L:1; // OSC1 Left
        u8 O2L:1; // OSC2 Left
        u8 O3L:1; // OSC3 Left
        u8 O1R:1; // OSC1 Right
        u8 O2R:1; // OSC2 Right
        u8 O3R:1; // OSC3 Right
        u8 E1A:1; // ENV1 Attack
        u8 E2A:1; // ENV2 Attack
        u8 E1R:1; // ENV1 Release
        u8 E2R:1; // ENV2 Release
        u8 L1:1; // LFO1 Sync
        u8 L2:1; // LFO2 Sync
        u8 L3:1; // LFO3 Sync
        u8 L4:1; // LFO4 Sync
        u8 L5:1; // LFO5 Sync
        u8 L6:1; // LFO6 Sync
        u8 W1R:1; // WT1 reset
        u8 W2R:1; // WT2 reset
        u8 W3R:1; // WT3 reset
        u8 W4R:1; // WT4 reset
        u8 W1S:1; // WT1 step
        u8 W2S:1; // WT2 step
        u8 W3S:1; // WT3 step
        u8 W4S:1; // WT4 step
    };
} sid_se_trg_t;


////////////////////////////////////////
// Drum model
////////////////////////////////////////
typedef enum {
    SID_DRUM_MODEL_WT = 0, // wavetable based model.    PAR1: GL offset, PAR2: Speed offset
} sid_drum_model_type_t ;

typedef struct sid_drum_model_t {
    char name[5];
    u8 type; // sid_drum_model_type_t
    u8 base_note;
    u8 gatelength;
    u8 waveform;
    u8 pulsewidth;
    u8 wt_speed;
    u8 wt_loop;
    u8 *wavetable;
} sid_drum_model_t;


////////////////////////////////////////
// MIDI Voice
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 ARP_ACTIVE:1;
        u8 ARP_UP:1;
        u8 SYNC_ARP:1;
        u8 HOLD_SAVED:1;
    };
} sid_se_arp_state_t;

typedef struct sid_se_midi_voice_t {
    u8     midi_voice; // number of MIDI voice

    u8 midi_channel;
    u8 split_lower;
    u8 split_upper;
    u8 transpose;
    u8 last_voice;
    sid_se_arp_state_t arp_state;
    u8 arp_div_ctr;
    u8 arp_gl_ctr;
    u8 arp_note_ctr;
    u8 arp_oct_ctr;
    u8 pitchbender;
    u8 wt_stack[4];
    notestack_t notestack;
    notestack_item_t notestack_items[SID_SE_NOTESTACK_SIZE];
} sid_se_midi_voice_t;


////////////////////////////////////////
// Voice
////////////////////////////////////////
typedef union {
    u16 ALL;
    struct {
        u8 VOICE_ACTIVE:1;
        u8 VOICE_DISABLED:1;
        u8 GATE_ACTIVE:1;
        u8 GATE_SET_REQ:1;
        u8 GATE_CLR_REQ:1;
        u8 OSC_SYNC_IN_PROGRESS:1;
        u8 PORTA_ACTIVE:1;
        u8 PORTA_INITIALIZED:1; // portamento omitted when first key played 
        u8 ACCENT:1;
        u8 SLIDE:1;
        u8 FORCE_FRQ_RECALC:1; // forces SID frequency re-calculation (if SID frequency registers have been changed externally)
    };
} sid_se_voice_state_t;

typedef union {
    u8 ALL;
    struct {
        u8 PORTA_MODE:2; // portamento (0), constant time glide (1), glissando (2)
        u8 GSA:1; // gate always enabled
    };
} sid_se_voice_flags_t;

typedef union {
    u8 ALL;
    struct {
        u8 WAVEFORM:4;
        u8 VOICE_OFF:1;
        u8 SYNC:1;
        u8 RINGMOD:1;
        u8 RESERVED:1;
    };
} sid_se_voice_waveform_t;

typedef union {
    u8 ALL;
    struct {
        u8 DECAY:4;
        u8 ATTACK:4;
    };
} sid_se_voice_ad_t;

typedef union {
    u8 ALL;
    struct {
        u8 SUSTAIN:4;
        u8 RELEASE:4;
    };
} sid_se_voice_sr_t;

typedef union {
    u8 ALL;
    struct {
        u8 ENABLE:1;
        u8 DIR:3; // up/down/U&D/D&U/Random
        u8 SORTED:1;
        u8 HOLD:1;
        u8 SYNC:1;
        u8 CAC:1;
    };
} sid_se_voice_arp_mode_t;

typedef union {
    u8 ALL;
    struct {
        u8 DIV:6;
        u8 EASY_CHORD:1;
        u8 ONESHOT:1;
    };
} sid_se_voice_arp_speed_div_t;

typedef union {
    u8 ALL;
    struct {
        u8 GATELENGTH:5;
        u8 OCTAVE_RANGE:3;
    };
} sid_se_voice_arp_gl_rng_t;

typedef union {
    u8 ALL;
    struct {
        u8 WAVEFORM_SECOND:4;
        u8 ENABLE_SECOND:1;
        u8 PITCHX2:1;
        u8 REVERSED_WAVEFORM:1;
    };
} sid_se_voice_swinsid_mode_t;

typedef union {
    u8 ALL;
    struct {
        u8 LEGATO:1; // mono/legato
        u8 WTO:1; // Wavetable Only
        u8 SUSKEY:1;
        u8 POLY:1; // Multi Engine only
        u8 OSC_PHASE_SYNC:1; // only used by Bassline and Multi Engine, Lead engine uses osc_phase parameter
    };

    struct {
        u8 dummy:4;
        u8 VOICE_ASG:4; // only used by Drum engine
    } D;

} sid_se_v_flags_t;

typedef union {
    struct {
        u8 flags; // sid_se_voice_flags_t 
        u8 waveform; // sid_se_voice_waveform_t 
        u8 ad; // sid_se_voice_ad_t
        u8 sr; // sid_se_voice_sr_t 
        u8 pulsewidth_l; // [11:0] only
        u8 pulsewidth_h;
        u8 accent; // used by Bassline Engine only - Lead engine uses this to set SwinSID Phase (TODO: separate unions for Lead/Bassline?)
        u8 delay; // 8bit
        u8 transpose; // 7bit
        u8 finetune; // 8bit
        u8 pitchrange; // 7bit
        u8 portamento; // 8bit
        u8 arp_mode; // sid_se_voice_arp_mode_t 
        u8 arp_speed_div; // sid_se_voice_arp_speed_div_t 
        u8 arp_gl_rng; // sid_se_voice_arp_gl_rng_t 
        u8 swinsid_mode; // sid_se_voice_swinsid_mode_t 
    };

    struct {
        u8 dummy[16]; // same as defined above
    } L;

    struct {
        u8 dummy[16]; // same as defined above

        // extended parameters
        u8 v_flags; // sid_se_v_flags_t
        u8 reserved1; // dedicated Voice (as for multi engine) not used
        u8 reserved2; // dedicated velocity assignment (as for multi engine) not used
        u8 reserved3; // dedicated pitchbender assignment (as for multi engine) not used
        u8 lfo[2][7]; // sid_se_lfo_patch_t.MINIMAL
        u8 env[9]; // sid_se_env_patch_t.MINIMAL
        u8 seq[5]; // sid_se_seq_patch_t.B
        u8 env_decay_a; // envelope decay used on accented notes
        u8 reserved4[15]; // free offsets: 0x31..0x3f
        u8 v2_waveform;
        u8 v2_pulsewidth_l;
        u8 v2_pulsewidth_h;
        u8 v2_oct_transpose;
        u8 v2_static_note;
        u8 v2_reserved[3]; // free offset: 0x45..0x47
        u8 v3_waveform;
        u8 v3_pulsewidth_l;
        u8 v3_pulsewidth_h;
        u8 v3_oct_transpose;
        u8 v3_static_note;
        u8 v3_reserved[3]; // free offset: 0x4d..0x4f
    } B;

    struct {
        u8 v_flags; // sid_se_v_flags_t
        u8 drum_model; // drum model
        u8 ad; // sid_se_voice_ad_t
        u8 sr; // sid_se_voice_sr_t 
        u8 tune; // 8bit
        u8 par1; // 8bit
        u8 par2; // 8bit
        u8 par3; // 8bit
        u8 velocity_asg; // optional velocity assignment
        u8 reserved1; // reserved for future extensions
    } D;

    struct {
        u8 dummy[16]; // same as defined above

        // extended parameters
        u8 v_flags; // sid_se_v_flags_t
        u8 voice_asg; // [3:0] voice assignment, [7:4] reserved
        u8 velocity_asg; // optional velocity assignment
        u8 pitchbender_asg; // optional pitchbender assignment
        u8 lfo[2][7]; // sid_se_lfo_patch_t.MINIMAL
        u8 env[9]; // sid_se_env_patch_t.MINIMAL
        u8 wt[5]; // sid_se_wt_patch_t
    } M;

} sid_se_voice_patch_t;

typedef struct sid_se_voice_t {
    sid_se_engine_t engine; // engine type
    u8     phys_sid; // number of assigned physical SID (chip)
    u8     voice; // number of assigned voice
    u8     phys_voice; // number of assigned physical SID voice
    sid_voice_t *phys_sid_voice; // reference to SID register
    sid_se_midi_voice_t *mv; // reference to assigned MIDI voice
    sid_se_voice_patch_t *voice_patch; // reference to Voice part of a patch
    s32    *mod_dst_pitch; // reference to SID_SE_MOD_DST_PITCHx
    s32    *mod_dst_pw; // reference to SID_SE_MOD_DST_PWx
    sid_se_trg_t *trg_mask_note_on;
    sid_se_trg_t *trg_mask_note_off;
    sid_drum_model_t *dm;

    sid_se_voice_state_t state;
    u8     note_restart_req;
    u8     note;
    u8     arp_note;
    u8     played_note;
    u8     prev_transposed_note;
    u8     glissando_note;
    u16    portamento_begin_frq;
    u16    portamento_end_frq;
    u16    portamento_ctr;
    u16    linear_frq;
    u16    set_delay_ctr;
    u16    clr_delay_ctr;

    u8     assigned_instrument;

    u8     drum_waveform;
    u8     drum_gatelength;
    u8     drum_wt_speed;
    u8     drum_par3;
} sid_se_voice_t;


////////////////////////////////////////
// Filters
////////////////////////////////////////
typedef struct sid_se_filter_patch_t {
    u8 chn_mode;
    u8 cutoff_l; // 12bit, 11 bits used - bit 7 controls FIP (interpolation option)
    u8 cutoff_h;
    u8 resonance; // 8 bit, 4 bits used)
    u8 keytrack;
    u8 reserved;
} sid_se_filter_patch_t;


typedef struct sid_se_filter_t {
    sid_se_engine_t engine; // engine type
    u8     phys_sid; // number of assigned physical SID (chip)
    u8     filter; // number of assigned filter
    s32    *mod_dst_filter; // reference to SID_SE_MOD_DST_FILTERx
    s32    *mod_dst_volume; // reference to SID_SE_MOD_DST_VOLUMEx

    sid_regs_t *phys_sid_regs; // reference to SID registers
    sid_se_filter_patch_t *filter_patch; // reference to filter part of a patch
} sid_se_filter_t;


////////////////////////////////////////
// LFO
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 ENABLE:1;
        u8 KEYSYNC:1; // *not* used for lead engine (keysync via trigger matrix)
        u8 CLKSYNC:1;
        u8 ONESHOT:1;
        u8 WAVEFORM:4;
    };
} sid_se_lfo_mode_t;

typedef union {
    struct {
        u8 mode; // sid_se_lfo_mode_t
        u8 depth;
        u8 rate;
        u8 delay;
        u8 phase;
    };

    struct {
        u8 mode; // sid_se_lfo_mode_t
        u8 depth;
        u8 rate;
        u8 delay;
        u8 phase;
    } L;

    struct {
        u8 mode; // sid_se_lfo_mode_t
        u8 depth_p;
        u8 rate;
        u8 delay;
        u8 phase;
        u8 depth_pw;
        u8 depth_f;
    } MINIMAL;

} sid_se_lfo_patch_t;

typedef struct sid_se_lfo_t {
    u8     lfo; // number of assigned LFO
    sid_se_engine_t engine; // engine type
    sid_se_lfo_patch_t *lfo_patch; // cross-reference to LFO patch
    s16    *mod_src_lfo; // reference to SID_SE_MOD_SRC_LFOx
    s32    *mod_dst_lfo_depth; // reference to SID_SE_MOD_DST_LDx
    s32    *mod_dst_lfo_rate; // reference to SID_SE_MOD_DST_LRx
    s32    *mod_dst_pitch; // reference to SID_SE_MOD_DST_PITCHx
    s32    *mod_dst_pw; // reference to SID_SE_MOD_DST_PWx
    s32    *mod_dst_filter; // reference to SID_SE_MOD_DST_FILx
    sid_se_trg_t *trg_mask_lfo_period;

    u16 ctr;
    u16 delay_ctr;
    u8  restart_req;
} sid_se_lfo_t;


////////////////////////////////////////
// Envelope
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 LOOP_BEGIN:3;
        u8 RESERVED1:1;
        u8 LOOP_END:3;
        u8 CLKSYNC:1;
    } L;

    struct {
        u8 RESERVED:4;
        u8 CURVE_A:1;
        u8 CURVE_D:1;
        u8 CURVE_R:1;
        u8 CLKSYNC:1;
    } MINIMAL;
} sid_se_env_mode_t;

typedef enum {
    SID_SE_ENV_STATE_IDLE = 0,
    SID_SE_ENV_STATE_ATTACK1,
    SID_SE_ENV_STATE_ATTACK2,
    SID_SE_ENV_STATE_DECAY1,
    SID_SE_ENV_STATE_DECAY2,
    SID_SE_ENV_STATE_SUSTAIN,
    SID_SE_ENV_STATE_RELEASE1,
    SID_SE_ENV_STATE_RELEASE2
} sid_se_env_state_t;


typedef union {
    u8 mode; // sid_se_env_mode_t

    struct {
        u8 mode; // sid_se_env_mode_t
        u8 depth;
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
        u8 att_curve;
        u8 dec_curve;
        u8 rel_curve;
    } L;

    struct {
        u8 mode; // sid_se_env_mode_t
        u8 depth_p;
        u8 depth_pw;
        u8 depth_f;
        u8 attack;
        u8 decay;
        u8 sustain;
        u8 release;
        u8 curve;
    } MINIMAL;

} sid_se_env_patch_t;

typedef struct sid_se_env_t {
    u8     env; // number of assigned envelope
    sid_se_env_patch_t *env_patch; // cross-reference to ENV patch
    s16    *mod_src_env; // reference to SID_SE_MOD_SRC_ENVx
    s32    *mod_dst_pitch; // reference to SID_SE_MOD_DST_PITCHx
    s32    *mod_dst_pw; // reference to SID_SE_MOD_DST_PWx
    s32    *mod_dst_filter; // reference to SID_SE_MOD_DST_FILx
    sid_se_trg_t *trg_mask_env_sustain;
    sid_se_voice_state_t *voice_state; // reference to voice state (to check for ACCENT)
    u8     *decay_a; // reference to alternative decay value (used on ACCENTed notes)
    u8     *accent; // reference to accent value (used on ACCENTed notes)

    sid_se_env_state_t state;
    u16 ctr;
    u16 delay_ctr;
    u8 restart_req;
    u8 release_req;
} sid_se_env_t;


////////////////////////////////////////
// Wavetable
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 CLKDIV:6;
        u8 CHANNEL_TARGET_SIDL:1;
        u8 CHANNEL_TARGET_SIDR:1;
    };
} sid_se_wt_speed_par_t;

typedef struct {
    u8 speed; // sid_se_wt_speed_par_t
    u8 assign;
    u8 begin; // [6:0] start position in wavetable, [7] position controlled by MOD
    u8 end; // [6:0] end position in wavetable, [7] position controlled by KEY
    u8 loop; // [6:0] loop position in wavetable, [7] one shot
} sid_se_wt_patch_t;

typedef struct sid_se_wt_t {
    u8     wt; // number of assigned WT
    sid_se_wt_patch_t *wt_patch; // cross-reference to WT patch
    s16    *mod_src_wt; // reference to SID_SE_MOD_SRC_WTx
    s32    *mod_dst_wt; // reference to SID_SE_MOD_DST_WTx

    u8 pos;
    u16 div_ctr; // should be >8bit for Drum mode (WTs clocked independent from BPM generator)
    u8 restart_req;
    u8 clk_req;
} sid_se_wt_t;


////////////////////////////////////////
// Modulation Pathes
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 OP:4;
        u8 RESERVED:2;
        u8 INVERT_TARGET_1;
        u8 INVERT_TARGET_2;
    };
} sid_se_mod_op_t;

typedef union {
    u8 ALL;
    struct {
        u8 PITCH_1:1;
        u8 PITCH_2:1;
        u8 PITCH_3:1;
        u8 PW_1:1;
        u8 PW_2:1;
        u8 PW_3:1;
        u8 FILTER:1;
        u8 VOLUME:1;
    };
} sid_se_mod_direct_targets_t;

typedef struct sid_se_mod_patch_t {
    u8 src1;
    u8 src2;
    u8 op; // sid_se_mod_op_t
    u8 depth;
    u8 direct_target[2]; // L/R, sid_se_mod_direct_targets_t
    u8 x_target[2];
} sid_se_mod_patch_t;


////////////////////////////////////////
// Sequencer (Bassline/Drum Mode)
////////////////////////////////////////
typedef union {
    u8 ALL;
    struct {
        u8 ENABLED:1;
        u8 RUNNING:1;
    };
} sid_se_seq_state_t;

typedef union {
    u8 ALL;
    struct {
        u8 CLKDIV:6;
        u8 SEQ_ON:1; // only used by drum sequencer
        u8 SYNCH_TO_MEASURE:1; // also called "16step sync"
    };
} sid_se_seq_speed_par_t;

typedef union {
    u8 ALL;
    struct {
        u8 NOTE:4;
        u8 OCTAVE:2;
        u8 SLIDE:1;
        u8 GATE:1;
    };
} sid_se_seq_note_item_t;

typedef union {
    u8 ALL;
    struct {
        u8 PAR_VALUE:7;
        u8 ACCENT:1;
    };
} sid_se_seq_asg_item_t;

typedef struct {
    u8 speed; // sid_se_seq_speed_par_t
    u8 num; // 0-8, 8=disabled
    u8 length; // [3:0] steps: 0-15
    u8 assign; // parameter assignment
    u8 reserved;
} sid_se_seq_patch_t;

typedef struct sid_se_seq_t {
    u8     seq; // number of assigned SEQ
    sid_se_seq_patch_t *seq_patch; // cross-reference to Seq patch
    sid_se_seq_patch_t *seq_patch_shadow; // cross-reference to Seq shadow patch
    sid_se_voice_t *v; // reference to assigned voice

    sid_se_seq_state_t state;
    u8 pos;
    u8 div_ctr;
    u8 sub_ctr;
    u8 restart_req;
} sid_se_seq_t;




/////////////////////////////////////////////////////////////////////////////
// Patch structures
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////
// Combined Patch Structure
////////////////////////////////////////
typedef union {
    u8 ALL[512];

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
// Knobs
/////////////////////////////////////////////////////////////////////////////
typedef enum {
  SID_KNOB_1 = 0,
  SID_KNOB_2,
  SID_KNOB_3,
  SID_KNOB_4,
  SID_KNOB_5,
  SID_KNOB_VELOCITY,
  SID_KNOB_PITCHBENDER,
  SID_KNOB_AFTERTOUCH
} sid_knob_num_t;

#endif /* _MBSID_STRUCTS_H */
