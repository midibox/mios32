// $Id$
/*
 * Header file for HW configuration routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_HWCFG_H
#define _SEQ_HWCFG_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// changing these constants requires changes in seq_hwcfg.c, seq_file_hw.c and probably seq_ui.c

#define SEQ_HWCFG_NUM_ENCODERS  17

#define SEQ_HWCFG_NUM_GP        16
#define SEQ_HWCFG_NUM_TRACK      4
#define SEQ_HWCFG_NUM_GROUP      4
#define SEQ_HWCFG_NUM_PAR_LAYER  3
#define SEQ_HWCFG_NUM_TRG_LAYER  3

// following constants can be safely changed (therefore documented)

// max. number of SRs which can be used for triggering gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_DOUT_GATES 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 down;
  u8 up;
  u8 left;
  u8 right;

  u8 scrub;
  u8 metronome;
  u8 record;

  u8 stop;
  u8 pause;
  u8 play;
  u8 rew;
  u8 fwd;
  u8 loop;
  u8 follow;

  u8 menu;
  u8 select;
  u8 exit;

  u8 track[SEQ_HWCFG_NUM_TRACK];

  u8 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u8 edit;
  u8 mute;
  u8 pattern;
  u8 song;

  u8 solo;
  u8 fast;
  u8 all;

  u8 gp[SEQ_HWCFG_NUM_GP];

  u8 group[SEQ_HWCFG_NUM_GROUP];

  u8 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u8 utility;
  u8 step_view;
  u8 trg_layer_sel;
  u8 par_layer_sel;
  u8 track_sel;

  u8 tap_tempo;
  u8 tempo_preset;
  u8 ext_restart;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 morph;
  u8 mixer;
  u8 transpose;
} seq_hwcfg_button_t;


typedef struct {
  u8 fast:1;
  u8 all:1;
  u8 all_with_triggers:1;
  u8 solo:1;
  u8 metronome:1;
  u8 loop:1;
  u8 follow:1;
  u8 scrub:1;
  u8 menu:1;
  u8 step_view:1;
  u8 trg_layer:1;
  u8 par_layer:1;
  u8 track_sel:1;
  u8 tempo_preset:1;
} seq_hwcfg_button_beh_t;


typedef struct {
  u8 gp_dout_l_sr;
  u8 gp_dout_r_sr;
  u8 gp_dout_l2_sr;
  u8 gp_dout_r2_sr;

  u8 track[SEQ_HWCFG_NUM_TRACK];

  u8 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u8 beat;

  u8 edit;
  u8 mute;
  u8 pattern;
  u8 song;

  u8 solo;
  u8 fast;
  u8 all;

  u8 group[SEQ_HWCFG_NUM_GROUP];

  u8 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u8 play;
  u8 stop;
  u8 pause;
  u8 rew;
  u8 fwd;
  u8 loop;
  u8 follow;

  u8 exit;
  u8 select;
  u8 menu;
  u8 scrub;
  u8 metronome;

  u8 utility;
  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 record;

  u8 step_view;
  u8 trg_layer_sel;
  u8 par_layer_sel;
  u8 track_sel;

  u8 tap_tempo;
  u8 tempo_preset;
  u8 ext_restart;

  u8 down;
  u8 up;

  u8 morph;
  u8 mixer;
  u8 transpose;
} seq_hwcfg_led_t;


typedef struct {
  u8 datawheel_fast_speed;
  u8 gp_fast_speed;
  u8 auto_fast;
} seq_hwcfg_enc_t;


typedef struct {
  u8 enabled;
  u8 dout_duocolour;
  u8 buttons_enabled;
  u8 buttons_no_ui;
} seq_hwcfg_blm_t;


typedef struct {
  u8 enabled;
  u8 dout_gp_mapping;
} seq_hwcfg_blm8x8_t;

typedef struct {
  u8 key;
  u8 cc;
} seq_hwcfg_midi_remote_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_HWCFG_Init(u32 mode);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_hwcfg_button_t seq_hwcfg_button;
extern seq_hwcfg_button_beh_t seq_hwcfg_button_beh;
extern seq_hwcfg_led_t seq_hwcfg_led;
extern seq_hwcfg_blm_t seq_hwcfg_blm;
extern seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8;
extern seq_hwcfg_enc_t seq_hwcfg_enc;
extern seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote;

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;

#endif /* _SEQ_HWCFG_H */
