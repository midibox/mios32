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
  //          SR   ignore    Pin
  .down  = ((( 1   -1)<<3)+    0),
  .up    = ((( 1   -1)<<3)+    1),
  .left  = 0xff, // disabled
  .right = 0xff, // disabled

  //              SR   ignore    Pin
  .scrub     = ((( 1   -1)<<3)+    2),
  .metronome = ((( 1   -1)<<3)+    3),

  //          SR   ignore    Pin
  .stop  = ((( 1   -1)<<3)+    4),
  .pause = ((( 1   -1)<<3)+    5),
  .play  = ((( 1   -1)<<3)+    6),
  .rew   = ((( 1   -1)<<3)+    7),
  .fwd   = ((( 2   -1)<<3)+    0),

  //       SR   ignore    Pin
  .f1 = ((( 2   -1)<<3)+    1),
  .f2 = ((( 2   -1)<<3)+    2),
  .f3 = ((( 2   -1)<<3)+    3),
  .f4 = ((( 2   -1)<<3)+    4),

  //           SR   ignore    Pin
  .menu   = ((( 2   -1)<<3)+    5),
  .select = ((( 2   -1)<<3)+    6),
  .exit   = ((( 2   -1)<<3)+    7),

  //             SR   ignore    Pin
  .track[0] = ((( 3   -1)<<3)+    0),
  .track[1] = ((( 3   -1)<<3)+    1),
  .track[2] = ((( 3   -1)<<3)+    2),
  .track[3] = ((( 3   -1)<<3)+    3),

  //                 SR   ignore    Pin
  .par_layer[0] = ((( 3   -1)<<3)+    4),
  .par_layer[1] = ((( 3   -1)<<3)+    5),
  .par_layer[2] = ((( 3   -1)<<3)+    6),

  //            SR   ignore    Pin
  .edit    = ((( 4  -1)<<3)+    0),
  .mute    = ((( 4  -1)<<3)+    1),
  .pattern = ((( 4  -1)<<3)+    2),
  .song    = ((( 4  -1)<<3)+    3),

  //         SR   ignore    Pin
  .solo = ((( 4   -1)<<3)+    4),
  .fast = ((( 4   -1)<<3)+    5),
  .all  = ((( 4   -1)<<3)+    6),

  //           SR   ignore    Pin
  .gp[ 0] = ((( 7   -1)<<3)+    0),
  .gp[ 1] = ((( 7   -1)<<3)+    1),
  .gp[ 2] = ((( 7   -1)<<3)+    2),
  .gp[ 3] = ((( 7   -1)<<3)+    3),
  .gp[ 4] = ((( 7   -1)<<3)+    4),
  .gp[ 5] = ((( 7   -1)<<3)+    5),
  .gp[ 6] = ((( 7   -1)<<3)+    6),
  .gp[ 7] = ((( 7   -1)<<3)+    7),
  .gp[ 8] = (((10   -1)<<3)+    0),
  .gp[ 9] = (((10   -1)<<3)+    1),
  .gp[10] = (((10   -1)<<3)+    2),
  .gp[11] = (((10   -1)<<3)+    3),
  .gp[12] = (((10   -1)<<3)+    4),
  .gp[13] = (((10   -1)<<3)+    5),
  .gp[14] = (((10   -1)<<3)+    6),
  .gp[15] = (((10   -1)<<3)+    7),

  //             SR   ignore    Pin
  .group[0] = (((13   -1)<<3)+    0),
  .group[1] = (((13   -1)<<3)+    1),
  .group[2] = (((13   -1)<<3)+    2),
  .group[3] = (((13   -1)<<3)+    3),

  //                 SR   ignore    Pin
  .trg_layer[0] = (((13   -1)<<3)+    4),
  .trg_layer[1] = (((13   -1)<<3)+    5),
  .trg_layer[2] = (((13   -1)<<3)+    6),

  //              SR   ignore    Pin
  .step_view = (((13   -1)<<3)+    7),
  .tap_tempo = (((14   -1)<<3)+    0),

  //            SR   ignore    Pin
  .utility = (((14   -1)<<3)+    1),
  .copy    = (((14   -1)<<3)+    2),
  .paste   = (((14   -1)<<3)+    3),
  .clear   = (((14   -1)<<3)+    4),
};


seq_hwcfg_led_t seq_hwcfg_led = {
  // GP LEDs DOUT shiftregister assignments
  .gp_dout_sr_l = 3,
  .gp_dout_sr_r = 4,
  .gp_dout_sr_l2 = 15,
  .gp_dout_sr_r2 = 16,

  // DOUT pin assignments

  //             SR    ignore    Pin
  .track[0] = ((( 1   -1)<<3)+    0),
  .track[1] = ((( 1   -1)<<3)+    1),
  .track[2] = ((( 1   -1)<<3)+    2),
  .track[3] = ((( 1   -1)<<3)+    3),

  //                 SR    ignore    Pin
  .par_layer[0] = ((( 1   -1)<<3)+    4),
  .par_layer[1] = ((( 1   -1)<<3)+    5),
  .par_layer[2] = ((( 1   -1)<<3)+    6),

  //         SR    ignore    Pin
  .beat = ((( 1   -1)<<3)+    7),

  //            SR    ignore    Pin
  .edit    = ((( 2   -1)<<3)+    0),
  .mute    = ((( 2   -1)<<3)+    1),
  .pattern = ((( 2   -1)<<3)+    2),
  .song    = ((( 2   -1)<<3)+    3),

  //         SR    ignore    Pin
  .solo = ((( 2   -1)<<3)+    4),
  .fast = ((( 2   -1)<<3)+    5),
  .all  = ((( 2   -1)<<3)+    6),

  //             SR    ignore    Pin
  .group[0] = (((11   -1)<<3)+    0),
  .group[1] = (((11   -1)<<3)+    2),
  .group[2] = (((11   -1)<<3)+    4),
  .group[3] = (((11   -1)<<3)+    6),

  //                 SR    ignore    Pin
  .trg_layer[0] = (((12   -1)<<3)+    0),
  .trg_layer[1] = (((12   -1)<<3)+    1),
  .trg_layer[2] = (((12   -1)<<3)+    2),

  //          SR    ignore    Pin
  .play  = (((12   -1)<<3)+    3),
  .stop  = (((12   -1)<<3)+    4),
  .pause = (((12   -1)<<3)+    5),
  .rew   = (((12   -1)<<3)+    6),
  .fwd   = (((12   -1)<<3)+    7),

  //              SR    ignore    Pin
  .menu      = (((13   -1)<<3)+    0),
  .scrub     = (((13   -1)<<3)+    1),
  .metronome = (((13   -1)<<3)+    2),
  .utility   = (((13   -1)<<3)+    3),
  .copy      = (((13   -1)<<3)+    4),
  .paste     = (((13   -1)<<3)+    5),
  .clear     = (((13   -1)<<3)+    6),

  //       SR    ignore    Pin
  .f1 = (((14   -1)<<3)+    0),
  .f2 = (((14   -1)<<3)+    1),
  .f3 = (((14   -1)<<3)+    2),
  .f4 = (((14   -1)<<3)+    3),

  //               SR    ignore    Pin
  .step_view  = (((14   -1)<<3)+    5),

  //         SR    ignore    Pin
  .down = (((14   -1)<<3)+    6),
  .up   = (((14   -1)<<3)+    7),
};


seq_hwcfg_srm_t seq_hwcfg_srm = {
  .enabled = 0,
  .dout_l1 = 6,
  .dout_r1 = 9,
  .dout_m = 0,

  .dout_cathodes1 = 5,
  .dout_cathodes2 = 8,
  .dout_cathodesm = 0,

  .dout_m_mapping = 1,

  .cathodes_inv_mask = 0x00,
  .cathodes_inv_mask_m = 0x00,

  .dout_duocolour = 1,

  .dout_l2 = 7,
  .dout_r2 = 10,

  .buttons_enabled = 0,
  .buttons_no_ui = 1,

  .din_l = 11,
  .din_r = 12,
  .din_m = 0,
};


u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
u8 seq_hwcfg_dout_gate_1ms;


/////////////////////////////////////////////////////////////////////////////
// Local constant arrays
/////////////////////////////////////////////////////////////////////////////

static const mios32_enc_config_t enc_config[SEQ_HWCFG_NUM_ENCODERS] = {
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 1, .cfg.pos=0 }, // Data Wheel

  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=0 }, // GP1
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=2 }, // GP2
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=4 }, // GP3
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 5, .cfg.pos=6 }, // GP4
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 6, .cfg.pos=0 }, // GP5
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 6, .cfg.pos=2 }, // GP6
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 6, .cfg.pos=4 }, // GP7
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 6, .cfg.pos=6 }, // GP8
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 8, .cfg.pos=0 }, // GP9
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 8, .cfg.pos=2 }, // GP10
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 8, .cfg.pos=4 }, // GP11
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 8, .cfg.pos=6 }, // GP12
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 9, .cfg.pos=0 }, // GP13
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 9, .cfg.pos=2 }, // GP14
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 9, .cfg.pos=4 }, // GP15
  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 9, .cfg.pos=6 }, // GP16
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

  // disable gate SRs
  for(i=0; i<SEQ_HWCFG_NUM_SR_DOUT_GATES; ++i)
    seq_hwcfg_dout_gate_sr[i] = 0;
  seq_hwcfg_dout_gate_1ms = 0;

  return 0; // no error
}
