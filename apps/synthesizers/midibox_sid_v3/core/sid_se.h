// $Id$
/*
 * Header file for MBSID Sound Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_SE_H
#define _SID_SE_H

#include <sid.h>
#include <notestack.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SID_SE_NUM_VOICES      6
#define SID_SE_NUM_FILTERS     2 // must be at least 2 for all engines
#define SID_SE_NUM_LFO         (2*SID_SE_NUM_VOICES) // must be at least SID_SE_NUM_VOICES for lead engine, and 2*SID_SE_NUM_VOICES for multi engine
#define SID_SE_NUM_ENV         (2*SID_SE_NUM_VOICES) // must be at least 2 for lead engine, and 2*SID_SE_NUM_VOICES for multi engine
#define SID_SE_NUM_WT          (SID_SE_NUM_VOICES) // must be at least 4 for lead engine, and SID_SE_NUM_VOICES for multi engine

#define SID_SE_NOTESTACK_SIZE 10


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
    unsigned sid_type:2; // 0=no SID restriction, 1=6581, 2=6582/8580, 3=SwinSID
    unsigned reserved:1;
    unsigned stereo:1;
    unsigned caps:4; // 0=470pF, 1=1nF, 2=2.2nF, 3=4.7nF, 4=6.8nF, 5=10nF, 6=22nF, 7=47nF, 8=100nF
  };
} sid_se_hw_flags_t;

typedef union {
  u16 ALL;
  struct {
    unsigned ABW:1; // ADSR Bug Workaround
  };
} sid_se_opt_flags_t;


////////////////////////////////////////
// Clock
////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    unsigned CLK:1;
    unsigned MIDI_START:1;
    unsigned MIDI_CONTINUE:1;
    unsigned MIDI_STOP:1;

    unsigned MIDI_START_REQ:1;
    unsigned MIDI_CONTINUE_REQ:1;
    unsigned MIDI_STOP_REQ:1;
  };

  struct {
    unsigned ALL_SE:4;
    unsigned ALL_MIDI:3;
  };
} sid_se_clk_event_t;

typedef struct sid_se_clk_t {
  sid_se_clk_event_t event;
  u16 incoming_clk_ctr;
  u16 incoming_clk_delay;
  u16 sent_clk_delay;
  u8 sent_clk_ctr;
  u8 clk_req_ctr;
  u8 global_clk_ctr6;
  u8 global_clk_ctr24;
  u8 tmp_bpm_ctr; // tmp.
} sid_se_clk_t;


////////////////////////////////////////
// Voice
////////////////////////////////////////
typedef union {
  u16 ALL;
  struct {
    unsigned VOICE_ACTIVE:1;
    unsigned VOICE_DISABLED:1;
    unsigned GATE_ACTIVE:1;
    unsigned GATE_SET_REQ:1;
    unsigned GATE_CLR_REQ:1;
    unsigned PORTA_ACTIVE:1;
    unsigned PORTA_INITIALIZED:1; // portamento omitted when first key played 
    unsigned ACCENT:1;
    unsigned SLIDE:1;
    unsigned FRQ_RECALC:1; // forces SID frequency re-calculation (used by multi-engine to takeover portamento)
  };
} sid_se_voice_state_t;

typedef union {
  u8 ALL;
  struct {
    unsigned PORTA_MODE:2; // portamento (0), constant time glide (1), glissando (2)
    unsigned GSA:1; // gate always enabled
  };
} sid_se_voice_flags_t;

typedef union {
  u8 ALL;
  struct {
    unsigned WAVEFORM:4;
    unsigned VOICE_OFF:1;
    unsigned SYNC:1;
    unsigned RINGMOD:1;
    unsigned RESERVED:1;
  };
} sid_se_voice_waveform_t;

typedef union {
  u8 ALL;
  struct {
    unsigned DECAY:4;
    unsigned ATTACK:4;
  };
} sid_se_voice_ad_t;

typedef union {
  u8 ALL;
  struct {
    unsigned SUSTAIN:4;
    unsigned RELEASE:4;
  };
} sid_se_voice_sr_t;

typedef union {
  u8 ALL;
  struct {
    unsigned ENABLE:1;
    unsigned DIR:3; // up/down/U&D/D&U/Random
    unsigned SORTED:1;
    unsigned HOLD:1;
    unsigned SYNC:1;
    unsigned CAC:1;
  };
} sid_se_voice_arp_mode_t;

typedef union {
  u8 ALL;
  struct {
    unsigned DIV:6;
    unsigned EASY_CHORD:1;
    unsigned ONESHOT:1;
  };
} sid_se_voice_arp_speed_div_t;

typedef union {
  u8 ALL;
  struct {
    unsigned GATELENGTH:5;
    unsigned OCTAVE_RANGE:3;
  };
} sid_se_voice_arp_gl_rng_t;

typedef union {
  u8 ALL;
  struct {
    unsigned ARP_ACTIVE:1;
    unsigned ARP_UP:1;
    unsigned SYNC_ARP:1;
    unsigned HOLD_SAVED:1;
  };
} sid_se_arp_state_t;

typedef union {
  u8 ALL;
  struct {
    unsigned WAVEFORM_SECOND:4;
    unsigned ENABLE_SECOND:1;
    unsigned PITCHX2:1;
    unsigned REVERSED_WAVEFORM:1;
  };
} sid_se_voice_swinsid_mode_t;

typedef struct sid_se_voice_patch_t {
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
} sid_se_voice_patch_t;

typedef struct sid_se_voice_t {
  u8     sid;   // number of assigned SID engine
  u8     phys_sid; // number of assigned physical SID (chip)
  u8     voice; // number of assigned voice
  u8     phys_voice; // number of assigned physical SID voice
  sid_voice_t *phys_sid_voice; // reference to SID register
  sid_se_voice_patch_t *voice_patch; // reference to Voice part of a patch
  sid_se_clk_t *clk; // reference to clock generator
  s32    *mod_dst_pitch; // reference to SID_SE_MOD_DST_PITCHx
  s32    *mod_dst_pw; // reference to SID_SE_MOD_DST_PWx
  u8     *trg_mask_note_on;
  u8     *trg_mask_note_off;
  u8     *trg_dst;

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
  u8 wt_stack[4];
  notestack_t notestack;
  notestack_item_t notestack_items[SID_SE_NOTESTACK_SIZE];

  sid_se_voice_state_t state;
  u8     assigned_mv;
  u8     note;
  u8     arp_note;
  u8     played_note;
  u8     prev_transposed_note;
  u8     glissando_note;
  u8     pitchbender;
  u16    portamento_begin_frq;
  u16    portamento_end_frq;
  u16    portamento_ctr;
  u16    linear_frq;
  u16    set_delay_ctr;
  u16    clr_delay_ctr;
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
  u8     sid;   // number of assigned SID engine
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
    unsigned ENABLE:1;
    unsigned RESERVED1:1;
    unsigned CLKSYNC:1;
    unsigned ONESHOT:1;
    unsigned WAVEFORM:4;
  };
} sid_se_lfo_mode_t;

typedef struct sid_se_lfo_patch_t {
  u8 mode; // sid_se_lfo_mode_t
  u8 depth;
  u8 rate;
  u8 delay;
  u8 phase;
} sid_se_lfo_patch_t;

typedef struct sid_se_lfo_t {
  u8     sid; // number of assigned SID engine
  u8     lfo; // number of assigned LFO
  sid_se_lfo_patch_t *lfo_patch; // cross-reference to LFO patch
  s16    *mod_src_lfo; // reference to SID_SE_MOD_SRC_LFOx
  s32    *mod_dst_lfo_depth; // reference to SID_SE_MOD_DST_LDx
  s32    *mod_dst_lfo_rate; // reference to SID_SE_MOD_DST_LRx
  u8     *trg_mask_lfo_period;
  u8     *trg_dst;

  u16 ctr;
  u16 delay_ctr;
} sid_se_lfo_t;


////////////////////////////////////////
// Envelope
////////////////////////////////////////
typedef union {
  u8 ALL;
  struct {
    unsigned LOOP_BEGIN:3;
    unsigned RESERVED1:1;
    unsigned LOOP_END:3;
    unsigned RESERVED2:1;
  };
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


typedef struct sid_se_env_patch_t {
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
} sid_se_env_patch_t;

typedef struct sid_se_env_t {
  u8     sid; // number of assigned SID engine
  u8     env; // number of assigned envelope
  sid_se_env_patch_t *env_patch; // cross-reference to ENV patch
  s16    *mod_src_env; // reference to SID_SE_MOD_SRC_ENVx
  u8     *trg_mask_env_sustain;
  u8     *trg_dst;

  sid_se_env_state_t state;
  u16 ctr;
  u16 delay_ctr;
} sid_se_env_t;


////////////////////////////////////////
// Wavetable
////////////////////////////////////////
typedef union {
  u8 ALL;
  struct {
    unsigned CLKDIV:6;
    unsigned CHANNEL_TARGET_SIDL:1;
    unsigned CHANNEL_TARGET_SIDR:1;
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
  u8     sid; // number of assigned SID engine
  u8     wt; // number of assigned WT
  sid_se_wt_patch_t *wt_patch; // cross-reference to WT patch
  s16    *mod_src_wt; // reference to SID_SE_MOD_SRC_WTx
  s32    *mod_dst_wt; // reference to SID_SE_MOD_DST_WTx
  u8     *trg_src;

  u8 pos;
  u8 div_ctr;
} sid_se_wt_t;


////////////////////////////////////////
// Modulation Pathes
////////////////////////////////////////
typedef union {
  u8 ALL;
  struct {
    unsigned OP:4;
    unsigned RESERVED:2;
    unsigned INVERT_TARGET_1;
    unsigned INVERT_TARGET_2;
  };
} sid_se_mod_op_t;

typedef union {
  u8 ALL;
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
// Trigger Matrix
////////////////////////////////////////
typedef union {
  u32 ALL32;
  u8 ALL[4];
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
} sid_se_trg_t;


////////////////////////////////////////
// Sequencer (Bassline/Drum Mode)
////////////////////////////////////////
typedef union {
  u8 ALL;
  struct {
    unsigned ENABLED:1;
    unsigned RUNNING:1;
    unsigned SUBCTR:3;
  };
} sid_se_seq_state_t;

typedef struct sid_se_seq_t {
  sid_se_seq_state_t state;
  u8 pos;
  u8 div_ctr;
} sid_se_seq_t;


////////////////////////////////////////
// General SID Engine variables
////////////////////////////////////////

typedef struct sid_se_vars_t {
  u8     sid; // number of assigned SID engine

  sid_se_trg_t triggers;

  s16 mod_src[SID_SE_NUM_MOD_SRC];
  s32 mod_dst[SID_SE_NUM_MOD_DST];
  u8 mod_transition;
  u8 lfo_overrun;
} sid_se_vars_t;




/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_SE_Init(u32 mode);

extern s32 SID_SE_VarsInit(sid_se_vars_t *vars, u8 sid);
extern s32 SID_SE_ClkInit(sid_se_clk_t *clk);
extern s32 SID_SE_VoiceInit(sid_se_voice_t *v, u8 sid, u8 voice);
extern s32 SID_SE_FilterInit(sid_se_filter_t *f, u8 sid, u8 filter);
extern s32 SID_SE_LFOInit(sid_se_lfo_t *l, u8 sid, u8 lfo);
extern s32 SID_SE_ENVInit(sid_se_env_t *e, u8 sid, u8 env);
extern s32 SID_SE_WTInit(sid_se_wt_t *w, u8 sid, u8 wt);
extern s32 SID_SE_SEQInit(sid_se_seq_t *seq);

extern s32 SID_SE_Update(void);

extern s32 SID_SE_IncomingRealTimeEvent(u8 event);
extern s32 SID_SE_Clk(sid_se_clk_t *clk);
extern s32 SID_SE_Gate(sid_se_voice_t *v);
extern s32 SID_SE_Pitch(sid_se_voice_t *v);
extern s32 SID_SE_PW(sid_se_voice_t *v);
extern s32 SID_SE_FilterAndVolume(sid_se_filter_t *f);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 sid_se_speed_factor;

extern sid_se_vars_t sid_se_vars[SID_NUM];
extern sid_se_clk_t sid_se_clk;
extern sid_se_voice_t sid_se_voice[SID_NUM][SID_SE_NUM_VOICES];
extern sid_se_filter_t sid_se_filter[SID_NUM][SID_SE_NUM_FILTERS];
extern sid_se_lfo_t sid_se_lfo[SID_NUM][SID_SE_NUM_LFO];
extern sid_se_env_t sid_se_env[SID_NUM][SID_SE_NUM_ENV];
extern sid_se_wt_t sid_se_wt[SID_NUM][SID_SE_NUM_WT];
extern sid_se_seq_t sid_se_seq[SID_NUM];

extern const u16 sid_se_frq_table[128];
extern const u16 sid_se_env_table[256];
extern const u16 sid_se_lfo_table[256];
extern const u16 sid_se_lfo_table_mclk[11];
extern const u16 sid_se_sin_table[128];

#endif /* _SID_SE_H */
