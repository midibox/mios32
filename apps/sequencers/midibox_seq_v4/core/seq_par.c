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
  u8 num_p_steps = par_layer_num_steps[track];
  if( step >= num_p_steps )
    return -3;

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
  u8 num_p_steps = par_layer_num_steps[track];
  u16 step_ix = (par_instrument * num_p_layers * num_p_steps) + (par_layer * num_p_steps) + step;
  if( step_ix >= SEQ_PAR_MAX_BYTES )
    return 0; // invalid step position: return 0 (parameter not set)

  return seq_par_layer_value[track][step_ix];
}
