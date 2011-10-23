// $Id$
/*
 * Hardware Soft-Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

// with this switch, seq_ui.h/seq_ui_pages.inc will create local variables
#define SEQ_UI_PAGES_INC_LOCAL_VARS 1

#include <mios32.h>

#include <blm_x.h>
#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// following predefenitions are matching with the standard layout (without Button/LED Matrix)
// They can be overwritten in the MBSEQ_HW.V4 file, which is loaded from SD Card during startup

seq_hwcfg_button_t seq_hwcfg_button = {
  .gp_din_l_sr = 3,
  .gp_din_r_sr = 7,

  //                SR   ignore    Pin
  .bar1        = ((( 1   -1)<<3)+    0),
  .bar2        = ((( 1   -1)<<3)+    1),
  .bar3        = ((( 1   -1)<<3)+    2),
  .bar4        = ((( 1   -1)<<3)+    3),

  .seq1        = ((( 1   -1)<<3)+    4),
  .seq2        = ((( 1   -1)<<3)+    5),

  .load        = ((( 1   -1)<<3)+    6),
  .save        = ((( 1   -1)<<3)+    7),

  .copy        = ((( 5   -1)<<3)+    0),
  .paste       = ((( 5   -1)<<3)+    1),
  .clear       = ((( 5   -1)<<3)+    2),
  .undo        = ((( 5   -1)<<3)+    3),

  .master      = ((( 5   -1)<<3)+    4),
  .tap_tempo   = ((( 5   -1)<<3)+    5),
  .stop        = ((( 5   -1)<<3)+    6),
  .play        = ((( 5   -1)<<3)+    7),

  .pause       = ((( 0   -1)<<3)+    0),
  .metronome   = ((( 0   -1)<<3)+    0),
  .ext_restart = ((( 0   -1)<<3)+    0),

  .trigger     = ((( 4   -1)<<3)+    0),
  .length      = ((( 4   -1)<<3)+    1),
  .progression = ((( 4   -1)<<3)+    2),
  .groove      = ((( 4   -1)<<3)+    3),
  .echo        = ((( 4   -1)<<3)+    4),
  .humanizer   = ((( 4   -1)<<3)+    5),
  .lfo         = ((( 4   -1)<<3)+    6),
  .scale       = ((( 4   -1)<<3)+    7),

  .mute        = ((( 8   -1)<<3)+    0),
  .midichn     = ((( 8   -1)<<3)+    1),

  .rec_arm     = ((( 8   -1)<<3)+    2),
  .rec_step    = ((( 8   -1)<<3)+    3),
  .rec_live    = ((( 8   -1)<<3)+    4),
  .rec_poly    = ((( 8   -1)<<3)+    5),
  .inout_fwd   = ((( 8   -1)<<3)+    6),
  .transpose   = ((( 8   -1)<<3)+    7),
};

seq_hwcfg_led_t seq_hwcfg_led = {
  .gp_dout_l_sr = 3,
  .gp_dout_r_sr = 7,

  .pos_dout_l_sr = 2,
  .pos_dout_r_sr = 6,

  //                SR   ignore    Pin
  .bar1        = ((( 1   -1)<<3)+    0),
  .bar2        = ((( 1   -1)<<3)+    1),
  .bar3        = ((( 1   -1)<<3)+    2),
  .bar4        = ((( 1   -1)<<3)+    3),

  .seq1        = ((( 1   -1)<<3)+    4),
  .seq2        = ((( 1   -1)<<3)+    5),
  .load        = ((( 1   -1)<<3)+    6),
  .save        = ((( 1   -1)<<3)+    7),

  .copy        = ((( 5   -1)<<3)+    0),
  .paste       = ((( 5   -1)<<3)+    1),
  .clear       = ((( 5   -1)<<3)+    2),
  .undo        = ((( 5   -1)<<3)+    3),

  .master      = ((( 5   -1)<<3)+    4),
  .tap_tempo   = ((( 5   -1)<<3)+    5),
  .stop        = ((( 5   -1)<<3)+    6),
  .play        = ((( 5   -1)<<3)+    7),

  .pause       = ((( 0   -1)<<3)+    0),
  .metronome   = ((( 0   -1)<<3)+    0),
  .ext_restart = ((( 0   -1)<<3)+    0),

  .trigger     = ((( 4   -1)<<3)+    0),
  .length      = ((( 4   -1)<<3)+    1),
  .progression = ((( 4   -1)<<3)+    2),
  .groove      = ((( 4   -1)<<3)+    3),
  .echo        = ((( 4   -1)<<3)+    4),
  .humanizer   = ((( 4   -1)<<3)+    5),
  .lfo         = ((( 4   -1)<<3)+    6),
  .scale       = ((( 4   -1)<<3)+    7),

  .mute        = ((( 8   -1)<<3)+    0),
  .midichn     = ((( 8   -1)<<3)+    1),

  .rec_arm     = ((( 8   -1)<<3)+    2),
  .rec_step    = ((( 8   -1)<<3)+    3),
  .rec_live    = ((( 8   -1)<<3)+    4),
  .rec_poly    = ((( 8   -1)<<3)+    5),
  .inout_fwd   = ((( 8   -1)<<3)+    6),
  .transpose   = ((( 8   -1)<<3)+    7),
};


seq_hwcfg_enc_t seq_hwcfg_enc = {
  .bpm_fast_speed = 3,
};


seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8 = {
  .enabled = 0,
  .dout_gp_mapping = 1,
};


seq_hwcfg_bpm_digits_t seq_hwcfg_bpm_digits = {
  .enabled = 0,
  .segments_sr = 0,
  .common1_pin = ((( 0   -1)<<3)+    0),
  .common2_pin = ((( 0   -1)<<3)+    0),
  .common3_pin = ((( 0   -1)<<3)+    0),
  .common4_pin = ((( 0   -1)<<3)+    0),
};


seq_hwcfg_step_digits_t seq_hwcfg_step_digits = {
  .enabled = 0,
  .segments_sr = 0,
  .common1_pin = ((( 0   -1)<<3)+    0),
  .common2_pin = ((( 0   -1)<<3)+    0),
  .common3_pin = ((( 0   -1)<<3)+    0),
};

seq_hwcfg_tpd_t seq_hwcfg_tpd = {
  .enabled = 0,
  .columns_sr = 0,
  .rows_sr = 0,
};

seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote = {
  .key = 96, // C-7, on some MIDI monitors displayed as C-6
  .cc = 0, // disabled
};


/////////////////////////////////////////////////////////////////////////////
// Local constant arrays
/////////////////////////////////////////////////////////////////////////////

static const mios32_enc_config_t enc_config[SEQ_HWCFG_NUM_ENCODERS] = {
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 0, .cfg.pos=0 }, // BPM
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HWCFG_Init(u32 mode)
{
  int i;

  // initialize encoders
  for(i=0; i<SEQ_HWCFG_NUM_ENCODERS; ++i)
    MIOS32_ENC_ConfigSet(i, enc_config[i]);

  // initial debounce delay for BLM_X
  blm_x_config_t config = BLM_X_ConfigGet();
  config.debounce_delay = 20; // mS
  BLM_X_ConfigSet(config);

  return 0; // no error
}
