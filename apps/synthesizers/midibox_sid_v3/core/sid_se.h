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

#define SID_SE_NUM_VOICES      (3*SID_NUM) // SID_NUN is defined in sid.h
#define SID_SE_NUM_MIDI_VOICES (SID_SE_NUM_VOICES)
#define SID_SE_NUM_LFO         (2*SID_SE_NUM_VOICES)
#define SID_SE_NUM_ENV         (2*SID_SE_NUM_VOICES)
#define SID_SE_NUM_WT          (SID_SE_NUM_VOICES)
#define SID_SE_NUM_SEQ         (SID_NUM)

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



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef enum sid_se_engine_t {
  SID_SE_LEAD = 0,
  SID_SE_BASSLINE,
  SID_SE_DRUM,
  SID_SE_MULTI
} sid_se_engine_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned sid_type:2; // 0=no SID restriction, 1=6581, 2=6582/8580, 3=SwinSID
    unsigned reserved:1;
    unsigned stereo:1;
    unsigned caps:4; // 0=470pF, 1=1nF, 2=2.2nF, 3=4.7nF, 4=6.8nF, 5=10nF, 6=22nF, 7=47nF, 8=100nF
  };
} sid_se_hw_flags_t;

typedef union {
  struct {
    u16 ALL;
  };
  struct {
    unsigned ABW:1; // ADSR Bug Workaround
  };
} sid_se_opt_flags_t;



////////////////////////////////////////
// Voice
////////////////////////////////////////
typedef union {
  struct {
    u16 ALL;
  };
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
  struct {
    u8 ALL;
  };
  struct {
    unsigned PORTA_MODE:2; // portamento (0), constant time glide (1), glissando (2)
    unsigned GAE:1; // gate always enabled
  };
} sid_se_voice_flags_t;

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
} sid_se_voice_waveform_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned DECAY:4;
    unsigned ATTACK:4;
  };
} sid_se_voice_ad_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned SUSTAIN:4;
    unsigned RELEASE:4;
  };
} sid_se_voice_sr_t;

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
} sid_se_voice_arp_mode_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned DIV:6;
    unsigned EASY_CHORD:1;
    unsigned ONESHOT:1;
  };
} sid_se_voice_arp_speed_div_t;

typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned GATELENGTH:5;
    unsigned OCTAVE_RANGE:3;
  };
} sid_se_voice_arp_gl_rng_t;

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
  sid_se_voice_state_t state;
  u8     assigned_mv;
  u8     note;
  u8     arp_note;
  u8     played_note;
  u8     transp_note;
  u8     old_transp_note;
  u16    pitchbender;
  u16    old_target_frq;
  u16    linear_frq;
  u16    porta_freq;
  u16    porta_ctr;
  u16    set_delay_ctr;
  u16    clr_delay_ctr;
} sid_se_voice_t;


////////////////////////////////////////
// MIDI Voices
////////////////////////////////////////
typedef union {
  struct {
    u8 ALL;
  };
  struct {
    unsigned ARP_ACTIVE:1;
    unsigned ARP_UP:1;
    unsigned SYNC_ARP:1;
    unsigned HOLD_SAVED:1;
  };
} sid_se_arp_state_t;

typedef struct sid_se_midi_voice_t {
  u8 channel;
  u8 split_lower;
  u8 split_upper;
  u8 split_transpose;
  u16 pitchbender;
  u8 last_voice;
  sid_se_arp_state_t arp_state;
  u8 arp_div_ctr;
  u8 arp_gl_ctr;
  u8 arp_note_ctr;
  u8 wt_stack[4];
  notestack_t notestack;
  notestack_item_t notestack_items[SID_SE_NOTESTACK_SIZE];
} sid_se_midi_voice_t;


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



////////////////////////////////////////
// LFO
////////////////////////////////////////
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
} sid_se_lfo_mode_t;

typedef struct sid_se_lfo_patch_t {
  u8 mode; // sid_se_lfo_mode_t
  s8 depth;
  u8 rate;
  u8 delay;
  u8 phase;
} sid_se_lfo_patch_t;

typedef struct sid_se_lfo_t {
  u16 ctr;
  u16 delay_ctr;
} sid_se_lfo_t;


////////////////////////////////////////
// Envelope
////////////////////////////////////////
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
} sid_se_env_mode_t;

typedef enum sid_se_env_state_t {
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
} sid_se_env_patch_t;

typedef struct sid_se_env_t {
  sid_se_env_state_t state;
  u16 ctr;
  u16 delay_ctr;
} sid_se_env_t;


////////////////////////////////////////
// Wavetable
////////////////////////////////////////
typedef union {
  struct {
    u8 ALL;
  };
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
  u8 pos;
  u8 div_ctr;
} sid_se_wt_t;


////////////////////////////////////////
// Modulation Pathes
////////////////////////////////////////
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
} sid_se_mod_op_t;

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
} sid_se_mod_direct_targets_t;

typedef struct sid_se_mod_patch_t {
  u8 src1;
  u8 src2;
  u8 op; // sid_se_mod_op_t
  s8 depth;
  u8 direct_target[2]; // L/R, sid_se_mod_direct_targets_t
  u8 x_target[2];
} sid_se_mod_patch_t;


////////////////////////////////////////
// Trigger Matrix
////////////////////////////////////////
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
} sid_se_trg_t;


////////////////////////////////////////
// Sequencer (Bassline/Drum Mode)
////////////////////////////////////////
typedef union {
  struct {
    u8 ALL;
  };
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



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_SE_Init(u32 mode);

extern s32 SID_SE_VoiceInit(sid_se_voice_t *voice);
extern s32 SID_SE_MIDIVoiceInit(sid_se_midi_voice_t *midi_voice);
extern s32 SID_SE_LFOInit(sid_se_lfo_t *lfo);
extern s32 SID_SE_ENVInit(sid_se_env_t *env);
extern s32 SID_SE_WTInit(sid_se_wt_t *wt);
extern s32 SID_SE_SEQInit(sid_se_seq_t *seq);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern sid_se_voice_t sid_se_voice[SID_SE_NUM_VOICES];
extern sid_se_midi_voice_t sid_se_midi_voice[SID_SE_NUM_MIDI_VOICES];
extern sid_se_lfo_t sid_se_lfo[SID_SE_NUM_LFO];
extern sid_se_env_t sid_se_env[SID_SE_NUM_ENV];
extern sid_se_wt_t sid_se_wt[SID_SE_NUM_WT];
extern sid_se_seq_t sid_se_seq[SID_SE_NUM_SEQ];

#endif /* _SID_SE_H */
