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

#include "seq_groove.h"
#include "seq_cc.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  char name[13];
  s8   add_step_delay[16];
  s8   add_step_length[16];
  s8   add_step_velocity[16];
} seq_groove_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// these special values insert the variable "intensity" parameter
#define VPOS  127
#define VNEG -128


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const seq_groove_entry_t seq_groove_table[] = {
  { " -- off --  ", // dummy entry, changes here have no effect!
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "  Shuffle   ",
    { VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "Inv. Shuffle",
    { VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Velocity
  },

  { "  Shuffle2  ",
    {    0,   32,    0, VPOS,    0,   32,    0, VPOS,    0,   32,    0, VPOS,    0,   32,    0, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,   20,    0,  -20,    0,   20,    0,  -20,    0,   20,    0,  -20,    0,   20,    0,  -20 }, // Velocity
  },

  { "  Shuffle3  ",
    {    0,   32,   16, VPOS,    0,   32,   16, VPOS,    0,   32,   16, VPOS,    0,   32,   16, VPOS }, // Delay
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, // Gatelength
    {    0,  -10,  -20,  -10,    0,  -10,  -20,  -10,    0,  -10,  -20,  -10,    0,  -10,  -20,  -10 }, // Velocity
  },

  { "  Shuffle4  ",
    {  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16 }, // Delay
    {    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG,    0, VNEG }, // Gatelength
    {    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS,    0, VPOS }, // Velocity
  },

  { "  Shuffle5  ",
    {  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16,  -16,   16 }, // Delay
    { VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG }, // Gatelength
    { VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS, VNEG, VPOS }, // Velocity
  }
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Init(u32 mode)
{
  // here we will read customized groove pattern later

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available grooves
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_NumGet(void)
{
  return sizeof(seq_groove_table)/sizeof(seq_groove_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a groove
// Length: 12 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_GROOVE_NameGet(u8 groove)
{
  if( groove >= SEQ_GROOVE_NumGet() )
    return "Invld Groove";

  return seq_groove_table[groove].name;
}


/////////////////////////////////////////////////////////////////////////////
// returns 0..95 for the number of ticks at which the step should be delayed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_DelayGet(u8 track, u8 step)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 groove = tcc->groove_style;

  // check if within allowed range
  if( !groove || groove >= SEQ_GROOVE_NumGet() )
    return 0; // no groove

  // 16 steps are supported
  step %= 16;

  // get delay value
  s8 delay = seq_groove_table[groove].add_step_delay[step];

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
  if( !groove || groove >= SEQ_GROOVE_NumGet() )
    return 0; // no groove

  // 16 steps are supported
  step %= 16;

  // get velocity modifier
  s8 add_velocity = seq_groove_table[groove].add_step_velocity[step];
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
    s8 add_length = seq_groove_table[groove].add_step_length[step];
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

