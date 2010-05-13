// $Id$
/*
 * Trigger Layer Routines
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

#include "seq_trg.h"
#include "seq_core.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_B, remaining functions should
// use SEQ_TRG_Get/Set
u8 seq_trg_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_TRG_MAX_BYTES];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 trg_layer_num_steps8[SEQ_CORE_NUM_TRACKS];
static u8 trg_layer_num_layers[SEQ_CORE_NUM_TRACKS];
static u8 trg_layer_num_instruments[SEQ_CORE_NUM_TRACKS];

static const char seq_trg_names[8][6] = {
  "Gate ", // 0
  "Acc. ", // 1
  "Roll ", // 2
  "Glide", // 3
  " Skip", // 4
  " R.G ", // 5
  " R.V ", // 6
  "No Fx"  // 7
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Init(u32 mode)
{
  // init trigger layer values
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {

    SEQ_TRG_TrackInit(track, 256, 8, 1); // track, steps, trigger layers, instruments

    // special init value for first track: set gates on each beat
    if( track == 0 )
      memset((u8 *)&seq_trg_layer_value[track], 0x11, trg_layer_num_steps8[track]);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Inits all trigger layers of the given track with the given constraints
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_TrackInit(u8 track, u16 steps, u8 trg_layers, u8 instruments)
{
  if( (instruments * trg_layers * (steps/8)) > SEQ_TRG_MAX_BYTES )
    return -1; // invalid configuration

  trg_layer_num_layers[track] = trg_layers;
  trg_layer_num_steps8[track] = steps/8;
  trg_layer_num_instruments[track] = instruments;

  // init trigger layer values
  memset((u8 *)&seq_trg_layer_value[track], 0, SEQ_TRG_MAX_BYTES);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of instruments for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_NumInstrumentsGet(u8 track)
{
  return trg_layer_num_instruments[track];
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of trigger layers for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_NumLayersGet(u8 track)
{
  return trg_layer_num_layers[track];
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of steps for a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_NumStepsGet(u8 track)
{
  return trg_layer_num_steps8[track]*8;
}


/////////////////////////////////////////////////////////////////////////////
// returns the assignment of a certain trigger
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_AssignmentGet(u8 track, u8 trg_num)
{
  return (seq_cc_trk[track].trg_assignments.ALL >> (trg_num*4)) & 0xf;
}


/////////////////////////////////////////////////////////////////////////////
// returns value of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Get(u8 track, u16 step, u8 trg_layer, u8 trg_instrument)
{
  u8 num_t_layers = trg_layer_num_layers[track];
  u8 num_t_steps8 = trg_layer_num_steps8[track];
  u16 step_ix = (trg_instrument * num_t_layers * num_t_steps8) + (trg_layer * num_t_steps8) + (step/8);
  if( step_ix >= SEQ_TRG_MAX_BYTES )
    return 0; // invalid step position: return 0 (trigger not set)

  u8 step_mask = 1 << (step % 8);
  return (seq_trg_layer_value[track][step_ix] & step_mask) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 8 steps of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Get8(u8 track, u8 step8, u8 trg_layer, u8 trg_instrument)
{
  u8 num_t_layers = trg_layer_num_layers[track];
  u8 num_t_steps8 = trg_layer_num_steps8[track];
  u16 step_ix = (trg_instrument * num_t_layers * num_t_steps8) + (trg_layer * num_t_steps8) + step8;
  if( step_ix >= SEQ_TRG_MAX_BYTES )
    return 0; // invalid step position: return 0 (trigger not set)

  return seq_trg_layer_value[track][step_ix];
}


/////////////////////////////////////////////////////////////////////////////
// returns 16 steps of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Get16(u8 track, u8 step16, u8 trg_layer, u8 trg_instrument)
{
  u8 num_t_layers = trg_layer_num_layers[track];
  u8 num_t_steps8 = trg_layer_num_steps8[track];
  u16 step_ix = (trg_instrument * num_t_layers * num_t_steps8) + (trg_layer * num_t_steps8) + step16;
  if( step_ix >= SEQ_TRG_MAX_BYTES )
    return 0; // invalid step position: return 0 (trigger not set)

  u8 *values = (u8 *)&seq_trg_layer_value[track][step_ix];
  return *values++ | (*values << 8);
}


/////////////////////////////////////////////////////////////////////////////
// returns value of assigned layers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_GateGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return SEQ_TRG_Get(track, step, 0, trg_instrument);

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.gate;
  // gate always set if not assigned
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 1;
}

s32 SEQ_TRG_AccentGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return (trg_layer_num_layers[track] >= 2) ? SEQ_TRG_Get(track, step, 1, trg_instrument) : 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.accent;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_RollGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.roll;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_GlideGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.glide;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_SkipGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.skip;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_RandomGateGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_gate;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_RandomValueGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_value;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}

s32 SEQ_TRG_NoFxGet(u8 track, u16 step, u8 trg_instrument)
{
  if( seq_cc_trk[track].event_mode == SEQ_EVENT_MODE_Drum )
    return 0;

  u8 trg_assignment = seq_cc_trk[track].trg_assignments.no_fx;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1, trg_instrument) : 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets value of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Set(u8 track, u16 step, u8 trg_layer, u8 trg_instrument, u8 value)
{
  u8 num_t_instruments = trg_layer_num_instruments[track];
  if( trg_instrument >= num_t_instruments )
    return -1;
  u8 num_t_layers = trg_layer_num_layers[track];
  if( trg_layer >= num_t_layers )
    return -2;
  u8 num_t_steps8 = trg_layer_num_steps8[track];
  if( (step/8) >= num_t_steps8 )
    return -3;

  u16 step_ix = (trg_instrument * num_t_layers * num_t_steps8) + (trg_layer * num_t_steps8) + (step/8);
  if( step_ix >= SEQ_TRG_MAX_BYTES )
    return -4; // invalid step position

  u8 step_mask = 1 << (step % 8);

  if( value )
    seq_trg_layer_value[track][step_ix] |= step_mask;
  else
    seq_trg_layer_value[track][step_ix] &= ~step_mask;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// sets 8 steps of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Set8(u8 track, u8 step8, u8 trg_layer, u8 trg_instrument, u8 value)
{
  u8 num_t_instruments = trg_layer_num_instruments[track];
  if( trg_instrument >= num_t_instruments )
    return -1;
  u8 num_t_layers = trg_layer_num_layers[track];
  if( trg_layer >= num_t_layers )
    return -2;
  u8 num_t_steps8 = trg_layer_num_steps8[track];
  if( step8 >= num_t_steps8 )
    return -3;

  u16 step_ix = (trg_instrument * num_t_layers * num_t_steps8) + (trg_layer * num_t_steps8) + step8;
  if( step_ix >= SEQ_TRG_MAX_BYTES )
    return -4; // invalid step position

  seq_trg_layer_value[track][step_ix] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// sets value of assigned layers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_GateSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.gate;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_AccentSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.accent;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_RollSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.roll;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_GlideSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.glide;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_SkipSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.skip;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_RandomGateSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_gate;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_RandomValueSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_value;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}

s32 SEQ_TRG_NoFxSet(u8 track, u16 step, u8 trg_instrument, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.no_fx;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, trg_instrument, value) : -1;
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the string to a trigger assignment type
/////////////////////////////////////////////////////////////////////////////
char *SEQ_TRG_TypeStr(u8 trg_num)
{
  return (char *)seq_trg_names[trg_num];
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the string to assigned trigger(s) - if multiple 
// triggers are assigned, it returns "Multi" - if layer is not assigned
// to any trigger, it returns "-----"
/////////////////////////////////////////////////////////////////////////////
char *SEQ_TRG_AssignedTypeStr(u8 track, u8 trg_layer)
{
  u32 trg_assignments = seq_cc_trk[track].trg_assignments.ALL;
  u32 pattern = trg_layer+1;
  u32 mask = 0xf;
  int assigned = -1;
  int num = 0;

  int i;
  for(i=0; i<8; ++i) {
    if( (trg_assignments & mask) == pattern ) {
      assigned = i;
      ++num;
    }
    mask <<= 4;
    pattern <<= 4;
  }

  if( !num )
    return "-----";

  if( num > 1 )
    return "Multi";

  return (char *)seq_trg_names[assigned];
}
