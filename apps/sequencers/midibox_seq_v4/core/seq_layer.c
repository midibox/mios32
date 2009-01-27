// $Id$
/*
 * Sequencer Parameter Layer Routines
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

#include "seq_layer.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_chord.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions and arrays
/////////////////////////////////////////////////////////////////////////////

// Control Type Names as 4 character string
static const char seq_layer_names[][5] = {
  "None",	//   SEQ_LAYER_ControlType_None,
  "Note",	//   SEQ_LAYER_ControlType_Note,
  "Vel,",	//   SEQ_LAYER_ControlType_Velocity,
  "Crd.",	//   SEQ_LAYER_ControlType_Chord,
  "Vel.",	//   SEQ_LAYER_ControlType_Chord_Velocity,
  "Len.",	//   SEQ_LAYER_ControlType_Length,
  " CC "	//   SEQ_LAYER_ControlType_CC
};

// preset table - each entry contains the initial track configuration and the step settings
// EVNT0 and CHN won't be overwritten
// for four steps (they are copied to all other 4step groups)
// structure of track settings matches with the SEQ_TRKRECORD in app_defines.h

// parameters which will be reset to the given values in all event modes
static const u8 seq_layer_preset_table_static[][2] = {
  // parameter             value
  { SEQ_CC_MODE,           SEQ_CORE_TRKMODE_Normal },
  { SEQ_CC_MODE_FLAGS,     0x01 },
  { SEQ_CC_DIRECTION,	   0x00 },
  { SEQ_CC_STEPS_REPLAY,   0x00 },
  { SEQ_CC_STEPS_FORWARD,  0x00 },
  { SEQ_CC_STEPS_JMPBCK,   0x00 },
  { SEQ_CC_CLK_DIVIDER,    0x03 },
  { SEQ_CC_CLKDIV_FLAGS,   0x00 },
  { SEQ_CC_LENGTH,         0x0f },
  { SEQ_CC_LOOP,           0x00 },
  { SEQ_CC_TRANSPOSE_SEMI, 0x00 },
  { SEQ_CC_TRANSPOSE_OCT,  0x00 },
  { SEQ_CC_GROOVE_VALUE,   0x00 },
  { SEQ_CC_GROOVE_STYLE,   0x00 },
  { SEQ_CC_MORPH_MODE,     0x00 },
  { SEQ_CC_MORPH_SPARE,    0x00 },
  { SEQ_CC_HUMANIZE_VALUE, 0x00 },
  { SEQ_CC_HUMANIZE_MODE,  0x00 },
  { SEQ_CC_ASG_GATE,       0x01 },
  { SEQ_CC_ASG_ACCENT,     0x02 },
  { SEQ_CC_ASG_ROLL,       0x03 },
  { SEQ_CC_ASG_SKIP,       0x04 },
  { SEQ_CC_ASG_GLIDE,      0x05 },
  { SEQ_CC_ASG_RANDOM_GATE, 0x06 },
  { SEQ_CC_ASG_RANDOM_VALUE, 0x07 },
  { SEQ_CC_ASG_NO_FX,      0x08 },
  { SEQ_CC_ECHO_REPEATS,   0x00 },
  { SEQ_CC_ECHO_DELAY,     0x07 }, // 1/8
  { SEQ_CC_ECHO_VELOCITY,  15 }, // 75%
  { SEQ_CC_ECHO_FB_VELOCITY, 15 }, // 75%
  { SEQ_CC_ECHO_FB_NOTE,   24 }, // +0
  { SEQ_CC_ECHO_FB_GATELENGTH, 20 }, // 100%
  { SEQ_CC_ECHO_FB_TICKS,  20 }, // 100%
  { 0xff,                  0xff } // end marker
};


// initial drum notes
static const u8 seq_layer_preset_table_drum_notes[16] = {
  0x24,
  0x26,
  0x29,
  0x2b,
  0x2f,
  0x27,
  0x46,
  0x4b,
  0x38,
  0x31,
  0x2e,
  0x2a,
  0x2c,
  0x3c,
  0x3d,
  0x4c
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_Init(u32 mode)
{
  // initialize parameter/trigger layers and CCs
  SEQ_PAR_Init(0);
  SEQ_TRG_Init(0);
  SEQ_CC_Init(0);

  // copy preset into all tracks
  u8 track;
  for(track=0; track<16; ++track) {
    u8 only_layers = 0;
    u8 all_triggers_cleared = (track >= 1) ? 1 : 0; // triggers only set for first track
    SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared);
  }


  SEQ_CHORD_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the layer_evnt_t information based on given
// layer (used for visualisation purposes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvntOfLayer(u8 track, u8 step, u8 layer, seq_layer_evnt_t *layer_event)
{
  seq_layer_evnt_t layer_events[16];
  s32 number_of_events = 0;

  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  number_of_events = SEQ_LAYER_GetEvents(track, step, layer_events);

  if( number_of_events <= 0 ) {
    // fill with dummy data
    layer_event->midi_package.ALL = 0;
    layer_event->len = -1;

    return -1; // no valid event
  }

  u8 event_num;
  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    event_num = layer;
  } else {
    event_num = 0; // TODO
  }

  layer_event->midi_package = layer_events[event_num].midi_package;
  layer_event->len = layer_events[event_num].len;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the control type of a variable layer
/////////////////////////////////////////////////////////////////////////////
seq_layer_ctrl_type_t SEQ_LAYER_GetVControlType(u8 track, u8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    return par_layer ? SEQ_LAYER_ControlType_Length : SEQ_LAYER_ControlType_Velocity; // TODO: variable assignments
  } else {
    if( par_layer == 0 ) {
      if( tcc->event_mode == SEQ_EVENT_MODE_CC )
	return SEQ_LAYER_ControlType_CC;
      else if( tcc->event_mode == SEQ_EVENT_MODE_Chord )
	return SEQ_LAYER_ControlType_Chord;
      else
	return SEQ_LAYER_ControlType_Note;
    } else if( par_layer == 1 ) {
      if( tcc->event_mode == SEQ_EVENT_MODE_CC )
	return SEQ_LAYER_ControlType_CC;
      else if( tcc->event_mode == SEQ_EVENT_MODE_Chord )
	return SEQ_LAYER_ControlType_Chord_Velocity;
      else
	return SEQ_LAYER_ControlType_Velocity;
    } else if( par_layer == 2 ) {
      if( tcc->event_mode == SEQ_EVENT_MODE_CC )
	return SEQ_LAYER_ControlType_CC;
      else
	return SEQ_LAYER_ControlType_Length;
    } else {
      return SEQ_LAYER_ControlType_None; // TODO
    }
  }

  return SEQ_LAYER_ControlType_None;
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the name of a control type as 4 character string
/////////////////////////////////////////////////////////////////////////////
char *SEQ_LAYER_GetVControlTypeString(u8 track, u8 par_layer)
{
  return (char *)seq_layer_names[SEQ_LAYER_GetVControlType(track, par_layer)];
}



/////////////////////////////////////////////////////////////////////////////
// Layer Event Modes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvents(u8 track, u8 step, seq_layer_evnt_t layer_events[16])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 num_events = 0;

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 step_par_layer = step % SEQ_PAR_NumStepsGet(track);
    u8 num_layers = SEQ_TRG_NumLayersGet(track); // TODO: find proper solution to define 2*16 (Gate/Accent) case
    u8 num_notes = (num_layers > 16) ? 16 : num_layers;
    int drum;
    for(drum=0; drum<num_notes; ++drum) {
      seq_layer_evnt_t *e = &layer_events[drum];
      mios32_midi_package_t *p = &e->midi_package;
      p->type     = NoteOn;
      p->event    = NoteOn;
      p->chn      = tcc->midi_chn; // TODO: optionally different channel taken from const D
      p->note     = tcc->lay_const[0*16 + drum];

      if( !SEQ_TRG_Get(track, step, drum) )
	p->velocity = 0;
      else {
	// TODO: selection from variable layer
	if( num_layers > num_notes ) {
	  // gate/accent layers
	  p->velocity = SEQ_TRG_Get(track, step, drum + 1*16) ? tcc->lay_const[2*16 + drum] : tcc->lay_const[1*16 + drum];
	} else { // Vel_N/A
	  p->velocity = tcc->lay_const[1*16 + drum];
	}
      }

      e->len      = 0x11; // TODO: selection from variable layer

      ++num_events;
    }
  } else {
    // static assignments for Parameter Layer A/B/C
    if( tcc->event_mode == SEQ_EVENT_MODE_Chord ) {
      u8 chord_value = SEQ_PAR_Get(track, step, 0);
      for(num_events=0; num_events<4; ++num_events) {
	s32 note = SEQ_CHORD_NoteGet(num_events, chord_value);
	if( note < 0 )
	  break;

	seq_layer_evnt_t *e = &layer_events[num_events];
	mios32_midi_package_t *p = &e->midi_package;
	p->type     = NoteOn;
	p->event    = NoteOn;
	p->chn      = tcc->midi_chn;
	p->note     = note;
	p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, step, 1) : 0x00;
	e->len      = SEQ_PAR_Get(track, step, 2);
      }
    } else {
      seq_layer_evnt_t *e = &layer_events[0];
      mios32_midi_package_t *p = &e->midi_package;
      p->type     = NoteOn;
      p->event    = NoteOn;
      p->chn      = tcc->midi_chn;
      p->note     = SEQ_PAR_Get(track, step, 0);
      p->velocity = SEQ_TRG_GateGet(track, step) ? SEQ_PAR_Get(track, step, 1) : 0x00;
      e->len      = SEQ_PAR_Get(track, step, 2);
      ++num_events;
    }
  }

  return num_events;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a track depending on selected event mode
// "only_layers" flag is used by CLR function
// "all_triggers_cleared": if 0, triggers will be initialized according to preset
//                         if 1: triggers will be cleared
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers, u8 all_triggers_cleared)
{
  int i;
  int cc;

  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);

  if( !only_layers ) {
    // copy static presets
    i = 0;
    while( (cc=seq_layer_preset_table_static[i][0]) != 0xff ) {
      SEQ_CC_Set(track, cc, seq_layer_preset_table_static[i][1]);
      i++;
    }

    // copy evnt mode depending settings
    switch( event_mode ) {
      case SEQ_EVENT_MODE_Note: {
      } break;

      case SEQ_EVENT_MODE_Chord: {
      } break;

      case SEQ_EVENT_MODE_CC: {
      } break;

      case SEQ_EVENT_MODE_Drum: {
	int drum;
	for(drum=0; drum<16; ++drum) {
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+drum, seq_layer_preset_table_drum_notes[drum]);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+drum, 100);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_C1+drum, 127);
	}
      } break;
    }
  }

  // get constraints of trigger layers
  int num_t_layers = SEQ_TRG_NumLayersGet(track);
  int num_t_steps  = SEQ_TRG_NumStepsGet(track);;

  // copy trigger layer values
  if( !all_triggers_cleared ) {
    int step8;

    if( event_mode == SEQ_EVENT_MODE_CC ) {
      // CC: enable gate for all steps
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, 0, 0xff);
    } else {
      // Note/Chord/Drum: enable trigger for each beat of (first) gate layer
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, 0, 0x11);
    }
  }

  // get constraints of parameter layers
  int num_p_layers = SEQ_PAR_NumLayersGet(track);
  int num_p_steps  = SEQ_PAR_NumStepsGet(track);;

  // copy parameter layer values
  int step;
  int par_layer;
  switch( event_mode ) {
    case SEQ_EVENT_MODE_Note: {
      for(step=0; step<num_p_steps; ++step) {
	SEQ_PAR_Set(track, step, 0, 0x3c); // C-3
	SEQ_PAR_Set(track, step, 1,  100); // velocity
	SEQ_PAR_Set(track, step, 2, 0x11); // length
      }
    } break;

    case SEQ_EVENT_MODE_Chord: {
      for(step=0; step<num_p_steps; ++step) {
	SEQ_PAR_Set(track, step, 0, 0x40); // Chord
	SEQ_PAR_Set(track, step, 1,  100); // velocity
	SEQ_PAR_Set(track, step, 2, 0x11); // length
      }
    } break;

    case SEQ_EVENT_MODE_CC: {
      for(step=0; step<num_p_steps; ++step) {
	SEQ_PAR_Set(track, step, 0, 0x40); // CC#16
	SEQ_PAR_Set(track, step, 1, 0x40); // CC#17
	SEQ_PAR_Set(track, step, 2, 0x40); // CC#18
      }
    } break;

    case SEQ_EVENT_MODE_Drum: {
      // TODO: variable routing
      for(step=0; step<num_p_steps; ++step) {
	SEQ_PAR_Set(track, step, 0,  100); // velocity
	SEQ_PAR_Set(track, step, 1, 0x11); // length
      }
    } break;
  }

  if( event_mode != SEQ_EVENT_MODE_Drum ) {
    // TODO: remaining layers
  }

  return 0; // no error
}
