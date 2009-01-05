// $Id$
/*
 * CC layer
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

#include <mios32.h>
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// global variables
/////////////////////////////////////////////////////////////////////////////
seq_cc_trk_t seq_cc_trk[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CC_Init(u32 mode)
{
  // initialize all CC parameters
  int track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_cc_trk_t *tcc = &seq_cc_trk[track];

    switch( track%4 ) {
      case 0: tcc->shared.chain = 0; break;
      case 1: tcc->shared.morph_pattern = 0; break;
      case 2: tcc->shared.scale = 0; break;
      case 3: tcc->shared.scale_root = 0; break;
    }

    tcc->mode.playmode = SEQ_CORE_TRKMODE_Normal;
    tcc->mode.UNSORTED = 0;
    tcc->mode.HOLD = 1;
    tcc->mode.RESTART = 0;
    tcc->mode.FORCE_SCALE = 0;
    tcc->mode.SUSTAIN = 0;

    tcc->evnt_mode = 0;

    tcc->evnt_const1 = 0;
    tcc->evnt_const2 = 0;
    tcc->evnt_const3 = 0;

    tcc->midi_chn = track % 16;
#if 0
    tcc->midi_port = UART0;
#else
    tcc->midi_port = DEFAULT;
#endif

    tcc->dir_mode = SEQ_CORE_TRKDIR_Forward;
    tcc->steps_replay = 0;
    tcc->steps_forward = 0;
    tcc->steps_jump_back = 0;

    tcc->clkdiv.value = 3;
    tcc->clkdiv.SYNCH_TO_MEASURE = 0;
    tcc->clkdiv.TRIPLETS = 0;

    tcc->length = 15;
    tcc->loop = 0;

    tcc->transpose_semi = 0;
    tcc->transpose_oct = 0;

    tcc->groove_style = 0;
    tcc->groove_value = 0;

    tcc->morph_mode = 0;
    tcc->morph_spare = 0;

    tcc->trg_assignments.gate = 1;
    tcc->trg_assignments.skip = 0;
    tcc->trg_assignments.accent = 2;
    tcc->trg_assignments.glide = 0;
    tcc->trg_assignments.roll = 3;
    tcc->trg_assignments.random_gate = 0;
    tcc->trg_assignments.random_value = 0;
    tcc->trg_assignments.spare = 0;

    tcc->humanize_mode = 0;
    tcc->humanize_value = 0;

    tcc->echo_repeats = 0;
    tcc->echo_delay = 7; // 1/8
    tcc->echo_velocity = 15; // 75%
    tcc->echo_fb_velocity = 15; // 75%
    tcc->echo_fb_note = 24; // +0
    tcc->echo_fb_gatelength = 20; // 100%
    tcc->echo_fb_ticks = 20; // 100%
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Set CCs
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CC_Set(u8 track, u8 cc, u8 value)
{
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // invalid track

  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // since CCs can be modified from other tasks at different priority we should do this operation atomic
  MIOS32_IRQ_Disable();

  switch( cc ) {
    case SEQ_CC_MODE: tcc->mode.playmode = value; break;
    case SEQ_CC_MODE_FLAGS: tcc->mode.flags = value; break;

    case SEQ_CC_MIDI_EVNT_MODE: tcc->evnt_mode = value; break;
    case SEQ_CC_MIDI_EVNT_CONST1: tcc->evnt_const1 = value; break;
    case SEQ_CC_MIDI_EVNT_CONST2: tcc->evnt_const2 = value; break;
    case SEQ_CC_MIDI_EVNT_CONST3: tcc->evnt_const3 = value; break;
    case SEQ_CC_MIDI_CHANNEL: tcc->midi_chn = value; break;
    case SEQ_CC_MIDI_PORT: tcc->midi_port = value; break;
  
    case SEQ_CC_DIRECTION: tcc->dir_mode = value; break;
    case SEQ_CC_STEPS_REPLAY: tcc->steps_replay = value; break;
    case SEQ_CC_STEPS_FORWARD: tcc->steps_forward = value; break;
    case SEQ_CC_STEPS_JMPBCK: tcc->steps_jump_back = value; break;
    case SEQ_CC_CLK_DIVIDER: tcc->clkdiv.value = value; break;
    case SEQ_CC_LENGTH: tcc->length = value; break;
    case SEQ_CC_LOOP: tcc->loop = value; break;
    case SEQ_CC_CLKDIV_FLAGS: tcc->clkdiv.flags = value; break;
  
    case SEQ_CC_TRANSPOSE_SEMI: tcc->transpose_semi = value; break;
    case SEQ_CC_TRANSPOSE_OCT: tcc->transpose_oct = value; break;
    case SEQ_CC_GROOVE_VALUE: tcc->groove_value = value; break;
    case SEQ_CC_GROOVE_STYLE: tcc->groove_style = value; break;
    case SEQ_CC_MORPH_MODE: tcc->morph_mode = value; break;
    case SEQ_CC_MORPH_SPARE: tcc->morph_spare = value; break;
    case SEQ_CC_HUMANIZE_VALUE: tcc->humanize_value = value; break;
    case SEQ_CC_HUMANIZE_MODE: tcc->humanize_mode = value; break;
  
    case SEQ_CC_ASG_GATE: tcc->trg_assignments.gate = value; break;
    case SEQ_CC_ASG_SKIP: tcc->trg_assignments.skip = value; break;
    case SEQ_CC_ASG_ACCENT: tcc->trg_assignments.accent = value; break;
    case SEQ_CC_ASG_GLIDE: tcc->trg_assignments.glide = value; break;
    case SEQ_CC_ASG_ROLL: tcc->trg_assignments.roll = value; break;
    case SEQ_CC_ASG_RANDOM_GATE: tcc->trg_assignments.random_gate = value; break;
    case SEQ_CC_ASG_RANDOM_VALUE: tcc->trg_assignments.random_value = value; break;
    case SEQ_CC_ASG_RANDOM_SPARE: tcc->trg_assignments.spare = value; break;
  
    case SEQ_CC_CHANGE_STEP: break; // TODO
  
    case SEQ_CC_ECHO_REPEATS: tcc->echo_repeats = value; break;
    case SEQ_CC_ECHO_DELAY: tcc->echo_delay = value; break;
    case SEQ_CC_ECHO_VELOCITY: tcc->echo_velocity = value; break;
    case SEQ_CC_ECHO_FB_VELOCITY: tcc->echo_fb_velocity = value; break;
    case SEQ_CC_ECHO_FB_NOTE: tcc->echo_fb_note = value; break;
    case SEQ_CC_ECHO_FB_GATELENGTH: tcc->echo_fb_gatelength = value; break;
    case SEQ_CC_ECHO_FB_TICKS: tcc->echo_fb_ticks = value; break;

    default:
      MIOS32_IRQ_Enable();
      return -2; // invalid CC
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get CCs
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CC_Get(u8 track, u8 cc)
{
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // invalid track

  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  switch( cc ) {
    case SEQ_CC_MODE: return tcc->mode.playmode;
    case SEQ_CC_MODE_FLAGS: return tcc->mode.flags;
    case SEQ_CC_MIDI_EVNT_MODE: return tcc->evnt_mode;
    case SEQ_CC_MIDI_EVNT_CONST1: return tcc->evnt_const1;
    case SEQ_CC_MIDI_EVNT_CONST2: return tcc->evnt_const2;
    case SEQ_CC_MIDI_EVNT_CONST3: return tcc->evnt_const3;
    case SEQ_CC_MIDI_CHANNEL: return tcc->midi_chn;
    case SEQ_CC_MIDI_PORT: return tcc->midi_port;
  
    case SEQ_CC_DIRECTION: return tcc->dir_mode;
    case SEQ_CC_STEPS_REPLAY: return tcc->steps_replay;
    case SEQ_CC_STEPS_FORWARD: return tcc->steps_forward;
    case SEQ_CC_STEPS_JMPBCK: return tcc->steps_jump_back;
    case SEQ_CC_CLK_DIVIDER: return tcc->clkdiv.value;
    case SEQ_CC_LENGTH: return tcc->length;
    case SEQ_CC_LOOP: return tcc->loop;
    case SEQ_CC_CLKDIV_FLAGS: return tcc->clkdiv.flags;
  
    case SEQ_CC_TRANSPOSE_SEMI: return tcc->transpose_semi;
    case SEQ_CC_TRANSPOSE_OCT: return tcc->transpose_oct;
    case SEQ_CC_GROOVE_VALUE: return tcc->groove_value;
    case SEQ_CC_GROOVE_STYLE: return tcc->groove_style;
    case SEQ_CC_MORPH_MODE: return tcc->morph_mode;
    case SEQ_CC_MORPH_SPARE: return tcc->morph_spare;
    case SEQ_CC_HUMANIZE_VALUE: return tcc->humanize_value;
    case SEQ_CC_HUMANIZE_MODE: return tcc->humanize_mode;
  
    case SEQ_CC_ASG_GATE: return tcc->trg_assignments.gate;
    case SEQ_CC_ASG_SKIP: return tcc->trg_assignments.skip;
    case SEQ_CC_ASG_ACCENT: return tcc->trg_assignments.accent;
    case SEQ_CC_ASG_GLIDE: return tcc->trg_assignments.glide;
    case SEQ_CC_ASG_ROLL: return tcc->trg_assignments.roll;
    case SEQ_CC_ASG_RANDOM_GATE: return tcc->trg_assignments.random_gate;
    case SEQ_CC_ASG_RANDOM_VALUE: return tcc->trg_assignments.random_value;
    case SEQ_CC_ASG_RANDOM_SPARE: return tcc->trg_assignments.spare;
  
    case SEQ_CC_CHANGE_STEP: return 0; // TODO

    case SEQ_CC_ECHO_REPEATS: return tcc->echo_repeats; break;
    case SEQ_CC_ECHO_DELAY: return tcc->echo_delay; break;
    case SEQ_CC_ECHO_VELOCITY: return tcc->echo_velocity; break;
    case SEQ_CC_ECHO_FB_VELOCITY: return tcc->echo_fb_velocity; break;
    case SEQ_CC_ECHO_FB_NOTE: return tcc->echo_fb_note; break;
    case SEQ_CC_ECHO_FB_GATELENGTH: return tcc->echo_fb_gatelength; break;
    case SEQ_CC_ECHO_FB_TICKS: return tcc->echo_fb_ticks; break;
  }

  return -2; // invalid CC
}
