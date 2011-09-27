/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * This file defines all structures used by MIDIbox CV
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_STRUCTS_H
#define _MBCV_STRUCTS_H

#include <mios32.h>
#include <notestack.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define CV_SE_NUM             8 // CV Channels

#define CV_SE_NOTESTACK_SIZE 10


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
    MBCV_MIDI_EVENT_MODE_NOTE        = 0,
    MBCV_MIDI_EVENT_MODE_VELOCITY    = 1,
    MBCV_MIDI_EVENT_MODE_AFTERTOUCH  = 2,
    MBCV_MIDI_EVENT_MODE_CC          = 3,
    MBCV_MIDI_EVENT_MODE_NRPN        = 4,
    MBCV_MIDI_EVENT_MODE_PITCHBENDER = 5,
} mbcv_midi_event_mode_t;
#define MBCV_MIDI_EVENT_MODE_NUM       6

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
        u8 PORTA_ACTIVE:1;
        u8 PORTA_INITIALIZED:1; // portamento omitted when first key played 
        u8 ACCENT:1;
        u8 SLIDE:1;
        u8 FORCE_FRQ_RECALC:1; // forces CV frequency re-calculation (if CV frequency registers have been changed externally)
    };
} cv_se_voice_state_t;

typedef union {
    u8 ALL;
    struct {
        u8 LEGATO:1; // mono/legato
        u8 SUSKEY:1;
        u8 POLY:1; // Multi Engine only
        u8 PORTA_MODE:2; // portamento (0), constant time glide (1), glissando (2)
    };
} cv_se_voice_flags_t;

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
} cv_se_voice_arp_mode_t;

typedef union {
    u8 ALL;
    struct {
        u8 DIV:6;
        u8 EASY_CHORD:1;
        u8 ONESHOT:1;
    };
} cv_se_voice_arp_speed_div_t;

typedef union {
    u8 ALL;
    struct {
        u8 GATELENGTH:5;
        u8 OCTAVE_RANGE:3;
    };
} cv_se_voice_arp_gl_rng_t;

typedef union {
    struct {
        u8 flags; // cv_se_voice_flags_t 
        u8 accent; // used by bassline sequencer
        u8 transpose; // 7bit
        u8 finetune; // 8bit
        u8 pitchrange; // 7bit
        u8 portamento; // 8bit
        u8 arp_mode; // cv_se_voice_arp_mode_t 
        u8 arp_speed_div; // cv_se_voice_arp_speed_div_t 
        u8 arp_gl_rng; // cv_se_voice_arp_gl_rng_t 
        u8 lfo[2][7]; // cv_se_lfo_patch_t.MINIMAL
        u8 env[9]; // cv_se_env_patch_t.MINIMAL
        u8 seq[5]; // cv_se_seq_patch_t.B
        u8 env_decay_a; // envelope decay used on accented notes
    };

} cv_se_voice_patch_t;

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
} cv_se_lfo_mode_t;

typedef union {
    struct {
        u8 mode; // cv_se_lfo_mode_t
        u8 depth_p;
        u8 rate;
        u8 delay;
        u8 phase;
        u8 depth_pw;
        u8 depth_f;
    };

} cv_se_lfo_patch_t;


////////////////////////////////////////
// Envelope
////////////////////////////////////////
typedef union {
    u8 ALL;

    struct {
        u8 RESERVED:4;
        u8 CURVE_A:1;
        u8 CURVE_D:1;
        u8 CURVE_R:1;
        u8 CLKSYNC:1;
    };
} cv_se_env_mode_t;

typedef union {

    struct {
        u8 mode; // cv_se_env_mode_t
        u8 depth_p;
        u8 depth_pw;
        u8 depth_f;
        u8 attack;
        u8 decay;
        u8 sustain;
        u8 release;
        u8 curve;
    };

} cv_se_env_patch_t;


////////////////////////////////////////
// Sequencer (Bassline)
////////////////////////////////////////

typedef union {
    u8 ALL;
    struct {
        u8 CLKDIV:6;
        u8 SEQ_ON:1; // only used by drum sequencer
        u8 SYNCH_TO_MEASURE:1; // also called "16step sync"
    };
} cv_se_seq_speed_par_t;

typedef union {
    u8 ALL;
    struct {
        u8 NOTE:4;
        u8 OCTAVE:2;
        u8 SLIDE:1;
        u8 GATE:1;
    };
} cv_se_seq_note_item_t;

typedef union {
    u8 ALL;
    struct {
        u8 PAR_VALUE:7;
        u8 ACCENT:1;
    };
} cv_se_seq_asg_item_t;

typedef struct {
    u8 speed; // cv_se_seq_speed_par_t
    u8 num; // 0-8, 8=disabled
    u8 length; // [3:0] steps: 0-15
    u8 assign; // parameter assignment
    u8 reserved;
} cv_se_seq_patch_t;


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

    };

} cv_patch_t;


#endif /* _MBCV_STRUCTS_H */
