// $Id$
/*
 * MBSID Bassline Engine
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
#include "sid_se_b.h"
#include "sid_patch.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SID_SE_B_Seq(sid_se_seq_t *s);


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_B_Update(u8 sid)
{
  int i;
  sid_se_vars_t *vars = &sid_se_vars[sid];


  ///////////////////////////////////////////////////////////////////////////
  // Clear all modulation destinations
  ///////////////////////////////////////////////////////////////////////////

  s32 *mod_dst_array_clr = sid_se_vars[sid].mod_dst;
  for(i=0; i<SID_SE_NUM_MOD_DST; ++i)
    *mod_dst_array_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise


  ///////////////////////////////////////////////////////////////////////////
  // LFOs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_lfo_t *l = &sid_se_lfo[sid][0];
  for(i=0; i<2*2; ++i, ++l)
    SID_SE_LFO(l);


  ///////////////////////////////////////////////////////////////////////////
  // ENVs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_env_t *e = &sid_se_env[sid][0];
  for(i=0; i<2; ++i, ++e)
    SID_SE_ENV(e);


  ///////////////////////////////////////////////////////////////////////////
  // Sequencer
  ///////////////////////////////////////////////////////////////////////////

  sid_se_seq_t *s = &sid_se_seq[sid][0];
  for(i=0; i<2; ++i, ++s)
    SID_SE_B_Seq(s);


  ///////////////////////////////////////////////////////////////////////////
  // Arps
  ///////////////////////////////////////////////////////////////////////////
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  for(i=0; i<6; i+=3, v+=3)
    SID_SE_Arp(v);


  ///////////////////////////////////////////////////////////////////////////
  // Voices
  ///////////////////////////////////////////////////////////////////////////

  // transfer note values and state flags from OSC1 (master) to OSC2/3 (slaves)
  for(i=0; i<6; i+=3) {
    sid_se_voice[sid][i+1].state = sid_se_voice[sid][i+2].state = sid_se_voice[sid][i+0].state;
    sid_se_voice[sid][i+1].note = sid_se_voice[sid][i+2].note = sid_se_voice[sid][i+0].note;
    sid_se_voice[sid][i+1].arp_note = sid_se_voice[sid][i+2].arp_note = sid_se_voice[sid][i+0].arp_note;
    sid_se_voice[sid][i+1].played_note = sid_se_voice[sid][i+2].played_note = sid_se_voice[sid][i+0].played_note;
  }

  // process voices
  v = &sid_se_voice[sid][0];
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
  for(i=0; i<6; i+=3) {
    sid_se_voice_t *v1 = &sid_se_voice[sid][i+0];
    sid_se_voice_t *v2 = &sid_se_voice[sid][i+1];
    sid_se_voice_t *v3 = &sid_se_voice[sid][i+2];

    // Voice 1 oscillator
    sid_se_voice_waveform_t v1_waveform = (sid_se_voice_waveform_t)v1->voice_patch->waveform;
    v1->phys_sid_voice->waveform = v1_waveform.WAVEFORM;
    v1->phys_sid_voice->sync = v1_waveform.SYNC;
    v1->phys_sid_voice->ringmod = v1_waveform.RINGMOD;

    // Voice 2 oscillator has own waveform and voice enable flag
    // no gate/sync if waveform disabled
    sid_se_voice_waveform_t v2_waveform = (sid_se_voice_waveform_t)v2->voice_patch->B.v2_waveform;
    if( !v2_waveform.WAVEFORM ) {
      v2->phys_sid_voice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
    } else {
      v2->phys_sid_voice->waveform = v2_waveform.WAVEFORM;
      v2->phys_sid_voice->sync = v2_waveform.SYNC;
      v2->phys_sid_voice->ringmod = v2_waveform.RINGMOD;
    }

    // Voice 3 oscillator has own waveform and voice enable flag
    // no gate/sync if waveform disabled
    sid_se_voice_waveform_t v3_waveform = (sid_se_voice_waveform_t)v3->voice_patch->B.v3_waveform;
    if( !v3_waveform.WAVEFORM ) {
      v3->phys_sid_voice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
    } else {
      v3->phys_sid_voice->waveform = v3_waveform.WAVEFORM;
      v3->phys_sid_voice->sync = v3_waveform.SYNC;
      v3->phys_sid_voice->ringmod = v3_waveform.RINGMOD;
    }

    // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
    if( !v->set_delay_ctr ) {
      u8 ad = v1->voice_patch->ad;
      u8 sr = v1->voice_patch->sr;

      // force sustain to maximum if accent flag active
      if( v1->state.ACCENT )
	sr |= 0xf0;

      v1->phys_sid_voice->ad = ad;
      v1->phys_sid_voice->sr = sr;
      v2->phys_sid_voice->ad = ad;
      v2->phys_sid_voice->sr = sr;
      v3->phys_sid_voice->ad = ad;
      v3->phys_sid_voice->sr = sr;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Bassline Sequencer
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_B_Seq(sid_se_seq_t *s)
{
  sid_se_voice_t *v = s->v;
  sid_se_v_flags_t v_flags = (sid_se_v_flags_t)v->voice_patch->B.v_flags;

  // clear gate and deselect sequence if MIDI clock stop requested
  if( sid_se_clk.event.MIDI_STOP ) {
    v->state.VOICE_ACTIVE = 0;
    v->state.GATE_CLR_REQ = 1;
    v->state.GATE_SET_REQ = 0;
    SID_SE_TriggerNoteOff(v, 0);
    return 0; // nothing else to do
  }

  // exit if WTO mode not active
  // clear gate if WTO just has been disabled (for proper transitions)
  if( !v_flags.WTO ) {
    // check if sequencer was disabled before
    if( s->state.ENABLED ) {
      // clear gate
      v->state.GATE_CLR_REQ = 1;
      v->state.GATE_SET_REQ = 0;
      SID_SE_TriggerNoteOff(v, 0);
    }
    s->state.ENABLED = 0;
    return 0; // nothing else to do
  }

  // exit if arpeggiator is enabled
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  if( arp_mode.ENABLE )
    return 0;

  // check if reset requested for FA event or sequencer was disabled before (transition Seq off->on)
  if( !s->state.ENABLED || sid_se_clk.event.MIDI_START || s->restart_req ) {
    s->restart_req = 0;
    // next clock event will increment to 0
    s->div_ctr = ~0;
    s->sub_ctr = ~0;
    // next step will increment to start position
    s->pos = ((s->seq_patch->num & 0x7) << 4) - 1;
    // ensure that slide flag is cleared
    v->state.SLIDE = 0;
  }

  if( sid_se_clk.event.MIDI_START || sid_se_clk.event.MIDI_CONTINUE ) {
    // enable voice (again)
    v->state.VOICE_ACTIVE = 1;
  }

  // sequencer enabled
  s->state.ENABLED = 1;

  // check for clock sync event
  if( sid_se_clk.event.CLK ) {
    sid_se_seq_speed_par_t speed_par = (sid_se_seq_speed_par_t)s->seq_patch->speed;
    // increment clock divider
    // reset divider if it already has reached the target value
    if( ++s->div_ctr == 0 || s->div_ctr > speed_par.CLKDIV ) {
      s->div_ctr = 0;

      // increment subcounter and check for state
      // 0: new note & gate set
      // 4: gate clear
      // >= 6: reset to 0, new note & gate set
      if( ++s->sub_ctr >= 6 )
	s->sub_ctr = 0;

      if( s->sub_ctr == 0 ) { // set gate
	// increment position counter, reset at end position
	u8 next_step = (s->pos & 0x0f) + 1;
	if( next_step > s->seq_patch->length )
	  next_step = 0;
	else
	  next_step &= 0x0f; // just to ensure...

	// change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
	u8 next_pattern = s->pos >> 4;
	if( !speed_par.SYNCH_TO_MEASURE || next_step == 0 )
	  next_pattern = s->seq_patch->num & 0x7;

	s->pos = (next_pattern << 4) | next_step;

	// play the step

	// gate off (without slide) if invalid song number (stop feature: seq >= 8)
	if( s->seq_patch->num >= 8 ) {
	  if( v->state.GATE_ACTIVE ) {
	    v->state.GATE_CLR_REQ = 1;
	    v->state.GATE_SET_REQ = 0;
	    SID_SE_TriggerNoteOff(v, 0);
	  }
	} else {
          // Sequence Storage - Structure:
          //   2 bytes for each step (selected with address bit #7)
          //   lower byte: [3:0] note, [5:4] octave, [6] glide, [7] gate
          //   upper byte: [6:0] parameter value, [7] accent
          // 16 Steps per sequence (offset 0x00..0x0f)
          // 8 sequences:
          //  0x100..0x10f/0x180..0x18f: sequence #1
          //  0x110..0x11f/0x190..0x19f: sequence #2
          //  0x120..0x12f/0x1a0..0x1af: sequence #3
          //  0x130..0x13f/0x1b0..0x1bf: sequence #4
          //  0x140..0x14f/0x1c0..0x1cf: sequence #5
          //  0x150..0x15f/0x1d0..0x1df: sequence #6
          //  0x160..0x16f/0x1e0..0x1ef: sequence #7
          //  0x170..0x17f/0x1f0..0x1ff: sequence #8

	  // get note/par value
	  sid_se_seq_note_item_t note_item = (sid_se_seq_note_item_t)sid_patch[s->sid].B.seq_memory[s->pos];
	  sid_se_seq_asg_item_t asg_item = (sid_se_seq_asg_item_t)sid_patch[s->sid].B.seq_memory[s->pos + 0x80];

#if DEBUG_VERBOSE_LEVEL >= 2
	  DEBUG_MSG("SEQ %d@%d/%d: 0x%02x 0x%02x\n", s->seq, s->pos >> 4, s->pos & 0xf, note_item.ALL, asg_item.ALL);
#endif

	  // determine note
	  u8 note = note_item.NOTE + 0x3c;

	  // add octave
	  switch( note_item.OCTAVE ) {
	    case 1: note += 12; break; // Up
	    case 2: note -= 12; break; // Down
	    case 3: note += 24; break; // Up2
	  }

	  // transfer to voice
	  v->note = note;

	  // set accent
	  // ignore if slide has been set by previous step
	  // (important for SID sustain: transition from sustain < 0xf to 0xf will reset the VCA)
	  if( !v->state.SLIDE ) {
	    // take over accent
	    v->state.ACCENT = asg_item.ACCENT;
	  }

	  // activate portamento if slide has been set by previous step
	  v->state.PORTA_ACTIVE = v->state.SLIDE;

	  // set slide flag of current flag
	  v->state.SLIDE = note_item.SLIDE;

	  // set gate if flag is set
	  if( note_item.GATE && v->state.VOICE_ACTIVE ) {
	    v->state.GATE_CLR_REQ = 0; // ensure that gate won't be cleared by previous CLR_REQ
	    if( !v->state.GATE_ACTIVE ) {
	      v->state.GATE_SET_REQ = 1;

	      // request ENV attack and LFO sync
	      SID_SE_TriggerNoteOn(v, 0);
	    }
	  }

	  // parameter track:
	  // determine SID channels which should be modified
	  if( s->seq_patch->assign ) {
	    u8 sidlr = 1 << s->seq; // SIDL/R selection
	    u8 ins = 0;
	    SID_PAR_SetWT(s->sid, s->seq_patch->assign, asg_item.PAR_VALUE + 0x80, sidlr, ins);
	  }
	}
      } else if( s->sub_ctr == 4 ) { // clear gate
	// don't clear if slide flag is set!
	if( !v->state.SLIDE ) {
	  v->state.GATE_CLR_REQ = 1;
	  v->state.GATE_SET_REQ = 0;
	  SID_SE_TriggerNoteOff(v, 0);
	}
      }
    }
  }

  return 0; // no error
}
