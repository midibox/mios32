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

#include "seq_trg.h"
#include "seq_core.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_B, remaining functions should
// use SEQ_TRG_Get/Set
u8 trg_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_TRG_NUM_LAYERS][SEQ_CORE_NUM_STEPS/8];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char seq_trg_names[8][6] = {
  "Gate ", // 0
  "Skip ", // 1
  "Acc. ", // 2
  "Glide", // 3
  " Roll", // 4
  " R.G ", // 5
  " R.V ", // 6
  "No Fx"  // 7
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Init(u32 mode)
{
  int i, track;

  // init parameter layer values
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    // init trigger layer values
    for(i=0; i<SEQ_CORE_NUM_STEPS/8; ++i) {
      trg_layer_value[track][0][i] = track == 0 ? 0x11 : 0x00; // gate
      trg_layer_value[track][1][i] = 0x00; // accent
      trg_layer_value[track][2][i] = 0x00; // roll
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns the assignment of a certain trigger
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_AssignmentGet(u8 track, u8 trg_num)
{
  return (seq_cc_trk[track].trg_assignments.ALL >> (trg_num*2)) & 3;
}


/////////////////////////////////////////////////////////////////////////////
// returns value of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Get(u8 track, u8 step, u8 trg_layer)
{
  u8 step_mask = 1 << (step % 8);
  u8 step_ix = step / 8;
  return (trg_layer_value[track][trg_layer][step_ix] & step_mask) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 8 steps of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Get8(u8 track, u8 step8, u8 trg_layer)
{
  return trg_layer_value[track][trg_layer][step8];
}


/////////////////////////////////////////////////////////////////////////////
// returns value of assigned layers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_GateGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.gate;
  // gate always set if not assigned
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 1;
}

s32 SEQ_TRG_SkipGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.skip;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_AccentGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.accent;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_GlideGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.glide;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_RollGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.roll;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_RandomGateGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_gate;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_RandomValueGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_value;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}

s32 SEQ_TRG_NoFxGet(u8 track, u8 step)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.no_fx;
  return trg_assignment ? SEQ_TRG_Get(track, step, trg_assignment-1) : 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets value of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Set(u8 track, u8 step, u8 trg_layer, u8 value)
{
  u8 step_mask = 1 << (step % 8);
  u8 step_ix = step / 8;

  if( value )
    trg_layer_value[track][trg_layer][step_ix] |= step_mask;
  else
    trg_layer_value[track][trg_layer][step_ix] &= ~step_mask;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// sets 8 steps of a given trigger layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_Set8(u8 track, u8 step8, u8 trg_layer, u8 value)
{
  trg_layer_value[track][trg_layer][step8] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// sets value of assigned layers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TRG_GateSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.gate;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_SkipSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.skip;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_AccentSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.accent;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_GlideSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.glide;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_RollSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.roll;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_RandomGateSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_gate;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_RandomValueSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.random_value;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
}

s32 SEQ_TRG_NoFxSet(u8 track, u8 step, u8 value)
{
  u8 trg_assignment = seq_cc_trk[track].trg_assignments.no_fx;
  return trg_assignment ? SEQ_TRG_Set(track, step, trg_assignment-1, value) : -1;
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
  u16 trg_assignments = seq_cc_trk[track].trg_assignments.ALL;
  u16 pattern = trg_layer+1;
  u16 mask = 3;
  int assigned = -1;
  int num = 0;

  int i;
  for(i=0; i<8; ++i) {
    if( (trg_assignments & mask) == pattern ) {
      assigned = i;
      ++num;
    }
    mask <<= 2;
    pattern <<= 2;
  }

  if( !num )
    return "-----";

  if( num > 1 )
    return "Multi";

  return (char *)seq_trg_names[assigned];
}
