// $Id: seq_lfo.c 1787 2013-05-19 15:23:20Z tk $
/*
 * LFO Functions
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

#include "seq_lfo.h"
#include "seq_core.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16 step_ctr;
  u16 pos;
} seq_lfo_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_lfo_t seq_lfo[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LFO_ValueGet(seq_cc_trk_t *tcc, seq_lfo_t *lfo);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LFO_Init(u32 mode)
{
  u8 track;

  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    SEQ_LFO_ResetTrk(track);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the LFO of a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LFO_ResetTrk(u8 track)
{
  seq_lfo_t *lfo = &seq_lfo[track];

  lfo->step_ctr = 0;
  lfo->pos = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Handles the LFO of a given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LFO_HandleTrk(u8 track, u32 bpm_tick)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  seq_lfo_t *lfo = &seq_lfo[track];

  // Note: we always have to process this part for each track, otherwise the LFO
  // could get out-of-sync if temporary disabled/enabled

  // increment step counter on each step
  if( (bpm_tick % 96) == 0 && lfo->step_ctr != 65535) // @384 ppqn (reference bpm_tick resolution)
    ++lfo->step_ctr;


  // increment waveform position
  if( lfo->step_ctr > tcc->lfo_steps_rst ) {
    if( tcc->lfo_enable_flags.ONE_SHOT ) {
      // oneshot mode: halt LFO counter
      lfo->step_ctr = 65535;
      lfo->pos = 65535;
    } else {
      // reset step counter and LFO position
      lfo->step_ctr = 0;
      lfo->pos = tcc->lfo_phase * 655; // possible phase offset: 0%..99%
    }
  } else {
    // increment waveform pointer
    u32 lfo_ticks = (u32)(tcc->lfo_steps+1) * 96; // @384 ppqn (reference bpm_tick resolution)
    u32 inc = 65536 / lfo_ticks;
    lfo->pos += inc;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on LFO settings
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LFO_Event(u8 track, seq_layer_evnt_t *e)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( !tcc->lfo_waveform )
    return 0; // LFO disabled

  seq_lfo_t *lfo = &seq_lfo[track];
  s32 lfo_value = SEQ_LFO_ValueGet(tcc, lfo);

  // apply
  if( e->midi_package.type == NoteOn ) {
    if( tcc->lfo_enable_flags.NOTE ) {
      s16 value = e->midi_package.note + lfo_value;

      // ensure that note is in the 0..127 range
      value = SEQ_CORE_TrimNote(value, 0, 127);

      e->midi_package.note = value;
    }

    if( tcc->lfo_enable_flags.VELOCITY && e->midi_package.velocity ) {
      s16 value = e->midi_package.velocity + lfo_value;
      if( value < 1 )
	value = 1;
      else if( value > 127 )
	value = 127;
      e->midi_package.velocity = value;
    }

    if( tcc->lfo_enable_flags.LENGTH ) {
      s16 value = e->len + lfo_value;
      if( value < 1 )
	value = 1;
      else if( value > 95 )
	value = 95;
      e->len = value;
    }
  } else if( e->midi_package.type == CC ) {
    if( tcc->lfo_enable_flags.CC ) {
      s16 value = e->midi_package.value + lfo_value;
      if( value < 0 )
	value = 0;
      else if( value > 127 )
	value = 127;
      e->midi_package.value = value;
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Returns a fast modulated CC event if return value >= 1
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LFO_FastCC_Event(u8 track, u32 bpm_tick, mios32_midi_package_t *p, u8 ignore_waveform)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( !ignore_waveform && !tcc->lfo_waveform )
    return 0; // LFO disabled

  if( !tcc->lfo_cc )
    return 0; // CC disabled

  if( tcc->lfo_cc_ppqn < 8 ) { // not 384 ppqn
    if( tcc->lfo_cc_ppqn == 0 ) { // 1 ppqn
      if( (bpm_tick % 384) != 0 )
	return 0; // skipped due to resolution
    } else {
      int resolution = (3 << (7-tcc->lfo_cc_ppqn));
      if( (bpm_tick % resolution) != 0 )
	return 0; // skipped due to resolution
    }
  }

  seq_lfo_t *lfo = &seq_lfo[track];
  s32 lfo_value = SEQ_LFO_ValueGet(tcc, lfo) + tcc->lfo_cc_offset;

  if( lfo_value < 0 )
    lfo_value = 0;
  else if( lfo_value > 127 )
    lfo_value = 127;

  p->type      = CC;
  p->cable     = track;
  p->event     = CC;
  p->chn       = tcc->midi_chn;
  p->cc_number = tcc->lfo_cc;
  p->value     = lfo_value;

  return 1; // returned 1 event
}


/////////////////////////////////////////////////////////////////////////////
// This help function returns the LFO value depending on selected waveform
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LFO_ValueGet(seq_cc_trk_t *tcc, seq_lfo_t *lfo)
{
  s32 lfo_value = (int)lfo->pos;
  switch( tcc->lfo_waveform ) {
    case SEQ_LFO_WAVEFORM_Off:
      return 0;

    case SEQ_LFO_WAVEFORM_Sine: { // currently no real sine!
      s32 tmp = 4*(lfo_value % 32768);
      if( tmp >= 65536 )
	tmp = 65536 - (tmp % 65536);
      lfo_value = (lfo_value >= 32768) ? -tmp : tmp;
    } break;

    case SEQ_LFO_WAVEFORM_Triangle: {
      s32 tmp = 2*(lfo_value % 32768);
      lfo_value = (lfo_value >= 32768) ? (65535-tmp) : tmp;
    } break;

    case SEQ_LFO_WAVEFORM_Saw:
      // no modification required
      break;

    default: {// SEQ_LFO_WAVEFORM_Rec05..SEQ_LFO_WAVEFORM_Rec95
      u16 trans = 5 * 655 * (tcc->lfo_waveform-SEQ_LFO_WAVEFORM_Rec05+1);
      lfo_value = (lfo_value >= trans) ? 0 : 65535;
    }
  }

  // scale down to +/- 7bit
  lfo_value = (lfo_value * (tcc->lfo_amplitude-128)) / 65536;

  return lfo_value;
}

