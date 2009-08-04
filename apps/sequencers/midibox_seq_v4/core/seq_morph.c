// $Id$
/*
 * Morph Functions
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

#include "seq_morph.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 morph_value;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static u8 SEQ_MORPH_ScaleValue(u8 value, u8 min, u8 max);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MORPH_Init(u32 mode)
{
  morph_value = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns morph value (0..127)
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_MORPH_ValueGet(void)
{
  return morph_value;
}

/////////////////////////////////////////////////////////////////////////////
// sets the morph value (0..127)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MORPH_ValueSet(u8 value)
{
  if( value >= 128 )
    return -1; // invalid range

  morph_value = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Modifies a Note event according to morph values
// layer_* value point to the parameter layers from which parts of the 
// note event are taken. If -1, the appr. parameter won't be touched.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MORPH_EventNote(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 layer_note, s8 layer_velocity, s8 layer_length)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // check if morphing enabled
  if( !tcc->morph_mode )
    return 0; // no morphing
  // currently only a single mode is supported...

  u16 morph_step = step + tcc->morph_dst;
  if( morph_step > 255 ) // aligned with display output in seq_ui_trkmorph.c
    morph_step = 255;

  // morph values
  mios32_midi_package_t *p = &e->midi_package; 
  if( layer_note >= 0 )
    p->note = SEQ_MORPH_ScaleValue(morph_value, p->note, SEQ_PAR_Get(track, morph_step, layer_note, instrument));
  if( layer_velocity >= 0 )
    p->velocity = SEQ_MORPH_ScaleValue(morph_value, p->velocity, SEQ_PAR_Get(track, morph_step, layer_velocity, instrument));
  if( layer_length >= 0 )
    e->len = SEQ_MORPH_ScaleValue(morph_value, e->len, SEQ_PAR_Get(track, morph_step, layer_length, instrument));
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Modifies a CC event according to morph values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MORPH_EventCC(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // check if morphing enabled
  if( !tcc->morph_mode )
    return 0; // no morphing
  // currently only a single mode is supported...

  u16 morph_step = step + tcc->morph_dst;
  if( morph_step > 255 ) // aligned with display output in seq_ui_trkmorph.c
    morph_step = 255;

  // morph value
  e->midi_package.value = SEQ_MORPH_ScaleValue(morph_value, e->midi_package.value, SEQ_PAR_Get(track, morph_step, par_layer, instrument));
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Modifies a PitchBend event according to morph values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MORPH_EventPitchBend(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // check if morphing enabled
  if( !tcc->morph_mode )
    return 0; // no morphing
  // currently only a single mode is supported...

  u16 morph_step = step + tcc->morph_dst;
  if( morph_step > 255 ) // aligned with display output in seq_ui_trkmorph.c
    morph_step = 255;

  // morph value
  e->midi_package.evnt2 = SEQ_MORPH_ScaleValue(morph_value, e->midi_package.evnt2, SEQ_PAR_Get(track, morph_step, par_layer, instrument));
  e->midi_package.evnt1 = e->midi_package.evnt2; // LSB (TODO: check if re-using the MSB is useful)

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to scale a value between min and max boundary
/////////////////////////////////////////////////////////////////////////////
static u8 SEQ_MORPH_ScaleValue(u8 value, u8 min, u8 max)
{
  // values equal? -> no scaling required
  if( min == max )
    return min;

  // swap min/max if reversed
  if( min > max ) {
    u8 tmp;
    tmp = min;
    min = max;
    max = tmp;

    // return scaled value
    u16 range = (max-min+1);
    return max - (u8)(((u16)value * (u16)range)/128);
  } else {
    // return scaled value
    u16 range = (max-min+1);
    return min + (u8)(((u16)value * (u16)range)/128);
  }
}

