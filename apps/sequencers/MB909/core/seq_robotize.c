// $Id: seq_robotize.c 2124 2015-01-17 01:55:58Z borfo $
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
// checks robotizing probabilities - overall probability is multiplied with individual probability
// returns true if the item should be robotized, or false otherwise 
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_ROBOTIZE_Check_Probabilities(u8 prob1, u8 prob2, u32 randoms[], u8 *big_randoms_used)
{
	u8 returnbit = 0;
	if( prob1 && prob2 ) { //robotize event is possible
 	  u8 randindex = (*big_randoms_used) / 3;
	  if ((u8)( (*big_randoms_used) % 3 ) == 0){
		  randoms[randindex] = SEQ_RANDOM_Gen(0); // get fresh random number if needed
	  }
	  
	  u16 testvar = ( prob1 + 1) * ( prob2 + 1); // multiply the two probabilities together to get 10 bit probability number
	  u16 big_random = ( (randoms[randindex]) >> ( ( (*big_randoms_used) % 3 ) * 10 ) ) & ( ( 1 << 10 ) - 1 ) ; // extract a new big random number from the longer 32 bit random number
	  *big_randoms_used += 1; // increment counter
	  if ( big_random <= testvar ) returnbit = 1; // we're a go for robotizing!
	}
	return returnbit;
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected robotize parameters
/////////////////////////////////////////////////////////////////////////////
seq_robotize_flags_t SEQ_ROBOTIZE_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_robotize_flags_t returnflags;
  returnflags.ALL = 0;//NOFX, +Echo, +Sustain and +duplicate flags
  
  
  
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  //exit if there's no chance of robotizing anything
  if( ! tcc->robotize_active || tcc->robotize_probability == 0 ) // other checks - not necessary in most cases || ( !tcc->robotize_vel && !tcc->robotize_len && !tcc->robotize_note && !tcc->robotize_oct && !tcc->robotize_skip_probability && !tcc->robotize_sustain_probability && !tcc->robotize_nofx_probability && !tcc->robotize_echo_probability && !tcc->robotize_duplicate_probability ) )
    return returnflags; // nothing to do
		
  //check robotize mask - does the current step % 16 point to an active step in the robotize mask?
  u8 cursixteenth = step % 16;
  
  //assemble robotize mask - it's split into two bytes because that's what the CCs store
  u16 robotize_mask = (SEQ_CC_Get(track, SEQ_CC_ROBOTIZE_MASK2) << 8) | SEQ_CC_Get(track, SEQ_CC_ROBOTIZE_MASK1);
	
  if ( !((robotize_mask) & (1<<cursixteenth)) ) { //is the current step active in the robotize mask?  Check the bit with ((robotize_mask) & (1<<cursixteenth))
		return returnflags; // nothing to do, step not active in mask
	}



//initialize random number array, counter, and range variables
u32 randoms[3] = {0} ; //array of 32 bit random numbers.  Only fills if and as needed, to minimize calls to seq_random_gen.  Use array rather than overwriting the same var, so that the old rands are kept in case we want to reuse them in future improvements to the robotizer
u8 big_randoms_used = 0; //keeps track of how many 10 bit random numbers we've pulled so far
u8 range = 0; //range - reused in several of the robotizers

/*
if ( tcc->robotize_X && tcc->robotize_X_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_X_probability, randoms, &big_randoms_used) ) {
//ROBOTIZING CODE GOES HERE
}

//debug with
MIOS32_MIDI_SendDebugMessage("random: %d ", randoms[0]);
* MIOS32_MIDI_SendDebugString("string")
*/

	//NOTE ROBOTIZER - shift by semitones
	if ( tcc->robotize_note && tcc->robotize_note_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_note_probability, randoms, &big_randoms_used) ) {
		range = tcc->robotize_note * 2;
		s16 value = e->midi_package.note + ((range/2) - (s16)SEQ_RANDOM_Gen_Range(0, range));

		// ensure that note is in the 0..127 range
		value = SEQ_CORE_TrimNote(value, 0, 127);

		e->midi_package.note = value;
	}
		

	//OCTAVE ROBOTIZER - shift by octaves - cumulative with notes
	if ( tcc->robotize_oct && tcc->robotize_oct_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_oct_probability, randoms, &big_randoms_used) ) {
		range = tcc->robotize_oct * 2;
		s16 value = e->midi_package.note + (((range/2) - (s16)SEQ_RANDOM_Gen_Range(0, range))*12);

		// ensure that note is in the 0..127 range
		value = SEQ_CORE_TrimNote(value, 0, 127);

		e->midi_package.note = value;
	}


	//VELOCITY ROBOTIZER - change note velocity
	if ( tcc->robotize_vel && tcc->robotize_vel_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_vel_probability, randoms, &big_randoms_used) ) {
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


	//GATELENGTH ROBOTIZER - change note duration
	if ( tcc->robotize_len && tcc->robotize_len_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_len_probability, randoms, &big_randoms_used) ) {
		s16 value = e->len + ((tcc->robotize_len/2) - (s16)SEQ_RANDOM_Gen_Range(0, tcc->robotize_len));
		if( value < 1 )
		  value = 1;
		else if( value > 95 )
		  value = 95;
		e->len = value;
	}


	// NOTE SKIP ROBOTIZER
	if ( tcc->robotize_skip_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_skip_probability, randoms, &big_randoms_used) ) {
		e->midi_package.velocity = 0;// play with zero velocity
	}


	// SUSTAIN ROBOTIZER
	if ( tcc->robotize_sustain_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_sustain_probability, randoms, &big_randoms_used) ) {
		//set sustain flag
		returnflags.SUSTAIN = 1;
	}


	// NOFX ROBOTIZER
	if ( tcc->robotize_nofx_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_nofx_probability, randoms, &big_randoms_used) ) {
		//set NOFX flag
		returnflags.NOFX = 1;
	}


	// +ECHO ROBOTIZER
	if ( tcc->robotize_echo_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_echo_probability, randoms, &big_randoms_used) ) {
		//set +ECHO flag
		returnflags.ECHO = 1;
	}


	// +DUPLICATE ROBOTIZER
	if ( tcc->robotize_duplicate_probability && SEQ_ROBOTIZE_Check_Probabilities( tcc->robotize_probability, tcc->robotize_duplicate_probability, randoms, &big_randoms_used) ) {
		//set +DUPLICATE flag
		returnflags.DUPLICATE = 1;
	}


  return returnflags;
}

