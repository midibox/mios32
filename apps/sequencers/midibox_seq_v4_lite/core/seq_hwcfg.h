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


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 gp_din_l_sr;
  u8 gp_din_r_sr;

  u8 bar1;
  u8 bar2;
  u8 bar3;
  u8 bar4;

  u8 seq1;
  u8 seq2;

  u8 load;
  u8 save;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 master;
  u8 tap_tempo;
  u8 stop;
  u8 play;

  u8 pause;
  u8 metronome;
  u8 ext_restart;

  u8 trigger;
  u8 length;
  u8 progression;
  u8 groove;
  u8 echo;
  u8 humanizer;
  u8 lfo;
  u8 scale;

  u8 mute;
  u8 midichn;

  u8 rec_arm;
  u8 rec_step;
  u8 rec_live;
  u8 rec_poly;
  u8 inout_fwd;
  u8 transpose;
} seq_hwcfg_button_t;


typedef struct {
  u8 gp_dout_l_sr;
  u8 gp_dout_r_sr;

  u8 pos_dout_l_sr;
  u8 pos_dout_r_sr;

  u8 bar1;
  u8 bar2;
  u8 bar3;
  u8 bar4;

  u8 seq1;
  u8 seq2;

  u8 load;
  u8 save;

  u8 copy;
  u8 paste;
  u8 clear;
  u8 undo;

  u8 master;
  u8 tap_tempo;
  u8 stop;
  u8 play;

  u8 pause;
  u8 metronome;
  u8 ext_restart;

  u8 trigger;
  u8 length;
  u8 progression;
  u8 groove;
  u8 echo;
  u8 humanizer;
  u8 lfo;
  u8 scale;

  u8 mute;
  u8 midichn;

  u8 rec_arm;
  u8 rec_step;
  u8 rec_live;
  u8 rec_poly;
  u8 inout_fwd;
  u8 transpose;
} seq_hwcfg_led_t;


typedef struct {
  u8 enabled;
  u8 dout_gp_mapping;
} seq_hwcfg_blm8x8_t;

typedef struct {
  u8 enabled;
  u8 segments_sr;
  u8 common1_pin;
  u8 common2_pin;
  u8 common3_pin;
} seq_hwcfg_bpm_digits_t;

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
extern seq_hwcfg_led_t seq_hwcfg_led;
extern seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8;
extern seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote;
extern seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits;

#endif /* _SEQ_HWCFG_H */
