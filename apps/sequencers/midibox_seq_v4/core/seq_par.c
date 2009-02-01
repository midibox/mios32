// $Id$
/*
 * Parameter Layer Routines
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

#include "seq_par.h"
#include "seq_cc.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_B, remaining functions should
// use SEQ_PAR_Get/Set
u8 seq_par_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_PAR_MAX_BYTES];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 par_layer_num_steps[SEQ_CORE_NUM_TRACKS];
static u8 par_layer_num_layers[SEQ_CORE_NUM_TRACKS];
static u8 par_layer_num_instruments[SEQ_CORE_NUM_TRACKS];

static const char seq_par_type_names[SEQ_PAR_NUM_TYPES][6] = {
  "None ", // 0
  "Note ", // 1
  "Chord", // 2
  "Vel. ", // 3
  "Len. ", // 4
  " CC  ", // 5
  "Pitch", // 6
  "Prob.", // 7
  "Delay", // 8
  "Roll ", // 9
  "BaseN", // 10
  "LoopB"  // 11
};

static const u8 seq_par_default_value[SEQ_PAR_NUM_TYPES] = {
  0x00, // None
  0x3c, // Note: C-3
  0x40, // Chord: 0
  100,  // Velocity
  71,   // Length
  64,   // CC
  64,   // PitchBender
  0,    // Probability (reversed: 100%)
  0,    // Delay
  0,    // Roll
  0x3c, // Base Note
  0x00  // Loopback
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Init(u32 mode)
{
  // init parameter layer values
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    SEQ_PAR_TrackInit(track, 64, 16, 1); // track, steps, parameter layers, instruments

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Inits all parameter layers of the given track with the given constraints
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_TrackInit(u8 track, u16 steps, u8 par_layers, u8 instruments)
{
  if( (instruments * par_layers * steps) > SEQ_PAR_MAX_BYTES )
    return -1; // invalid configuration

  par_layer_num_layers[track] = par_layers;
  par_layer_num_steps[track] = steps;
  par_layer_num_instruments[track] = instruments;

  // init parameter layer values
  memset((u8 *)&seq_par_layer_value[track], 0, SEQ_PAR_MAX_BYTES);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of instruments for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_NumInstrumentsGet(u8 track)
{
  return par_layer_num_instruments[track];
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of trigger layers for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_NumLayersGet(u8 track)
{
  return par_layer_num_layers[track];
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of steps for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_NumStepsGet(u8 track)
{
  return par_layer_num_steps[track];
}


/////////////////////////////////////////////////////////////////////////////
// returns the assignment of a certain parameter layer
/////////////////////////////////////////////////////////////////////////////
seq_par_layer_type_t SEQ_PAR_AssignmentGet(u8 track, u8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum )
    return (par_layer >= 2) ? SEQ_PAR_Type_None : tcc->par_assignment_drum[par_layer];
  else
    return (par_layer >= 16) ? SEQ_PAR_Type_None : tcc->lay_const[0*16 + par_layer];
}


/////////////////////////////////////////////////////////////////////////////
// Sets a value of parameter layer
// (using this interface function to allow dynamic lists in future)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Set(u8 track, u16 step, u8 par_layer, u8 par_instrument, u8 value)
{
  u8 num_p_instruments = par_layer_num_instruments[track];
  if( par_instrument >= num_p_instruments )
    return -1;
  u8 num_p_layers = par_layer_num_layers[track];
  if( par_layer >= num_p_layers )
    return -2;
  u16 num_p_steps = par_layer_num_steps[track];

  // modulo of num_p_steps to allow mirroring of parameter layer in drum mode
  step %= num_p_steps;

  u16 step_ix = (par_instrument * num_p_layers * num_p_steps) + (par_layer * num_p_steps) + step;
  if( step_ix >= SEQ_PAR_MAX_BYTES )
    return -4; // invalid step position

  seq_par_layer_value[track][step_ix] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns a value of parameter layer
// (using this interface function to allow dynamic lists in future)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Get(u8 track, u16 step, u8 par_layer, u8 par_instrument)
{
  u8 num_p_layers = par_layer_num_layers[track];
  u16 num_p_steps = par_layer_num_steps[track];

  // modulo of num_p_steps to allow mirroring of parameter layer in drum mode
  step %= num_p_steps;

  u16 step_ix = (par_instrument * num_p_layers * num_p_steps) + (par_layer * num_p_steps) + step;
  if( step_ix >= SEQ_PAR_MAX_BYTES )
    return 0; // invalid step position: return 0 (parameter not set)

  return seq_par_layer_value[track][step_ix];
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which plays a note
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_NoteGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_note) >= 0 )
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which plays a chord
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_chord) >= 0 )
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which controls velocity
// if not assigned to a layer, returns 100 as default velocity
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_VelocityGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  // note: due to performance reasons, similar code exists in SEQ_LAYER_GetEvents()

  if( (par_layer=tcc->link_par_layer_velocity) >= 0 )
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);

  return 100; // default velocity
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which controls note length
// returns 1..96 (71 if no length assigned)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_LengthGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  // note: due to performance reasons, similar code exists in SEQ_LAYER_GetEvents()

  if( (par_layer=tcc->link_par_layer_length) >= 0 ) {
    u8 value = SEQ_PAR_Get(track, step, par_layer, par_instrument);
    return ((value > 95) ? 95 : value) + 1;
  }

  return 71; // no assignment
}


/////////////////////////////////////////////////////////////////////////////
// returns the play probability if assigned to any parameter layer
// Probability ranges from 0 to 100 for 0%..100%
// (the stored value is inverted, to that 0 results into 100%)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_ProbabilityGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_probability) >= 0 ) {
    u8 value = SEQ_PAR_Get(track, step, par_layer, par_instrument);
    return (value >= 100) ? 0 : 100-value;
  }

  return 100; // 100% probability
}


/////////////////////////////////////////////////////////////////////////////
// returns the step delay if assigned to any parameter layer
// Delay ranges from -95..95
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_StepDelayGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_delay) >= 0 ) {
    s32 value = SEQ_PAR_Get(track, step, par_layer, par_instrument);
    if( value >= 128 ) {
      value = 0 - value;
      return (value < -95) ? -95 : value;
    } else {
      return (value > 95) ? 95 : value;
    }
  }

  return 0; // no delay
}


/////////////////////////////////////////////////////////////////////////////
// returns the roll mode if assigned to any parameter layer
//   0:      none
//   1.. 15: 2D01..2D15
//  16.. 31: 3D00..3D15
//  32.. 47: 4D00..4D15
//  48.. 63: 5D00..5D15
//  64.. 79: 2U00..2U15
//  80.. 95: 3U00..3U15
//  96..111: 4U00..4U15
// 112..127: 5U00..5U15
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_RollModeGet(u8 track, u8 step, u8 par_instrument)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_roll) >= 0 ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no roll
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the string to a parameter assignment type
/////////////////////////////////////////////////////////////////////////////
char *SEQ_PAR_TypeStr(seq_par_layer_type_t par_type)
{
  if( par_type >= SEQ_PAR_NUM_TYPES )
    return (char *)"-----";
  else
    return (char *)seq_par_type_names[par_type];
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the string to the assigned parameter
// if layer is not assigned, it returns "None "
/////////////////////////////////////////////////////////////////////////////
char *SEQ_PAR_AssignedTypeStr(u8 track, u8 par_layer)
{
  return SEQ_PAR_TypeStr(SEQ_PAR_AssignmentGet(track, par_layer));
}


/////////////////////////////////////////////////////////////////////////////
// Returns initial value of given parameter assignment type
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_PAR_InitValueGet(seq_par_layer_type_t par_type)
{
  if( par_type >= SEQ_PAR_NUM_TYPES )
    return 0;
  else
    return seq_par_default_value[par_type];
}



