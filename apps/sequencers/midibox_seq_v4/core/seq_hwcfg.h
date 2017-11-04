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

#define SEQ_HWCFG_NUM_ENCODERS     18

#define SEQ_HWCFG_NUM_GP           16
#define SEQ_HWCFG_NUM_TRACK         4
#define SEQ_HWCFG_NUM_GROUP         4
#define SEQ_HWCFG_NUM_DIRECT_TRACK 16
#define SEQ_HWCFG_NUM_PAR_LAYER     3
#define SEQ_HWCFG_NUM_TRG_LAYER     3
#define SEQ_HWCFG_NUM_DIRECT_BOOKMARK 16

// following constants can be safely changed (therefore documented)

// max. number of SRs which can be used for CV gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_CV_GATES 1

// max. number of SRs which can be used for triggering gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_DOUT_GATES 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 down;
  u16 up;
  u16 left;
  u16 right;

  u16 scrub;
  u16 metronome;
  u16 record;
  u16 jam_live;
  u16 jam_step;
  u16 live;

  u16 stop;
  u16 pause;
  u16 play;
  u16 rew;
  u16 fwd;
  u16 loop;
  u16 follow;

  u16 menu;
  u16 select;
  u16 exit;
  u16 bookmark;

  u16 track[SEQ_HWCFG_NUM_TRACK];

  u16 direct_track[SEQ_HWCFG_NUM_DIRECT_TRACK];

  u16 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u16 direct_bookmark[SEQ_HWCFG_NUM_DIRECT_BOOKMARK];

  u16 edit;
  u16 mute;
  u16 pattern;
  u16 song;
  u16 phrase;

  u16 solo;
  u16 fast;
  u16 fast2;
  u16 all;

  u16 gp[SEQ_HWCFG_NUM_GP];

  u16 group[SEQ_HWCFG_NUM_GROUP];

  u16 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u16 utility;
  u16 step_view;
  u16 trg_layer_sel;
  u16 par_layer_sel;
  u16 ins_sel;
  u16 track_sel;

  u16 tap_tempo;
  u16 tempo_preset;
  u16 ext_restart;

  u16 copy;
  u16 paste;
  u16 clear;
  u16 undo;
  u16 move;
  u16 scroll;

  u16 mixer;

  u16 save;
  u16 save_all;

  u16 track_mode;
  u16 track_groove;
  u16 track_length;
  u16 track_direction;
  u16 track_morph;
  u16 track_transpose;
  u16 fx;

  u16 mute_all_tracks;
  u16 mute_track_layers;
  u16 mute_all_tracks_and_layers;
  u16 unmute_all_tracks;
  u16 unmute_track_layers;
  u16 unmute_all_tracks_and_layers;

  u16 footswitch;
  u16 enc_btn_fwd;
  u16 pattern_remix;
} seq_hwcfg_button_t;


typedef struct {
  u32 fast:1;
  u32 fast2:1;
  u32 all:1;
  u32 all_with_triggers:1;
  u32 solo:1;
  u32 metronome:1;
  u32 loop:1;
  u32 follow:1;
  u32 scrub:1;
  u32 menu:1;
  u32 bookmark:1;
  u32 mute:1;
  u32 step_view:1;
  u32 trg_layer:1;
  u32 par_layer:1;
  u32 ins_sel:1;
  u32 track_sel:1;
  u32 tempo_preset:1;
} seq_hwcfg_button_beh_t;


typedef struct {
  u16 gp_dout_l_sr;
  u16 gp_dout_r_sr;
  u16 gp_dout_l2_sr;
  u16 gp_dout_r2_sr;
  u16 tracks_dout_l_sr;
  u16 tracks_dout_r_sr;

  u16 track[SEQ_HWCFG_NUM_TRACK];

  u16 par_layer[SEQ_HWCFG_NUM_PAR_LAYER];

  u16 beat;
  u16 measure;

  u16 midi_in_combined;
  u16 midi_out_combined;

  u16 edit;
  u16 mute;
  u16 pattern;
  u16 song;
  u16 phrase;

  u16 solo;
  u16 fast;
  u16 fast2;
  u16 all;

  u16 group[SEQ_HWCFG_NUM_GROUP];

  u16 trg_layer[SEQ_HWCFG_NUM_TRG_LAYER];

  u16 play;
  u16 stop;
  u16 pause;
  u16 rew;
  u16 fwd;
  u16 loop;
  u16 follow;

  u16 exit;
  u16 select;
  u16 menu;
  u16 bookmark;
  u16 scrub;
  u16 metronome;

  u16 utility;
  u16 copy;
  u16 paste;
  u16 clear;
  u16 undo;
  u16 move;
  u16 scroll;

  u16 record;
  u16 live;
  u16 jam_live;
  u16 jam_step;

  u16 step_view;
  u16 trg_layer_sel;
  u16 par_layer_sel;
  u16 ins_sel;
  u16 track_sel;

  u16 tap_tempo;
  u16 tempo_preset;
  u16 ext_restart;

  u16 down;
  u16 up;

  u16 mixer;

  u16 track_mode;
  u16 track_groove;
  u16 track_length;
  u16 track_direction;
  u16 track_transpose;
  u16 track_morph;
  u16 fx;

  u16 mute_all_tracks;
  u16 mute_track_layers;
  u16 mute_all_tracks_and_layers;
  u16 unmute_all_tracks;
  u16 unmute_track_layers;
  u16 unmute_all_tracks_and_layers;
} seq_hwcfg_led_t;


typedef struct {
  u8 datawheel_fast_speed;
  u8 bpm_fast_speed;
  u8 gp_fast_speed;
  u8 auto_fast;
} seq_hwcfg_enc_t;


typedef struct {
  u8 enabled:1;
  u8 dout_duocolour:2;
  u8 buttons_enabled:1;
  u8 buttons_no_ui:1;
  u8 gp_always_select_menu_page:1;
} seq_hwcfg_blm_t;


typedef struct {
  u8 enabled:1;
  u8 dout_gp_mapping:2;
} seq_hwcfg_blm8x8_t;

typedef struct {
  u8  enabled;
  u8  segments_sr;
  u16 common1_pin;
  u16 common2_pin;
  u16 common3_pin;
  u16 common4_pin;
} seq_hwcfg_bpm_digits_t;

typedef struct {
  u8 enabled;
  u8 segments_sr;
  u8 common1_pin;
  u8 common2_pin;
  u8 common3_pin;
} seq_hwcfg_step_digits_t;

typedef struct {
  u8 enabled;
  u8 columns_sr[2];
  u8 rows_sr_green[2];
  u8 rows_sr_red[2];
} seq_hwcfg_tpd_t;


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
extern seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits;
extern seq_hwcfg_step_digits_t seq_hwcfg_step_digits;
extern seq_hwcfg_tpd_t seq_hwcfg_tpd;

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;
extern u8 seq_hwcfg_cv_gate_sr[SEQ_HWCFG_NUM_SR_CV_GATES];
extern u8 seq_hwcfg_clk_sr;
extern u8 seq_hwcfg_j5_enabled;

#endif /* _SEQ_HWCFG_H */
