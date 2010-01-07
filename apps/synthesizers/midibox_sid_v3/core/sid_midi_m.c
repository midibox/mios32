// $Id$
/*
 * MBSID MIDI Parser for Multi Engine
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

#include <sid.h>
#include <notestack.h>

#include "sid_midi.h"
#include "sid_midi_b.h"

#include "sid_se.h"
#include "sid_se_m.h"

#include "sid_patch.h"
#include "sid_knob.h"
#include "sid_voice.h"



/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////
static sid_se_voice_t *SID_MIDI_M_VOICE_Get(sid_se_midi_voice_t *mv, sid_se_voice_patch_t *voice_patch, u8 poly_mode);


/////////////////////////////////////////////////////////////////////////////
// NoteOn/Off Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_M_Receive_Note(u8 sid, mios32_midi_package_t midi_package)
{
  // go through six midi voices
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  int midi_voice;
  for(midi_voice=0; midi_voice<6; ++midi_voice, ++mv) {
    // check if MIDI channel and splitzone matches
    if( midi_package.chn != mv->midi_channel ||
	midi_package.note < mv->split_lower ||
	(mv->split_upper && midi_package.note > mv->split_upper) )
      continue; // next MIDI voice

    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].M.voice[midi_voice];
    sid_se_v_flags_t v_flags = (sid_se_v_flags_t)voice_patch->M.v_flags;
    sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)voice_patch->arp_mode;

    if( midi_package.velocity ) { // Note On
      // if first MIDI voice: copy velocity into mod matrix source
      if( midi_voice == 0 )
	SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

      // push note into WT stack
      SID_MIDI_PushWT(mv, midi_package.note);

      // branch depending on normal/arp mode
      if( arp_mode.ENABLE ) {
	// get new voice
	sid_se_voice_t *v = SID_MIDI_M_VOICE_Get(mv, voice_patch, 0);

	// dedicated velocity assignment for instrument
	u8 velocity_asg = voice_patch->M.velocity_asg;
	// forward velocity to parameter (if assigned)
	if( velocity_asg )
	  SID_PAR_Set16(sid, velocity_asg, midi_package.velocity << 9, 3, midi_voice);

	// call Arp Note On Handler
	SID_MIDI_ArpNoteOn(v, midi_package.note, midi_package.velocity);
      } else {
	// push note into stack
	NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);

	// get new voice
	sid_se_voice_t *v = SID_MIDI_M_VOICE_Get(mv, voice_patch, v_flags.POLY);

	// dedicated velocity assignment for instrument
	u8 velocity_asg = voice_patch->M.velocity_asg;
	// forward velocity to parameter (if assigned)
	if( velocity_asg )
	  SID_PAR_Set16(sid, velocity_asg, midi_package.velocity << 9, 3, midi_voice);

	// switch off gate if not in legato or WTO mode
	if( !v_flags.LEGATO && !v_flags.WTO )
	  SID_MIDI_GateOff(v, mv, midi_package.note);

	// call Note On Handler
	SID_MIDI_NoteOn(v, midi_package.note, midi_package.velocity, v_flags);
      }
    } else { // Note Off
      // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
      // if not in arp mode: sustain only relevant if only one active note in stack

      // pop from WT stack if sustain not active (TODO: sustain switch)
      SID_MIDI_PopWT(mv, midi_package.note);

      // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
      // if not in arp mode: sustain only relevant if only one active note in stack

      // get pointer to last voice
      sid_se_voice_t *last_v = (sid_se_voice_t *)&sid_se_voice[mv->sid][mv->last_voice];

      // branch depending on normal/arp mode
      if( arp_mode.ENABLE ) {
	// call Arp Note Off Handler
	SID_MIDI_ArpNoteOff(last_v, midi_package.note);
	// release voice
	SID_VOICE_Release(mv->sid, last_v->voice);
      } else {
	u8 last_first_note = mv->notestack.note_items[0].note;
	// pop note from stack
	if( NOTESTACK_Pop(&mv->notestack, midi_package.note) > 0 ) {
	  // call Note Off Handler
	  if( SID_MIDI_NoteOff(last_v, midi_package.note, last_first_note, v_flags) > 0 ) { // retrigger requested?
	    sid_se_voice_t *v = SID_MIDI_M_VOICE_Get(mv, voice_patch, v_flags.POLY);
	    SID_MIDI_NoteOn(v, mv->notestack.note_items[0].note, mv->notestack.note_items[0].tag, v_flags); // new note
	  }
	}
      }
    }
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// CC Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_M_Receive_CC(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  int midi_voice;
  for(midi_voice=0; midi_voice<6; ++midi_voice, ++mv) {
    if( midi_package.chn != mv->midi_channel )
      continue; // filtered
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Pitchbender Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_M_Receive_PitchBender(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);

  int midi_voice;
  for(midi_voice=0; midi_voice<6; ++midi_voice, ++mv) {
    if( midi_package.chn != mv->midi_channel )
      continue; // filtered

    // dedicated pitchbender assignment for instrument
    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].M.voice[midi_voice];
    u8 pitchbender_asg = voice_patch->M.pitchbender_asg;
    if( pitchbender_asg )
      SID_PAR_Set16(sid, pitchbender_asg, pitchbend_value_14bit << 2, 3, midi_voice);
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Functions to get/release a voice
/////////////////////////////////////////////////////////////////////////////
static sid_se_voice_t *SID_MIDI_M_VOICE_Get(sid_se_midi_voice_t *mv, sid_se_voice_patch_t *voice_patch, u8 poly_mode)
{
  int i;

  // get voice assignment
  u8 voice_asg = voice_patch->M.voice_asg;

  // number of voices depends on mono/stereo mode (TODO: once ensemble available...)
  u8 num_voices = 6;

  // request new voice depending on poly or mono/arp mode
  u8 voice;
  if( poly_mode )
    voice = SID_VOICE_Get(mv->sid, mv->midi_voice, voice_asg, num_voices);
  else
    voice = SID_VOICE_GetLast(mv->sid, mv->midi_voice, voice_asg, num_voices, mv->last_voice);

  // transfer frequency settings of last voice into new voice (for proper portamento in poly mode)
  sid_se_voice_t *last_v = (sid_se_voice_t *)&sid_se_voice[mv->sid][mv->last_voice];
  sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[mv->sid][voice];

  v->prev_transposed_note = last_v->prev_transposed_note;
  v->portamento_end_frq = last_v->portamento_end_frq;
  v->linear_frq = last_v->linear_frq;
  v->state.FORCE_FRQ_RECALC = 1; // take over new frequencies

  // remember new voice in MIDI voice structure
  mv->last_voice = v->voice;

  // store pointer to MIDI voice structure and patch in voice structure
  v->assigned_instrument = mv->midi_voice;
  v->mv = mv;
  v->voice_patch = voice_patch;

  // assign LFO/ENV/WTs to voice patch
  sid_se_lfo_t *l = (sid_se_lfo_t *)&sid_se_lfo[mv->sid][2*voice];
  for(i=0; i<2; ++i, ++l)
    l->lfo_patch = (sid_se_lfo_patch_t *)&voice_patch->M.lfo[i];

  sid_se_env_t *e = (sid_se_env_t *)&sid_se_env[mv->sid][voice];
  e->env_patch = (sid_se_env_patch_t *)&voice_patch->M.env;

  sid_se_wt_t *w = (sid_se_wt_t *)&sid_se_wt[mv->sid][voice];
  w->wt_patch = (sid_se_wt_patch_t *)&voice_patch->M.wt;

  return v; // return voice
}
