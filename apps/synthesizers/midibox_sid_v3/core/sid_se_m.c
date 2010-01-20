// $Id$
/*
 * MBSID Multi Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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

#include <sid.h>

#include "sid_se.h"
#include "sid_se_m.h"
#include "sid_patch.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_M_Update(u8 sid)
{
  int i;


  ///////////////////////////////////////////////////////////////////////////
  // Clear all modulation destinations
  ///////////////////////////////////////////////////////////////////////////

  s32 *mod_dst_array_clr = sid_se_vars[sid].mod_dst;
  for(i=0; i<SID_SE_NUM_MOD_DST; ++i)
    *mod_dst_array_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise


  ///////////////////////////////////////////////////////////////////////////
  // Clock
  ///////////////////////////////////////////////////////////////////////////

  if( sid_se_clk.event.MIDI_START ) {
    // TODO?
  }

  if( sid_se_clk.event.CLK ) {
    // clock WTs
    sid_se_wt_t *w = &sid_se_wt[sid][0];
    for(i=0; i<6; ++i, ++w)
      w->clk_req = 1;

    // propagate clock/4 event to trigger matrix on each 6th clock
    if( sid_se_clk.global_clk_ctr6 == 0 ) {
      // TODO?
    }

    // propagate clock/16 event to trigger matrix on each 24th clock
    if( sid_se_clk.global_clk_ctr24 == 0 ) {
      // TODO?
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // LFOs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_lfo_t *l = &sid_se_lfo[sid][0];
  for(i=0; i<2*6; ++i, ++l)
    SID_SE_LFO(l);


  ///////////////////////////////////////////////////////////////////////////
  // ENVs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_env_t *e = &sid_se_env[sid][0];
  for(i=0; i<6; ++i, ++e)
    SID_SE_ENV(e);


  ///////////////////////////////////////////////////////////////////////////
  // Wavetables
  ///////////////////////////////////////////////////////////////////////////

  sid_se_wt_t *w = &sid_se_wt[sid][0];
  for(i=0; i<6; ++i, ++w)
    SID_SE_WT(w);


  ///////////////////////////////////////////////////////////////////////////
  // Arps
  // In difference to other engines, each MIDI voice controls an arpeggiator
  // for the last assigned voice
  ///////////////////////////////////////////////////////////////////////////
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];
  for(i=0; i<6; ++i, ++mv) {
    sid_se_voice_t *v = &sid_se_voice[sid][mv->last_voice];
    if( v->assigned_instrument == i )
      SID_SE_Arp(v);
  }


  ///////////////////////////////////////////////////////////////////////////
  // Voices
  ///////////////////////////////////////////////////////////////////////////

  // process voices
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    if( SID_SE_Gate(v) > 0 )
      SID_SE_Pitch(v);
    SID_SE_PW(v);
  }


  ///////////////////////////////////////////////////////////////////////////
  // Filters and Volume
  ///////////////////////////////////////////////////////////////////////////
  sid_se_filter_t *f = &sid_se_filter[sid][0];
  SID_SE_FilterAndVolume(f++);
  SID_SE_FilterAndVolume(f);


  ///////////////////////////////////////////////////////////////////////////
  // Tmp: copy register values directly into SID registers
  ///////////////////////////////////////////////////////////////////////////
  v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    sid_se_voice_waveform_t waveform = (sid_se_voice_waveform_t)v->voice_patch->waveform;
    v->phys_sid_voice->waveform = waveform.WAVEFORM;
    v->phys_sid_voice->sync = waveform.SYNC;
    v->phys_sid_voice->ringmod = waveform.RINGMOD;

    // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
    if( !v->set_delay_ctr ) {
      v->phys_sid_voice->ad = v->voice_patch->ad;
      v->phys_sid_voice->sr = v->voice_patch->sr;
    }
  }

  return 0; // no error
}
