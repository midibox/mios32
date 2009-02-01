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
#include <string.h>
#include "tasks.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// global variables
/////////////////////////////////////////////////////////////////////////////
seq_cc_trk_t seq_cc_trk[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 CC_LinkUpdate(u8 track);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CC_Init(u32 mode)
{
  int i;

  // initialize all CC parameters
  int track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_cc_trk_t *tcc = &seq_cc_trk[track];

    // clear all CCs
    memset((u8 *)tcc, 0, sizeof(seq_cc_trk_t));

    // set parameters which are not changed by SEQ_LAYER_CopyPreset() function
    tcc->midi_chn = track % 16;
    tcc->midi_port = DEFAULT;
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
  portENTER_CRITICAL();

  if( cc < 0x40 ) {
    tcc->lay_const[cc] = value;
    if( tcc->event_mode != SEQ_EVENT_MODE_Drum )
      CC_LinkUpdate(track);
  } else {
    switch( cc ) {
      case SEQ_CC_MODE: tcc->mode.playmode = value; break;
      case SEQ_CC_MODE_FLAGS: tcc->mode.flags = value; break;
  
      case SEQ_CC_MIDI_EVENT_MODE: 
	tcc->event_mode = value; 
	CC_LinkUpdate(track);
	break;

      case SEQ_CC_MIDI_CHANNEL: tcc->midi_chn = value; break;
      case SEQ_CC_MIDI_PORT: tcc->midi_port = value; break;
    
      case SEQ_CC_DIRECTION: tcc->dir_mode = value; break;
      case SEQ_CC_STEPS_REPLAY: tcc->steps_replay = value; break;
      case SEQ_CC_STEPS_FORWARD: tcc->steps_forward = value; break;
      case SEQ_CC_STEPS_JMPBCK: tcc->steps_jump_back = value; break;
      case SEQ_CC_CLK_DIVIDER: tcc->clkdiv.value = value; break;

      case SEQ_CC_LENGTH:
	// ensure that step position pointer matches with the new track length
	portENTER_CRITICAL();
	tcc->length = value;
	seq_core_trk[track].step %= ((u16)value+1);
	portEXIT_CRITICAL();
	break;

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
      case SEQ_CC_ASG_ACCENT: tcc->trg_assignments.accent = value; break;
      case SEQ_CC_ASG_ROLL: tcc->trg_assignments.roll = value; break;
      case SEQ_CC_ASG_GLIDE: tcc->trg_assignments.glide = value; break;
      case SEQ_CC_ASG_SKIP: tcc->trg_assignments.skip = value; break;
      case SEQ_CC_ASG_RANDOM_GATE: tcc->trg_assignments.random_gate = value; break;
      case SEQ_CC_ASG_RANDOM_VALUE: tcc->trg_assignments.random_value = value; break;
      case SEQ_CC_ASG_NO_FX: tcc->trg_assignments.no_fx = value; break;

      case SEQ_CC_PAR_ASG_DRUM_LAYER_A:
	tcc->par_assignment_drum[0] = value;
	CC_LinkUpdate(track);
	break;
      case SEQ_CC_PAR_ASG_DRUM_LAYER_B:
	tcc->par_assignment_drum[1] = value;
	CC_LinkUpdate(track);
	break;

#if 0  
      case SEQ_CC_CHANGE_STEP: break; // TODO
#endif
    
      case SEQ_CC_ECHO_REPEATS: tcc->echo_repeats = value; break;
      case SEQ_CC_ECHO_DELAY: tcc->echo_delay = value; break;
      case SEQ_CC_ECHO_VELOCITY: tcc->echo_velocity = value; break;
      case SEQ_CC_ECHO_FB_VELOCITY: tcc->echo_fb_velocity = value; break;
      case SEQ_CC_ECHO_FB_NOTE: tcc->echo_fb_note = value; break;
      case SEQ_CC_ECHO_FB_GATELENGTH: tcc->echo_fb_gatelength = value; break;
      case SEQ_CC_ECHO_FB_TICKS: tcc->echo_fb_ticks = value; break;
  
      default:
	portEXIT_CRITICAL();
        return -2; // invalid CC
    }
  }

  portEXIT_CRITICAL();

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

  if( cc < 0x40 ) {
    return tcc->lay_const[cc];
  }

  switch( cc ) {
    case SEQ_CC_MODE: return tcc->mode.playmode;
    case SEQ_CC_MODE_FLAGS: return tcc->mode.flags;
    case SEQ_CC_MIDI_EVENT_MODE: return tcc->event_mode;
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
    case SEQ_CC_ASG_ACCENT: return tcc->trg_assignments.accent;
    case SEQ_CC_ASG_ROLL: return tcc->trg_assignments.roll;
    case SEQ_CC_ASG_GLIDE: return tcc->trg_assignments.glide;
    case SEQ_CC_ASG_SKIP: return tcc->trg_assignments.skip;
    case SEQ_CC_ASG_RANDOM_GATE: return tcc->trg_assignments.random_gate;
    case SEQ_CC_ASG_RANDOM_VALUE: return tcc->trg_assignments.random_value;
    case SEQ_CC_ASG_NO_FX: return tcc->trg_assignments.no_fx;

    case SEQ_CC_PAR_ASG_DRUM_LAYER_A: return tcc->par_assignment_drum[0];
    case SEQ_CC_PAR_ASG_DRUM_LAYER_B: return tcc->par_assignment_drum[1];

#if 0  
    case SEQ_CC_CHANGE_STEP: return 0; // TODO
#endif

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



/////////////////////////////////////////////////////////////////////////////
// Called whenever the event mode or par layer assignments have been changed
/////////////////////////////////////////////////////////////////////////////
static s32 CC_LinkUpdate(u8 track)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  seq_par_layer_type_t *par_asg = (tcc->event_mode == SEQ_EVENT_MODE_Drum)
    ? (seq_par_layer_type_t *)&seq_cc_trk[track].par_assignment_drum[0]
    : (seq_par_layer_type_t *)&seq_cc_trk[track].lay_const[0*16];

  tcc->link_par_layer_note = -1;
  tcc->link_par_layer_chord = -1;
  tcc->link_par_layer_velocity = -1;
  tcc->link_par_layer_length = -1;
  tcc->link_par_layer_probability = -1;
  tcc->link_par_layer_delay = -1;
  tcc->link_par_layer_roll = -1;

  u8 num_layers = SEQ_PAR_NumLayersGet(track);
  if( num_layers ) {
    // search backwards to ensure that first assignments will be taken
    int layer;
    for(layer=num_layers-1; layer>=0; --layer) {
      switch( par_asg[layer] ) {
        case SEQ_PAR_Type_Note: tcc->link_par_layer_note = layer; break;
        case SEQ_PAR_Type_Chord: tcc->link_par_layer_chord = layer; break;
        case SEQ_PAR_Type_Velocity: tcc->link_par_layer_velocity = layer; break;
        case SEQ_PAR_Type_Length: tcc->link_par_layer_length = layer; break;
        case SEQ_PAR_Type_Probability: tcc->link_par_layer_probability = layer; break;
        case SEQ_PAR_Type_Delay: tcc->link_par_layer_delay = layer; break;
        case SEQ_PAR_Type_Roll: tcc->link_par_layer_roll = layer; break;
      }
    }
  }

  return 0; // no error
}
