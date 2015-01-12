// $Id$
/*
 * New Robotize Functions by Borfo (Rob K.)
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

#include "seq_robotize.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_trg.h"
#include "seq_random.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_ROBOTIZE_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected robotize parameters
/////////////////////////////////////////////////////////////////////////////
seq_robotize_flags_t SEQ_ROBOTIZE_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_robotize_flags_t returnflags;
  returnflags.ALL = 0;//NOFX, +Echo, +Sustain and +duplicate flags
  
  s16 testvar;
  
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  //exit if there's no chance of robotizing anything
  if( ! tcc->robotize_active || tcc->robotize_probability == 0 || ( !tcc->robotize_vel && !tcc->robotize_len && !tcc->robotize_note && !tcc->robotize_oct && !tcc->robotize_skip_probability && !tcc->robotize_sustain_probability && !tcc->robotize_nofx_probability && !tcc->robotize_echo_probability && !tcc->robotize_duplicate_probability ) )
    return returnflags; // nothing to do
		
  //check robotize mask - does the current step % 16 point to an active step in the robotize mask?
  u8 cursixteenth = step % 16;
  
  //assemble robotize mask - it's split into two bytes because that's what the CCs store
  u16 robotize_mask = (SEQ_CC_Get(track, SEQ_CC_ROBOTIZE_MASK2) << 8) | SEQ_CC_Get(track, SEQ_CC_ROBOTIZE_MASK1);
	
  if ( !((robotize_mask) & (1<<cursixteenth)) ) { //is the current step active in the robotize mask?  Check the bit with ((robotize_mask) & (1<<cursixteenth))
		return returnflags; // nothing to do, step not active in mask
	}

  // NOTE ROBOTIZATION - jump semitones
  testvar = tcc->robotize_note_probability * tcc->robotize_probability;
  if( tcc->robotize_note && testvar > 0 && ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= testvar ) ) { // Note Event
    u8 note_intensity = tcc->robotize_note * 2;
    s16 value = e->midi_package.note + ((note_intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, note_intensity));

    // ensure that note is in the 0..127 range
    value = SEQ_CORE_TrimNote(value, 0, 127);

    e->midi_package.note = value;
  }


  // OCTAVE ROBOTIZATION - jump octaves, cumulative with note jumps
  testvar = tcc->robotize_oct_probability * tcc->robotize_probability;
  if( tcc->robotize_oct  && testvar > 0 && ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= testvar ) ) { // Octave Event
    u8 oct_intensity = tcc->robotize_oct * 2;
    s16 value = e->midi_package.note + (((oct_intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, oct_intensity))*12);

    // ensure that note is in the 0..127 range
    value = SEQ_CORE_TrimNote(value, 0, 127);

    e->midi_package.note = value;
  }


  // VELOCITY ROBOTIZATION
  testvar = tcc->robotize_vel_probability * tcc->robotize_probability;
  if( tcc->robotize_vel && testvar > 0 && ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= testvar )  ) { // Velocity
	s16 randnum = SEQ_RANDOM_Gen_Range( 0 , tcc->robotize_vel * 2) - tcc->robotize_vel;
	s16 value = randnum + e->midi_package.velocity;

	//weight randnum if out of range in a superconvoluted way because I'm terrible at math.
	if (value > 127) {value = e->midi_package.velocity + (((127-e->midi_package.velocity)/127)*randnum);}
	if (value < 0) {value = e->midi_package.velocity + ((e->midi_package.velocity/127)*randnum);}

    if( value < 0 )
      value = 0;
    else if( value > 127 )
      value = 127;
    e->midi_package.velocity = value;
  }


  // GATELENGTH ROBOTIZATION
  testvar = tcc->robotize_len_probability * tcc->robotize_probability;
  if( tcc->robotize_len  && testvar > 0 && ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= testvar ) ) { // Gatelength (only if no glide)
    s16 value = e->len + ((tcc->robotize_len/2) - (s16)SEQ_RANDOM_Gen_Range(0, tcc->robotize_len));
    if( value < 1 )
      value = 1;
    else if( value > 95 )
      value = 95;
    e->len = value;
  }


  // NOTE SKIPPING ROBOTIZATION - plays with zero velocity if skipped
  if( tcc->robotize_skip_probability && (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->robotize_skip_probability * tcc->robotize_probability ) { // Skip note
	//play with zero velocity
    e->midi_package.velocity = 0;
  }

  // SUSTAIN ROBOTIZATION - adds sustain to note
  if( tcc->robotize_sustain_probability && (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->robotize_sustain_probability * tcc->robotize_probability ) { // Skip note
	//set sustain flag
    returnflags.SUSTAIN = 1;
  }

  // NOFX ROBOTIZATION - cancels any subsequent FX that are set on the note (eg: will play this note without echo even if echo is on)
  if( tcc->robotize_nofx_probability && (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->robotize_nofx_probability * tcc->robotize_probability ) { // Skip note
	//set NOFX flag
    returnflags.NOFX = 1;
  }

  // +ECHO ROBOTIZATION - Adds echo to just this note, using the current echo settings (even if echo FX is off)
  if( tcc->robotize_echo_probability && (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->robotize_echo_probability * tcc->robotize_probability ) { // Skip note
	//set +ECHO flag
    returnflags.ECHO = 1;
  }

  // +DUPLICATION ROBOTIZATION - Adds duplication to just this note, using the current FX Duplication settings (even if duplication FX is off)
  if( tcc->robotize_duplicate_probability && (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->robotize_duplicate_probability * tcc->robotize_probability ) { // Skip note
	//set +DUPLICATE flag
    returnflags.DUPLICATE = 1;
  }

  return returnflags;
}

