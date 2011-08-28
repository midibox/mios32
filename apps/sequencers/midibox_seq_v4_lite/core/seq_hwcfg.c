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

seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote = {
  .key = 96, // C-7, on some MIDI monitors displayed as C-6
  .cc = 0, // disabled
};



/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HWCFG_Init(u32 mode)
{
  return 0; // no error
}
