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

  u8 stop;
  u8 pause;
  u8 play;
  u8 rew;
  u8 fwd;

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
  u8 sync_ext;

  u8 copy;
  u8 paste;
  u8 clear;
} seq_hwcfg_button_t;

typedef union {
  unsigned fast:1;
  unsigned all:1;
  unsigned solo:1;
  unsigned metronome:1;
  unsigned scrub:1;
  unsigned menu:1;
  unsigned step_view:1;
  unsigned trg_layer:1;
  unsigned par_layer:1;
  unsigned track_sel:1;
  unsigned tempo_preset:1;
} seq_hwcfg_button_beh_t;


typedef struct {
  u8 gp_dout_sr_l;
  u8 gp_dout_sr_r;
  u8 gp_dout_sr_l2;
  u8 gp_dout_sr_r2;

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

  u8 menu;
  u8 scrub;
  u8 metronome;

  u8 utility;
  u8 copy;
  u8 paste;
  u8 clear;

  u8 step_view;
  u8 trg_layer_sel;
  u8 par_layer_sel;
  u8 track_sel;

  u8 tap_tempo;
  u8 tempo_preset;
  u8 sync_ext;

  u8 down;
  u8 up;
} seq_hwcfg_led_t;


typedef union {
  u8 datawheel_fast_speed;
  u8 gp_fast_speed;
  u8 auto_fast;
} seq_hwcfg_enc_t;


typedef struct {
  u8 enabled;
  u8 dout_l1;
  u8 dout_r1;

  u8 dout_cathodes1;
  u8 dout_cathodes2;

  u8 dout_m_mapping;

  u8 cathodes_inv_mask;

  u8 dout_duocolour;

  u8 dout_l2;
  u8 dout_r2;

  u8 buttons_enabled;
  u8 buttons_no_ui;

  u8 din_l;
  u8 din_r;
} seq_hwcfg_srm_t;



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
extern seq_hwcfg_srm_t seq_hwcfg_srm;
extern seq_hwcfg_enc_t seq_hwcfg_enc;

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;

#endif /* _SEQ_HWCFG_H */
