// $Id$
/*
 * New Humanize Functions by Borfo
 *
 * ==========================================================================
 *
 *  Copyright (C) 2015 Borfo
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "seq_humanize2.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_trg.h"
#include "seq_random.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HUMANIZE2_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected humanize intensity
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HUMANIZE2_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 humprob, humvel, humlen, humnote, humoct, humskip;
  humprob = tcc->humanize2_probability;
  humvel = tcc->humanize2_vel;
  humlen = tcc->humanize2_len;
  humnote = tcc->humanize2_note;
  humoct = tcc->humanize2_oct;
  humskip = tcc->humanize2_skip_probability;
  
  // check if random value trigger set
  if( SEQ_TRG_RandomValueGet(track, step, 0) ) {//RK - I broke this.
//    mode = 0x01; // Note Only
//    intensity = 4*24; // +/- 1 octave
  } else {

    if( !humvel && !humlen && !humnote && !humoct && !humskip )
      return 0; // nothing to do

  }

  if( ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->humanize2_note_probability * humprob ) && humnote ) { // Note Event
    u8 note_intensity = humnote * 2;
    s16 value = e->midi_package.note + ((note_intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, note_intensity));

    // ensure that note is in the 0..127 range
    value = SEQ_CORE_TrimNote(value, 0, 127);

    e->midi_package.note = value;
  }

  if( ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->humanize2_oct_probability * humprob ) && humoct ) { // Octave Event
    u8 oct_intensity = humoct * 2;
    s16 value = e->midi_package.note + (((oct_intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, oct_intensity))*12);

    // ensure that note is in the 0..127 range
    value = SEQ_CORE_TrimNote(value, 0, 127);

    e->midi_package.note = value;
  }

  if( ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->humanize2_vel_probability * humprob ) && humvel ) { // Velocity
	s16 randnum = SEQ_RANDOM_Gen_Range( 0 , humvel * 2) - humvel;
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

  if( ( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->humanize2_len_probability * humprob ) && e->len < 96 && humlen ) { // Gatelength (only if no glide)
    s16 value = e->len + ((humlen/2) - (s16)SEQ_RANDOM_Gen_Range(0, humlen));
    if( value < 1 )
      value = 1;
    else if( value > 95 )
      value = 95;
    e->len = value;
  }

  if( (s16)SEQ_RANDOM_Gen_Range(0, 31 * 31) <= tcc->humanize2_skip_probability * humprob ) { // Skip note
	//play with zero velocity
    e->midi_package.velocity = 0;
  }


  // Delay as well?

  return 0; // no error
}

