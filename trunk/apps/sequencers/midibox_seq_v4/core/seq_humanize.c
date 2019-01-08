// $Id$
/*
 * Humanize Functions
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

#include "seq_humanize.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_trg.h"
#include "seq_random.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HUMANIZE_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected humanize intensity
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_HUMANIZE_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  u8 mode, intensity;

  // check if random value trigger set
  if( SEQ_TRG_RandomValueGet(track, step, 0) ) {
    mode = 0x01; // Note Only
    intensity = 4*24; // +/- 1 octave
  } else {
    mode = tcc->humanize_mode;
    if( !mode )
      return 0; // nothing to do

    intensity = tcc->humanize_value;
    if( !intensity )
      return 0; // nothing to do
  }

  if( mode & (1 << 0) ) { // Note Event
    u8 note_intensity = intensity / 4; // important for combination with velocity/gatelength
    s16 value = e->midi_package.note + ((note_intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, note_intensity));

    // ensure that note is in the 0..127 range
    value = SEQ_CORE_TrimNote(value, 0, 127);

    e->midi_package.note = value;
  }

  if( mode & (1 << 1) ) { // Velocity
    s16 value = e->midi_package.velocity + ((intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, intensity));
    if( value < 1 )
      value = 1;
    else if( value > 127 )
      value = 127;
    e->midi_package.velocity = value;
  }

  if( e->len < 96 && (mode & (1 << 2)) ) { // Gatelength (only if no glide)
    s16 value = e->len + ((intensity/2) - (s16)SEQ_RANDOM_Gen_Range(0, intensity));
    if( value < 1 )
      value = 1;
    else if( value > 95 )
      value = 95;
    e->len = value;
  }

  // Delay as well?

  return 0; // no error
}

