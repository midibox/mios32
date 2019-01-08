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

#define SEQ_HWCFG_NUM_ENCODERS     1


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 gp_din_l_sr;
  u16 gp_din_r_sr;

  u16 bar1;
  u16 bar2;
  u16 bar3;
  u16 bar4;

  u16 seq1;
  u16 seq2;

  u16 load;
  u16 save;

  u16 copy;
  u16 paste;
  u16 clear;
  u16 undo;

  u16 master;
  u16 tap_tempo;
  u16 stop;
  u16 play;

  u16 pause;
  u16 metronome;
  u16 ext_restart;

  u16 trigger;
  u16 length;
  u16 progression;
  u16 groove;
  u16 echo;
  u16 humanizer;
  u16 lfo;
  u16 scale;

  u16 mute;
  u16 midichn;

  u16 rec_arm;
  u16 rec_step;
  u16 rec_live;
  u16 rec_poly;
  u16 inout_fwd;
  u16 transpose;
} seq_hwcfg_button_t;


typedef struct {
  u8  gp_dout_l_sr;
  u8  gp_dout_r_sr;

  u8  pos_dout_l_sr;
  u8  pos_dout_r_sr;

  u16 bar1;
  u16 bar2;
  u16 bar3;
  u16 bar4;

  u16 seq1;
  u16 seq2;

  u16 load;
  u16 save;

  u16 copy;
  u16 paste;
  u16 clear;
  u16 undo;

  u16 master;
  u16 tap_tempo;
  u16 stop;
  u16 play;

  u16 pause;
  u16 metronome;
  u16 ext_restart;

  u16 trigger;
  u16 length;
  u16 progression;
  u16 groove;
  u16 echo;
  u16 humanizer;
  u16 lfo;
  u16 scale;

  u16 mute;
  u16 midichn;

  u16 rec_arm;
  u16 rec_step;
  u16 rec_live;
  u16 rec_poly;
  u16 inout_fwd;
  u16 transpose;
} seq_hwcfg_led_t;


typedef struct {
  u8 bpm_fast_speed;
} seq_hwcfg_enc_t;

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
  u8  enabled;
  u8  segments_sr;
  u16 common1_pin;
  u16 common2_pin;
  u16 common3_pin;
} seq_hwcfg_step_digits_t;

typedef struct {
  u8 enabled;
  u8 columns_sr[2];
  u8 rows_sr_green[2];
  u8 rows_sr_red[2];
} seq_hwcfg_tpd_t;

// following constants can be safely changed (therefore documented)

// max. number of SRs which can be used for CV gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_CV_GATES 1

// max. number of SRs which can be used for triggering gates (each SR provides 8 gates)
#define SEQ_HWCFG_NUM_SR_DOUT_GATES 8


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_HWCFG_Init(u32 mode);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_hwcfg_button_t seq_hwcfg_button;
extern seq_hwcfg_led_t seq_hwcfg_led;
extern seq_hwcfg_enc_t seq_hwcfg_enc;
extern seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8;
extern seq_hwcfg_step_digits_t seq_hwcfg_step_digits;
extern seq_hwcfg_tpd_t seq_hwcfg_tpd;
extern seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits;

extern u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
extern u8 seq_hwcfg_dout_gate_1ms;
extern u8 seq_hwcfg_cv_gate_sr[SEQ_HWCFG_NUM_SR_CV_GATES];
extern u8 seq_hwcfg_clk_sr;
extern u8 seq_hwcfg_j5_enabled;

#endif /* _SEQ_HWCFG_H */
