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
  //          SR   ignore    Pin
  .down  = ((( 1   -1)<<3)+    0),
  .up    = ((( 1   -1)<<3)+    1),
  .left  = 0xff, // disabled
  .right = 0xff, // disabled

  //              SR   ignore    Pin
  .scrub     = ((( 1   -1)<<3)+    2),
  .metronome = ((( 1   -1)<<3)+    3),
  .record    = ((( 0   -1)<<3)+    0),
  .live      = ((( 0   -1)<<3)+    0),

  //          SR   ignore    Pin
  .stop  = ((( 1   -1)<<3)+    4),
  .pause = ((( 1   -1)<<3)+    5),
  .play  = ((( 1   -1)<<3)+    6),
  .rew   = ((( 1   -1)<<3)+    7),
  .fwd   = ((( 2   -1)<<3)+    0),

  //           SR   ignore    Pin
  .menu     = ((( 2   -1)<<3)+    5),
  .bookmark = ((( 0   -1)<<3)+    0),
  .select   = ((( 2   -1)<<3)+    6),
  .exit     = ((( 2   -1)<<3)+    7),

  //             SR   ignore    Pin
  .track[0] = ((( 3   -1)<<3)+    0),
  .track[1] = ((( 3   -1)<<3)+    1),
  .track[2] = ((( 3   -1)<<3)+    2),
  .track[3] = ((( 3   -1)<<3)+    3),

  //                     SR   ignore    Pin
  .direct_track[0]  = ((( 0   -1)<<3)+    0),
  .direct_track[1]  = ((( 0   -1)<<3)+    0),
  .direct_track[2]  = ((( 0   -1)<<3)+    0),
  .direct_track[3]  = ((( 0   -1)<<3)+    0),
  .direct_track[4]  = ((( 0   -1)<<3)+    0),
  .direct_track[5]  = ((( 0   -1)<<3)+    0),
  .direct_track[6]  = ((( 0   -1)<<3)+    0),
  .direct_track[7]  = ((( 0   -1)<<3)+    0),
  .direct_track[8]  = ((( 0   -1)<<3)+    0),
  .direct_track[9]  = ((( 0   -1)<<3)+    0),
  .direct_track[10] = ((( 0   -1)<<3)+    0),
  .direct_track[11] = ((( 0   -1)<<3)+    0),
  .direct_track[12] = ((( 0   -1)<<3)+    0),
  .direct_track[13] = ((( 0   -1)<<3)+    0),
  .direct_track[14] = ((( 0   -1)<<3)+    0),
  .direct_track[15] = ((( 0   -1)<<3)+    0),

  //                        SR   ignore    Pin
  .direct_bookmark[0]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[1]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[2]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[3]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[4]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[5]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[6]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[7]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[8]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[9]  = ((( 0   -1)<<3)+    0),
  .direct_bookmark[10] = ((( 0   -1)<<3)+    0),
  .direct_bookmark[11] = ((( 0   -1)<<3)+    0),
  .direct_bookmark[12] = ((( 0   -1)<<3)+    0),
  .direct_bookmark[13] = ((( 0   -1)<<3)+    0),
  .direct_bookmark[14] = ((( 0   -1)<<3)+    0),
  .direct_bookmark[15] = ((( 0   -1)<<3)+    0),

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
  .solo  = ((( 4   -1)<<3)+    4),
  .fast  = ((( 4   -1)<<3)+    5),
  .fast2 = ((( 0   -1)<<3)+    0),
  .all   = ((( 4   -1)<<3)+    6),

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

#ifdef MIOS32_FAMILY_EMULATION
  //                  SR   ignore    Pin
  .utility       = (((14   -1)<<3)+    1),
  .step_view     = (((13   -1)<<3)+    7),
  .par_layer_sel = ((( 2   -1)<<3)+    2),
  .trg_layer_sel = ((( 2   -1)<<3)+    3),
  .track_sel     = ((( 2   -1)<<3)+    4),


  //                 SR   ignore    Pin
  .tap_tempo    = ((( 0   -1)<<3)+    0),
  .tempo_preset = ((( 2   -1)<<3)+    1),
  .ext_restart  = ((( 0   -1)<<3)+    0),

  //            SR   ignore    Pin
  .copy    = (((14   -1)<<3)+    2),
  .paste   = (((14   -1)<<3)+    3),
  .clear   = (((14   -1)<<3)+    4),
  .undo    = ((( 0   -1)<<3)+    0),
#else
  //                  SR   ignore    Pin
  .utility       = ((( 2   -1)<<3)+    1),
  .step_view     = ((( 2   -1)<<3)+    2),
  .trg_layer_sel = ((( 2   -1)<<3)+    3),
  .track_sel     = ((( 2   -1)<<3)+    4),

  .par_layer_sel = ((( 0   -1)<<3)+    0),


  //                 SR   ignore    Pin
  .tap_tempo    = ((( 0   -1)<<3)+    0),
  .tempo_preset = ((( 0   -1)<<3)+    0),
  .ext_restart  = ((( 0   -1)<<3)+    0),

  //            SR   ignore    Pin
  .copy    = ((( 0   -1)<<3)+    0),
  .paste   = ((( 0   -1)<<3)+    0),
  .clear   = ((( 0   -1)<<3)+    0),
  .undo    = ((( 0   -1)<<3)+    0),
#endif

  //            SR   ignore    Pin
  .loop    = ((( 0   -1)<<3)+    0),
  .follow  = ((( 0   -1)<<3)+    0),

  //              SR   ignore    Pin
  .mixer     = ((( 0   -1)<<3)+    0),

  //              SR   ignore    Pin
  .save      = ((( 0   -1)<<3)+    0),
  .save_all  = ((( 0   -1)<<3)+    0),

  //                     SR   ignore    Pin
  .track_mode      = ((( 0   -1)<<3)+    0),
  .track_groove    = ((( 0   -1)<<3)+    0),
  .track_length    = ((( 0   -1)<<3)+    0),
  .track_direction = ((( 0   -1)<<3)+    0),
  .track_morph     = ((( 0   -1)<<3)+    0),
  .track_transpose = ((( 0   -1)<<3)+    0),
  .fx              = ((( 0   -1)<<3)+    0),

  //                                   SR   ignore    Pin
  .mute_all_tracks               = ((( 0   -1)<<3)+    0),
  .mute_track_layers             = ((( 0   -1)<<3)+    0),
  .mute_all_tracks_and_layers    = ((( 0   -1)<<3)+    0),
  .unmute_all_tracks             = ((( 0   -1)<<3)+    0),
  .unmute_track_layers           = ((( 0   -1)<<3)+    0),
  .unmute_all_tracks_and_layers  = ((( 0   -1)<<3)+    0),

  .footswitch      = ((( 0   -1)<<3)+    0),
  .pattern_remix   = ((( 0   -1)<<3)+    0),
};


seq_hwcfg_button_beh_t seq_hwcfg_button_beh = {
  .fast = 1,
  .fast2 = 0,
  .all = 1,
  .all_with_triggers = 0,
  .solo = 1,
  .metronome = 1,
  .loop = 1,
  .follow = 1,
  .bookmark = 1,
#if defined(MIOS32_FAMILY_EMULATION) && !defined(MIOS32_BOARD_IPAD)
  .scrub = 1,
  .menu = 1,
  .step_view = 1,
  .trg_layer = 1,
  .par_layer = 1,
  .track_sel = 1,
  .tempo_preset = 1,
#else
  .scrub = 0,
  .menu = 0,
  .step_view = 0,
  .trg_layer = 0,
  .par_layer = 0,
  .track_sel = 0,
  .tempo_preset = 0,
#endif
};


seq_hwcfg_led_t seq_hwcfg_led = {
  // GP LEDs DOUT shiftregister assignments
  .gp_dout_l_sr = 3,
  .gp_dout_r_sr = 4,

#ifdef MIOS32_FAMILY_EMULATION
  .gp_dout_l2_sr = 15,
  .gp_dout_r2_sr = 16,
#else
  .gp_dout_l2_sr = 0,
  .gp_dout_r2_sr = 0,
#endif

  .tracks_dout_l_sr = 0,
  .tracks_dout_r_sr = 0,

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

  //            SR    ignore    Pin
  .beat    = ((( 1   -1)<<3)+    7),
  .measure = ((( 0   -1)<<3)+    0),

  //                     SR    ignore    Pin
  .midi_in_combined =  ((( 0   -1)<<3)+    0),
  .midi_out_combined = ((( 0   -1)<<3)+    0),

  //            SR    ignore    Pin
  .edit    = ((( 2   -1)<<3)+    0),
  .mute    = ((( 2   -1)<<3)+    1),
  .pattern = ((( 2   -1)<<3)+    2),
  .song    = ((( 2   -1)<<3)+    3),

  //         SR    ignore    Pin
  .solo  = ((( 2   -1)<<3)+    4),
  .fast  = ((( 2   -1)<<3)+    5),
  .fast2 = ((( 0   -1)<<3)+    0),
  .all   = ((( 2   -1)<<3)+    6),

  //             SR    ignore    Pin
  .group[0] = (((11   -1)<<3)+    0),
  .group[1] = (((11   -1)<<3)+    2),
  .group[2] = (((11   -1)<<3)+    4),
  .group[3] = (((11   -1)<<3)+    6),

  //                 SR    ignore    Pin
  .trg_layer[0] = (((12   -1)<<3)+    0),
  .trg_layer[1] = (((12   -1)<<3)+    1),
  .trg_layer[2] = (((12   -1)<<3)+    2),

#ifdef MIOS32_FAMILY_EMULATION
  //          SR    ignore    Pin
  .play  = (((12   -1)<<3)+    3),
  .stop  = (((12   -1)<<3)+    4),
  .pause = (((12   -1)<<3)+    5),
  .rew   = (((12   -1)<<3)+    6),
  .fwd   = (((12   -1)<<3)+    7),
  .loop  = ((( 0   -1)<<3)+    0),
  .follow = ((( 0   -1)<<3)+    0),

  //              SR    ignore    Pin
  .exit      = ((( 0   -1)<<3)+    0),
  .select    = ((( 0   -1)<<3)+    0),
  .menu      = (((13   -1)<<3)+    0),
  .bookmark  = ((( 0   -1)<<3)+    0),
  .scrub     = (((13   -1)<<3)+    1),
  .metronome = (((13   -1)<<3)+    2),
  .utility   = (((13   -1)<<3)+    3),
  .copy      = (((13   -1)<<3)+    4),
  .paste     = (((13   -1)<<3)+    5),
  .clear     = (((13   -1)<<3)+    6),
  .undo      = ((( 0   -1)<<3)+    0),
  .record    = ((( 0   -1)<<3)+    0),
  .live      = ((( 0   -1)<<3)+    0),

  //               SR    ignore    Pin
  .step_view     = (((14   -1)<<3)+    5),
  .par_layer_sel = (((14   -1)<<3)+    1),
  .trg_layer_sel = (((14   -1)<<3)+    2),
  .track_sel     = (((14   -1)<<3)+    3),

  //                 SR    ignore    Pin
  .tap_tempo    = ((( 0   -1)<<3)+    0),
  .tempo_preset = (((14   -1)<<3)+    0),
  .ext_restart  = ((( 0   -1)<<3)+    0),

  //         SR    ignore    Pin
  .down = (((14   -1)<<3)+    6),
  .up   = (((14   -1)<<3)+    7),

  //         SR    ignore    Pin
  .mixer =     ((( 0   -1)<<3)+    0),

  //         SR    ignore    Pin
  .track_mode      = ((( 0   -1)<<3)+    0),
  .track_groove    = ((( 0   -1)<<3)+    0),
  .track_length    = ((( 0   -1)<<3)+    0),
  .track_direction = ((( 0   -1)<<3)+    0),
  .track_morph     = ((( 0   -1)<<3)+    0),
  .track_transpose = ((( 0   -1)<<3)+    0),
  .fx              = ((( 0   -1)<<3)+    0),
#else
  //          SR    ignore    Pin
  .play  = ((( 0   -1)<<3)+    0),
  .stop  = ((( 0   -1)<<3)+    0),
  .pause = ((( 0   -1)<<3)+    0),
  .rew   = ((( 0   -1)<<3)+    0),
  .fwd   = ((( 0   -1)<<3)+    0),
  .loop  = ((( 0   -1)<<3)+    0),
  .follow = ((( 0   -1)<<3)+    0),

  //              SR    ignore    Pin
  .menu      = ((( 0   -1)<<3)+    0),
  .bookmark  = ((( 0   -1)<<3)+    0),
  .scrub     = ((( 0   -1)<<3)+    0),
  .metronome = ((( 0   -1)<<3)+    0),
  .utility   = ((( 0   -1)<<3)+    0),
  .copy      = ((( 0   -1)<<3)+    0),
  .paste     = ((( 0   -1)<<3)+    0),
  .clear     = ((( 0   -1)<<3)+    0),
  .undo      = ((( 0   -1)<<3)+    0),

  //               SR    ignore    Pin
  .step_view     = ((( 0   -1)<<3)+    0),
  .par_layer_sel = ((( 0   -1)<<3)+    0),
  .trg_layer_sel = ((( 0   -1)<<3)+    0),
  .track_sel     = ((( 0   -1)<<3)+    0),

  //                 SR    ignore    Pin
  .tap_tempo    = ((( 0   -1)<<3)+    0),
  .tempo_preset = ((( 0   -1)<<3)+    0),
  .ext_restart  = ((( 0   -1)<<3)+    0),

  //         SR    ignore    Pin
  .down = ((( 0   -1)<<3)+    0),
  .up   = ((( 0   -1)<<3)+    0),

  //         SR    ignore    Pin
  .mixer =     ((( 0   -1)<<3)+    0),

  //         SR    ignore    Pin
  .track_mode      = ((( 0   -1)<<3)+    0),
  .track_groove    = ((( 0   -1)<<3)+    0),
  .track_length    = ((( 0   -1)<<3)+    0),
  .track_direction = ((( 0   -1)<<3)+    0),
  .track_morph     = ((( 0   -1)<<3)+    0),
  .track_transpose = ((( 0   -1)<<3)+    0),
  .fx              = ((( 0   -1)<<3)+    0),

  //                                   SR   ignore    Pin
  .mute_all_tracks               = ((( 0   -1)<<3)+    0),
  .mute_track_layers             = ((( 0   -1)<<3)+    0),
  .mute_all_tracks_and_layers    = ((( 0   -1)<<3)+    0),
  .unmute_all_tracks             = ((( 0   -1)<<3)+    0),
  .unmute_track_layers           = ((( 0   -1)<<3)+    0),
  .unmute_all_tracks_and_layers  = ((( 0   -1)<<3)+    0),
#endif
};


seq_hwcfg_enc_t seq_hwcfg_enc = {
  .datawheel_fast_speed = 3,
  .bpm_fast_speed = 3,
  .gp_fast_speed = 3,
  .auto_fast = 1,
};


seq_hwcfg_blm_t seq_hwcfg_blm = {
  .enabled = 0,
  .dout_duocolour = 1,
  .buttons_enabled = 1,
  .buttons_no_ui = 1,
  .gp_always_select_menu_page = 0,
};

seq_hwcfg_blm8x8_t seq_hwcfg_blm8x8 = {
  .enabled = 0,
  .dout_gp_mapping = 1,
  .din_gp_mapping = 0,
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
  .columns_sr = {0, 0},
  .rows_sr_green = {0, 0},
  .rows_sr_red = {0, 0},
};


seq_hwcfg_midi_remote_t seq_hwcfg_midi_remote = {
  .key = 96, // C-7, on some MIDI monitors displayed as C-6
  .cc = 0, // disabled
};

seq_hwcfg_track_cc_t seq_hwcfg_track_cc = {
  .mode = 0,
  .port = USB1,
  .chn = 0,
  .cc = 100,
};


u8 seq_hwcfg_dout_gate_sr[SEQ_HWCFG_NUM_SR_DOUT_GATES];
u8 seq_hwcfg_dout_gate_1ms = 0;
u8 seq_hwcfg_cv_gate_sr[SEQ_HWCFG_NUM_SR_CV_GATES];
u8 seq_hwcfg_clk_sr = 0;
u8 seq_hwcfg_j5_enabled = 0;


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

  { .cfg.type=DETENTED2, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr= 0, .cfg.pos=0 }, // BPM
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HWCFG_Init(u32 mode)
{
  int i;

  // using 16 SRs by default, can be increased to up to 23 SRs
  MIOS32_SRIO_ScanNumSet(16);

  // initialize encoders
  for(i=0; i<SEQ_HWCFG_NUM_ENCODERS; ++i)
    MIOS32_ENC_ConfigSet(i, enc_config[i]);

  // disable gate SRs
  for(i=0; i<SEQ_HWCFG_NUM_SR_DOUT_GATES; ++i)
    seq_hwcfg_dout_gate_sr[i] = 0;
  for(i=0; i<SEQ_HWCFG_NUM_SR_CV_GATES; ++i)
    seq_hwcfg_cv_gate_sr[i] = 0;

  // initial debounce delay for BLM_X
  blm_x_config_t config = BLM_X_ConfigGet();
  config.debounce_delay = 20; // mS
  BLM_X_ConfigSet(config);

  return 0; // no error
}
