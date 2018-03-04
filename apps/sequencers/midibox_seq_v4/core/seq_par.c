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
#ifndef AHB_SECTION
#define AHB_SECTION
#endif
u8 AHB_SECTION seq_par_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_PAR_MAX_BYTES];


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
  "Roll2", // 10
  "PrgCh", // 11
  "Nth1 ", // 12
  "Nth2 ", // 13
  "Chrd2", // 14
  "AfTch", // 15
  "Root ", // 16
  "Scale", // 17
  "Chrd3", // 18
};

static const u8 seq_par_map[SEQ_PAR_NUM_TYPES] = { // allows to change the order for the UI selection
  SEQ_PAR_Type_None,
  SEQ_PAR_Type_Note,
  SEQ_PAR_Type_Chord1,
  SEQ_PAR_Type_Chord2,
  SEQ_PAR_Type_Chord3,
  SEQ_PAR_Type_Velocity,
  SEQ_PAR_Type_Length,
  SEQ_PAR_Type_CC,
  SEQ_PAR_Type_PitchBend,
  SEQ_PAR_Type_Aftertouch,
  SEQ_PAR_Type_ProgramChange,
  SEQ_PAR_Type_Probability,
  SEQ_PAR_Type_Delay,
  SEQ_PAR_Type_Roll,
  SEQ_PAR_Type_Roll2,
  SEQ_PAR_Type_Nth1,
  SEQ_PAR_Type_Nth2,
  SEQ_PAR_Type_Root,
  SEQ_PAR_Type_Scale,
};

static const u8 seq_par_default_value[SEQ_PAR_NUM_TYPES] = {
  0x00, // None
  0x3c, // Note: C-3
  0x40, // Chord1: A/2
  100,  // Velocity
  71,   // Length
  0x80, // CC // NEW: overruled via seq_core_options.INIT_CC !!!
  64,   // PitchBender
  0,    // Probability (reversed: 100%)
  0,    // Delay
  0,    // Roll
  0,    // Roll2
  0x80, // PrgCh // NEW: disabled by default
  0,    // Nth1
  0,    // Nth2
  0x40, // Chord2: A/2
  0,    // Aftertouch: 0
  0,    // Root: C
  0,    // Scale: 0
  0x01, // Chord3: 1
};

static const u8 seq_par_max_value[SEQ_PAR_NUM_TYPES] = {
  0x7f, // None
  0x7f, // Note: C-3
  0x7f, // Chord1: A/2
  0x7f, // Velocity
  101,  // Length
  0x80, // CC
  0x80, // PitchBender
  0x7f, // Probability (reversed: 100%)
  0x7f, // Delay
  0x7f, // Roll
  0x7f, // Roll2
  0x80, // PrgCh
  0x7f, // Nth1
  0x7f, // Nth2
  0x7f, // Chord2
  0x80, // Aftertouch
  0x7f, // Root
  0x7f, // Scale
  0x7f, // Chord3
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Init(u32 mode)
{
#ifndef MBSEQV4L
  // init parameter layer values
  // Note: keep in sync with SEQ_TRG_Init() !!!
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    SEQ_PAR_TrackInit(track, 128, 8, 1); // track, steps, parameter layers, instruments
#else
  // extra for MBSEQ V4L:
  // G1T1/2/3 and G3T1/2/3 use 64 steps and 16 layers (for Notes)
  // remaining tracks use 256 steps and 4 layers (Pitchbender/CC) with 4 times speed
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    if( (track >= 0 && track <= 2) || (track >= 8 && track <= 10) )
      SEQ_PAR_TrackInit(track, 64, 16, 1); // track, steps, parameter layers, instruments
    else
      SEQ_PAR_TrackInit(track, 256, 4, 1); // track, steps, parameter layers, instruments
  }
#endif

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
    return (par_layer >= 4) ? SEQ_PAR_Type_None : tcc->par_assignment_drum[par_layer];
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
s32 SEQ_PAR_NoteGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_note) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which plays a chord
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_chord) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which controls velocity
// if not assigned to a layer, returns 100 as default velocity
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_VelocityGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  // note: due to performance reasons, similar code exists in SEQ_LAYER_GetEvents()

  if( (par_layer=tcc->link_par_layer_velocity) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 100; // default velocity
}


/////////////////////////////////////////////////////////////////////////////
// returns the first layer which controls note length
// returns 1..96 (96 for glide, 71 if no length assigned)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_LengthGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  // note: due to performance reasons, similar code exists in SEQ_LAYER_GetEvents()

  if( (par_layer=tcc->link_par_layer_length) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
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
s32 SEQ_PAR_ProbabilityGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_probability) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    u8 value = SEQ_PAR_Get(track, step, par_layer, par_instrument);
    return (value >= 100) ? 0 : 100-value;
  }

  return 100; // 100% probability
}


/////////////////////////////////////////////////////////////////////////////
// returns the step delay if assigned to any parameter layer
// Delay ranges from 0..95
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_StepDelayGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_delay) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    s32 value = SEQ_PAR_Get(track, step, par_layer, par_instrument);
    return (value > 95) ? 95 : value;
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
s32 SEQ_PAR_RollModeGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_roll) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no roll
}


/////////////////////////////////////////////////////////////////////////////
// returns the roll2 mode if assigned to any parameter layer
//   0:      none
//   1.. 31: 2x..
//  32.. 63: 3x..
//  64.. 95: 4x..
//  96..127: 5x..
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Roll2ModeGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_roll2) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no roll2
}


/////////////////////////////////////////////////////////////////////////////
// returns the Nth1 mode if assigned to any parameter layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Nth1ValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_nth1) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no nth1
}

/////////////////////////////////////////////////////////////////////////////
// returns the Nth2 mode if assigned to any parameter layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_Nth2ValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_nth2) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no nth2
}


/////////////////////////////////////////////////////////////////////////////
// returns the Root value if assigned to any parameter layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_RootValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_root) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no root
}

/////////////////////////////////////////////////////////////////////////////
// returns the Scale if assigned to any parameter layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_ScaleValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  s8 par_layer;

  if( (par_layer=tcc->link_par_layer_scale) >= 0 &&
      !(layer_muted & (1 << par_layer)) ) {
    return SEQ_PAR_Get(track, step, par_layer, par_instrument);
  }

  return 0; // no scale
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
// This function puts the string to the assigned parameter into str_buffer[6]
// (5 characters + terminator)
// if layer is not assigned, it returns "None "
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PAR_AssignedTypeStr(u8 track, u8 par_layer, u8 instrument, char *str_buffer)
{
  seq_par_layer_type_t asg = SEQ_PAR_AssignmentGet(track, par_layer);

  if( asg == SEQ_PAR_Type_CC ) {
    seq_cc_trk_t *tcc = &seq_cc_trk[track];
    u8 cc_number;
    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
#ifdef MBSEQV4P
      cc_number = seq_layer_drum_cc[instrument][par_layer];
#else
      cc_number = 0xff;
#endif
    } else {
      cc_number = SEQ_CC_Get(track, SEQ_CC_LAY_CONST_B1 + par_layer);
    }

    if( cc_number == 0xff ) {
      strcpy(str_buffer, "n/a  ");
    } else if( cc_number >= 0x80 ) {
      strcpy(str_buffer, "COff ");
    } else {
      sprintf(str_buffer, "#%03d ", cc_number);
    }
  } else {
    strncpy(str_buffer, SEQ_PAR_TypeStr(asg), 6);
  }
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns mapped parameter type
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_PAR_MappedTypeGet(u8 par_type)
{
  int i;
  u8 *seq_par_map_ptr = (u8 *)&seq_par_map[0];
  for(i=0; i<SEQ_PAR_NUM_TYPES; ++i) {
    if( *(seq_par_map_ptr++) == par_type )
      return i;
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Returns unmapped parameter type
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_PAR_UnMappedTypeGet(u8 mapped_par_type)
{
  return (mapped_par_type >= SEQ_PAR_NUM_TYPES) ? 0 : seq_par_map[mapped_par_type];
}

/////////////////////////////////////////////////////////////////////////////
// Returns initial value of given parameter assignment type
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_PAR_InitValueGet(seq_par_layer_type_t par_type, u8 par_layer)
{
  if( par_type >= SEQ_PAR_NUM_TYPES )
    return 0;

  if( par_layer > 0 && (par_type == SEQ_PAR_Type_Note || par_type == SEQ_PAR_Type_Chord1 || par_type == SEQ_PAR_Type_Chord2 || par_type == SEQ_PAR_Type_Chord3) )
    return 0x00; // Note/Chords are 0 by default if not in Layer A

  // new: variable init value
  if( par_type == SEQ_PAR_Type_CC )
    return seq_core_options.INIT_CC;

  return seq_par_default_value[par_type];
}


/////////////////////////////////////////////////////////////////////////////
// Returns maximum value of given parameter assignment type
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_PAR_MaxValueGet(seq_par_layer_type_t par_type)
{
  return seq_par_max_value[par_type];
}

