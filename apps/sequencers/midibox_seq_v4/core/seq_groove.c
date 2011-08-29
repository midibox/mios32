// $Id$
/*
 * Groove Functions
 *
 * TODO: customized groove templates stored on SD Card
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

#include "seq_groove.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// these special values insert the variable "intensity" parameter
#define VPOS  127
#define VNEG -128


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// preset grooves
const seq_groove_entry_t seq_groove_presets[SEQ_GROOVE_NUM_PRESETS] = {
  { " -- off --  ", // dummy entry, changes here have no effect!
    16, // Number of Steps
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "  Shuffle   ",
    16, // Number of Steps
    {    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Inv. Shuffle",
    16, // Number of Steps
    { VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "  Shuffle2  ",
    16, // Number of Steps
    {    0,   32,    0, VPOS,    0,   32,    0, VPOS,    0,   32,    0, VPOS,    0,   32,    0, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,   20,    0,  -20,    0,   20,    0,  -20,    0,   20,    0,  -20,    0,   20,    0,  -20 }, // Velocity
  },

  { "  Shuffle3  ",
    16, // Number of Steps
    {    0,   32,   16, VPOS,    0,   32,   16, VPOS,    0,   32,   16, VPOS,    0,   32,   16, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,  -10,  -20,  -10,    0,  -10,  -20,  -10,    0,  -10,  -20,  -10,    0,  -10,  -20,  -10 }, // Velocity
  },

  { "  Shuffle4  ",
    16, // Number of Steps
    {  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16 }, // Delay
    {    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG }, // Gatelength
    {    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS }, // Velocity
  },

  { "  Shuffle5  ",
    16, // Number of Steps
    {  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16 }, // Delay
    { VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG }, // Gatelength
    { VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS }, // Velocity
  }
};


// custom groove templates are read from MBSEQ_G.V4 file which is located on SD Card
seq_groove_entry_t seq_groove_templates[SEQ_GROOVE_NUM_TEMPLATES];

// here some useful presets
const seq_groove_entry_t seq_groove_templates_presets[SEQ_GROOVE_NUM_TEMPLATES] = {
  { "Custom #1   ", // first one should be "empty" (no groove)
    4, // Number of Steps
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #2   ",
    4, // Number of Steps
    {    0,    6,    0,    6,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #3   ",
    4, // Number of Steps
    {    0,   12,    0,   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #4   ",
    4, // Number of Steps
    {    0,   24,    0,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #5   ",
    4, // Number of Steps
    {    0,    0,    6,    6,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #6   ",
    4, // Number of Steps
    {    0,    0,   12,   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #7   ",
    4, // Number of Steps
    {    0,    0,   24,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #8   ",
    4, // Number of Steps
    {    0,   24,   24,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,  -12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #9   ",
    4, // Number of Steps
    {    0,    6,    0,    6,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,  -16,    0,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #10  ",
    4, // Number of Steps
    {    0,   12,    0,   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,  -16,    0,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #11  ",
    4, // Number of Steps
    {    0,   24,    0,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,  -16,    0,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #12  ",
    4, // Number of Steps
    {    0,    0,    6,    6,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,  -16,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #13  ",
    4, // Number of Steps
    {    0,    0,   12,   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,  -16,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #14  ",
    4, // Number of Steps
    {    0,    0,   24,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,  -16,   -8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #15  ",
    4, // Number of Steps
    {    0,   24,   24,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,  -16,   -8,  -16,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Custom #16  ",
    4, // Number of Steps
    {    0,   24,    6,   24,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,  -48,  -30,  -16,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Init(u32 mode)
{
  // initialise custom templates with presets
  // will be loaded from SD Card in SEQ_FILE_G
  memcpy(seq_groove_templates, seq_groove_templates_presets, sizeof(seq_groove_templates));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a groove
// Length: 12 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_GROOVE_NameGet(u8 groove)
{
  if( groove >= (SEQ_GROOVE_NUM_PRESETS+SEQ_GROOVE_NUM_TEMPLATES)  )
    return "Invld Groove";

  if( groove >= SEQ_GROOVE_NUM_PRESETS )
    return seq_groove_templates[groove-SEQ_GROOVE_NUM_PRESETS].name;

  return (char *)seq_groove_presets[groove].name;
}


/////////////////////////////////////////////////////////////////////////////
// returns 0..95 for the number of ticks at which the step should be delayed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_DelayGet(u8 track, u8 step)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 groove = tcc->groove_style;

  // check if within allowed range
  if( !groove || groove >= (SEQ_GROOVE_NUM_PRESETS+SEQ_GROOVE_NUM_TEMPLATES) )
    return 0; // no groove

  seq_groove_entry_t *g;
  if( groove >= SEQ_GROOVE_NUM_PRESETS )
    g = (seq_groove_entry_t *)&seq_groove_templates[groove-SEQ_GROOVE_NUM_PRESETS];
  else
    g = (seq_groove_entry_t *)&seq_groove_presets[groove];

  step %= g->num_steps;

  // get delay value
  s8 delay = g->add_step_delay[step];

  // insert positive/negative intensity
  if( delay == VPOS )
    delay = tcc->groove_value;
  else if( delay == VNEG )
    delay = -tcc->groove_value;

  return delay;
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected groove
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 groove = tcc->groove_style;

  // check if within allowed range
  if( !groove || groove >= (SEQ_GROOVE_NUM_PRESETS+SEQ_GROOVE_NUM_TEMPLATES) )
    return 0; // no groove

  seq_groove_entry_t *g;
  if( groove >= SEQ_GROOVE_NUM_PRESETS )
    g = (seq_groove_entry_t *)&seq_groove_templates[groove-SEQ_GROOVE_NUM_PRESETS];
  else
    g = (seq_groove_entry_t *)&seq_groove_presets[groove];

  step %= g->num_steps;

  // get velocity modifier
  s8 add_velocity = g->add_step_velocity[step];
  // insert positive/negative intensity
  if( add_velocity == VPOS )
    add_velocity = tcc->groove_value;
  else if( add_velocity == VNEG )
    add_velocity = -tcc->groove_value;

  if( add_velocity ) {
    s16 value = e->midi_package.velocity + add_velocity;
    if( value < 1 )
      value = 1;
    else if( value > 127 )
      value = 127;
    e->midi_package.velocity = value;
  }

  // get gatelength modifier if len < 96 (glide not active)
  if( e->len < 96 ) {
    s8 add_length = g->add_step_length[step];
    // insert positive/negative intensity
    if( add_length == VPOS )
      add_length = tcc->groove_value;
    else if( add_length == VNEG )
      add_length = -tcc->groove_value;

    if( add_length ) {
      s16 value = e->len + add_length;
      if( value < 1 )
	value = 1;
      else if( value > 95 )
	value = 95;
      e->len = value;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clears a groove template
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Clear(u8 groove)
{
  if( groove < SEQ_GROOVE_NUM_PRESETS )
    return -1; // preset not editable

  s32 groove_template = groove - SEQ_GROOVE_NUM_PRESETS; // negative if not a custom template

  memcpy(&seq_groove_templates[groove_template], &seq_groove_presets[0], sizeof(seq_groove_entry_t));
  sprintf(seq_groove_templates[groove_template].name, "Custom #%d   ", groove_template+1);
  seq_groove_templates[groove_template].name[12] = 0; // terminator
  seq_groove_templates[groove_template].num_steps = 2; // for fast success
    
  return 0; // no error
}
