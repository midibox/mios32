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

#include "seq_par.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 par_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_PAR_NUM_LAYERS][SEQ_CORE_NUM_STEPS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Init(u32 mode)
{
  int track, step;

  // init parameter layer values
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    for(step=0; step<SEQ_CORE_NUM_STEPS; ++step) {
      par_layer_value[track][0][step] = 0x3c; // note C-3
      par_layer_value[track][1][step] = 100;  // velocity
      par_layer_value[track][2][step] = 17;   // gatelength 75%
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets a value of parameter layer
// (using this interface function to allow dynamic lists in future)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Set(u8 track, u8 p_layer, u8 step, u8 value)
{
  par_layer_value[track][p_layer][step] = value;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns a value of parameter layer
// (using this interface function to allow dynamic lists in future)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Get(u8 track, u8 p_layer, u8 step)
{
  return par_layer_value[track][p_layer][step];
}
