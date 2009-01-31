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
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// to display activity of selected track in trigger/parameter selection page
u8 seq_layer_vu_meter[16];


/////////////////////////////////////////////////////////////////////////////
// Local definitions and arrays
/////////////////////////////////////////////////////////////////////////////

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
    u8 init_assignments = 1;
    SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);
  }


  SEQ_CHORD_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the layer_evnt_t information based on given
// layer (used for visualisation purposes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvntOfLayer(u8 track, u16 step, u8 layer, seq_layer_evnt_t *layer_event)
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
// Layer Event Modes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvents(u8 track, u16 step, seq_layer_evnt_t layer_events[16])
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 num_events = 0;

  u8 handle_vu_meter = (ui_page == SEQ_UI_PAGE_TRGSEL || ui_page == SEQ_UI_PAGE_PARSEL) && track == SEQ_UI_VisibleTrackGet();

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(track); // we assume, that PAR layer has same number of instruments!
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    u8 num_p_steps = SEQ_PAR_NumStepsGet(track);
    u16 step_par_layer = step % num_p_steps;

    u8 drum;
    for(drum=0; drum<num_instruments; ++drum) {
      seq_layer_evnt_t *e = &layer_events[drum];
      mios32_midi_package_t *p = &e->midi_package;
      p->type     = NoteOn;
      p->event    = NoteOn;
      p->chn      = tcc->midi_chn; // TODO: optionally different channel taken from const D
      p->note     = tcc->lay_const[0*16 + drum];

      if( !SEQ_TRG_Get(track, step, 0, drum) )
	p->velocity = 0;
      else {
	if( num_p_layers == 2 )
	  p->velocity = SEQ_PAR_VelocityGet(track, step_par_layer, drum);
	else
	  p->velocity = tcc->lay_const[1*16 + drum];
      }

      if( handle_vu_meter && p->velocity ) {
	seq_layer_vu_meter[drum] = p->velocity;
      } else {
	seq_layer_vu_meter[drum] &= 0x7f; // ensure that no static assignment is displayed
      }

      e->len = SEQ_PAR_LengthGet(track, step_par_layer, drum);

      ++num_events;
    }
  } else {
    u8 instrument = 0;
    int par_layer;

    // get velocity and length from first parameter layer which holds it
    // if not assigned, we will get back a default value
    
    u8 velocity = 100; // default velocity
    if( (par_layer=tcc->link_par_layer_velocity) >= 0 ) {
      velocity = SEQ_TRG_GateGet(track, step, instrument) ? SEQ_PAR_Get(track, step, par_layer, instrument) : 0;
      if( handle_vu_meter )
	seq_layer_vu_meter[par_layer] = velocity | 0x80;
    }

    u8 length = 71; // default length
    if( (par_layer=tcc->link_par_layer_length) >= 0 ) {
      length = SEQ_PAR_Get(track, step, par_layer, instrument);
      if( length > 95 )
	length = 95;
      ++length;
	  
      if( handle_vu_meter )
	seq_layer_vu_meter[par_layer] = length | 0x80;
    }

    // go through all layers to generate events
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
      switch( tcc->lay_const[0*16 + par_layer] ) {

        case SEQ_PAR_Type_Note: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;

	  p->type     = NoteOn;
	  p->event    = NoteOn;
	  p->chn      = tcc->midi_chn;
	  p->note     = SEQ_PAR_Get(track, step, par_layer, instrument);
	  p->velocity = velocity;
	  e->len      = length;
	  ++num_events;

	  if( handle_vu_meter && velocity )
	    seq_layer_vu_meter[par_layer] = velocity;

	} break;

        case SEQ_PAR_Type_Chord: {
	  u8 chord_value = SEQ_PAR_Get(track, step, par_layer, instrument);
	  int i;
	  for(i=0; i<4; ++i) {
	    if( num_events >= 16 )
	      break;

	    s32 note = SEQ_CHORD_NoteGet(i, chord_value);
	    if( note < 0 )
	      break;

	    seq_layer_evnt_t *e = &layer_events[num_events];
	    mios32_midi_package_t *p = &e->midi_package;
	    p->type     = NoteOn;
	    p->event    = NoteOn;
	    p->chn      = tcc->midi_chn;
	    p->note     = note;
	    p->velocity = velocity;
	    e->len      = length;
	    ++num_events;
	  }

	  if( handle_vu_meter && velocity )
	    seq_layer_vu_meter[par_layer] = velocity;

	} break;

        case SEQ_PAR_Type_CC: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;

	  p->type     = CC;
	  p->event    = CC;
	  p->chn      = tcc->midi_chn;
	  p->note     = tcc->lay_const[1*16 + par_layer];
	  p->value    = SEQ_PAR_Get(track, step, par_layer, instrument);
	  e->len      = -1;
	  ++num_events;

	  if( handle_vu_meter )
	    seq_layer_vu_meter[par_layer] = p->value | 0x80;

	} break;

        case SEQ_PAR_Type_PitchBend: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  p->type     = PitchBend;
	  p->event    = PitchBend;
	  p->chn      = tcc->midi_chn;
	  p->evnt1    = value; // LSB (TODO: check if re-using the MSB is useful)
	  p->evnt2    = value; // MSB
	  e->len      = -1;
	  ++num_events;

	  if( handle_vu_meter )
	    seq_layer_vu_meter[par_layer] = p->evnt2 | 0x80;

	} break;

      }

      if( num_events >= 16 )
	break;
    }
  }

  return num_events;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a track depending on selected event mode
// "only_layers" flag is used by CLR function
// "all_triggers_cleared": if 0, triggers will be initialized according to preset
//                         if 1: triggers will be cleared
// "init_assignments": if 1, parameter/trigger assignments will be changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers, u8 all_triggers_cleared, u8 init_assignments)
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

    // initialize assignments
    if( init_assignments ) {
      switch( event_mode ) {
        case SEQ_EVENT_MODE_Note:
        case SEQ_EVENT_MODE_Chord: {
	  // Trigger Layer Assignments
	  for(i=0; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, i+1);

	  // Parameter Layer Assignments
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1, (event_mode == SEQ_EVENT_MODE_Chord) ? SEQ_PAR_Type_Chord : SEQ_PAR_Type_Note);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A2, SEQ_PAR_Type_Velocity);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A3, SEQ_PAR_Type_Length);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A4, SEQ_PAR_Type_Roll);

	  for(i=0; i<16; ++i)
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 16+i);
        } break;

        case SEQ_EVENT_MODE_CC: {
	  // Trigger Layer Assignments
	  for(i=0; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, i+1);

	  // Parameter Layer Assignments
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1, SEQ_PAR_Type_CC);

	  for(i=0; i<16; ++i)
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 16+i);
        } break;

        case SEQ_EVENT_MODE_Drum: {
	  // Trigger Layer Assignments
	  SEQ_CC_Set(track, SEQ_CC_ASG_GATE, 1);
	  SEQ_CC_Set(track, SEQ_CC_ASG_ACCENT, (SEQ_TRG_NumLayersGet(track) > 1) ? 2 : 0);
	  for(i=2; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, 0); // not relevant in drum mode

	  // parameter layer assignments
	  if( SEQ_TRG_NumLayersGet(track) > 1 ) {
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Velocity);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_B, SEQ_PAR_Type_Roll);
	  } else {
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Roll);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_B, SEQ_PAR_Type_None);
	  }

	  // Constant Layer Values
	  int drum;
	  for(drum=0; drum<16; ++drum) {
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+drum, seq_layer_preset_table_drum_notes[drum]);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+drum, 100);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_C1+drum, 127);
	  }
	} break;
      }
    }

    // copy event mode depending settings
    switch( event_mode ) {
      case SEQ_EVENT_MODE_Note:
      case SEQ_EVENT_MODE_Chord:
      case SEQ_EVENT_MODE_CC: {
	for(i=0; i<16; ++i)
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 16+i);
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
  int num_t_instruments = SEQ_TRG_NumInstrumentsGet(track);
  int num_t_layers = SEQ_TRG_NumLayersGet(track);
  int num_t_steps  = SEQ_TRG_NumStepsGet(track);;

  // copy trigger layer values
  if( !all_triggers_cleared ) {
    int step8;
    u8 instrument = 0;
    u8 layer = 0;

    if( event_mode == SEQ_EVENT_MODE_CC ) {
      // CC: enable gate for all steps
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, layer, instrument, 0xff);
    } else {
      // Note/Chord/Drum: enable trigger for each beat of (first) gate layer
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, layer, instrument, 0x11);
    }
  }


  // copy parameter layer values
  int par_layer;
  int num_p_layers = SEQ_PAR_NumLayersGet(track);
  for(par_layer=0; par_layer<num_p_layers; ++par_layer)
    SEQ_LAYER_CopyParLayerPreset(track, par_layer);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copies parameter layer preset depending on selected parameter type
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_CopyParLayerPreset(u8 track, u8 par_layer)
{
  int num_p_instruments = SEQ_PAR_NumInstrumentsGet(track);
  int num_p_steps  = SEQ_PAR_NumStepsGet(track);;

  u8 init_value = SEQ_PAR_InitValueGet(SEQ_PAR_AssignmentGet(track, par_layer));

  int step;
  int instrument;
  for(instrument=0; instrument<num_p_instruments; ++instrument)
    for(step=0; step<num_p_steps; ++step)
      SEQ_PAR_Set(track, step, par_layer, instrument, init_value);

  return 0; // no error
}
