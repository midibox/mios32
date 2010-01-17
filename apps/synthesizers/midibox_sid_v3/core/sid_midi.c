// $Id$
/*
 * MBSID MIDI Parser
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

#include "sid_midi.h"
#include "sid_midi_l.h"
#include "sid_midi_b.h"
#include "sid_midi_d.h"
#include "sid_midi_m.h"

#include "sid_patch.h"
#include "sid_bank.h"
#include "sid_se.h"
#include "sid_voice.h"
#include "sid_knob.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // TODO: support for multiple SIDs
  u8 sid = 0;
  sid_se_engine_t engine = sid_patch[sid].engine;

  switch( midi_package.event ) {
    case NoteOff:
      midi_package.velocity = 0;
      // no break, fall through

    case NoteOn:
      switch( engine ) {
        case SID_SE_LEAD:      return SID_MIDI_L_Receive_Note(sid, midi_package);
        case SID_SE_BASSLINE:  return SID_MIDI_B_Receive_Note(sid, midi_package);
        case SID_SE_DRUM:      return SID_MIDI_D_Receive_Note(sid, midi_package);
        case SID_SE_MULTI:     return SID_MIDI_M_Receive_Note(sid, midi_package);
        default:               return -1; // unsupported engine
      }
      break;

    case CC:
      // knob values are available for all engines
      if( midi_package.chn == sid_se_voice[sid][0].mv->midi_channel ) {
	switch( midi_package.cc_number ) {
	  case  1: SID_KNOB_SetValue(sid, SID_KNOB_1, midi_package.value << 1); break;
	  case 16: SID_KNOB_SetValue(sid, SID_KNOB_2, midi_package.value << 1); break;
	  case 17: SID_KNOB_SetValue(sid, SID_KNOB_3, midi_package.value << 1); break;
	  case 18: SID_KNOB_SetValue(sid, SID_KNOB_4, midi_package.value << 1); break;
	  case 19: SID_KNOB_SetValue(sid, SID_KNOB_5, midi_package.value << 1); break;
	}
      }

      switch( engine ) {
        case SID_SE_LEAD:      return SID_MIDI_L_Receive_CC(sid, midi_package);
        case SID_SE_BASSLINE:  return SID_MIDI_B_Receive_CC(sid, midi_package);
        case SID_SE_DRUM:      return SID_MIDI_D_Receive_CC(sid, midi_package);
        case SID_SE_MULTI:     return SID_MIDI_M_Receive_CC(sid, midi_package);
        default:               return -1; // unsupported engine
      }
      break;

    case PitchBend: {
      u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
      u16 pitchbend_value_8bit = pitchbend_value_14bit >> 6;

      // copy pitchbender value into mod matrix source
      SID_KNOB_SetValue(sid, SID_KNOB_PITCHBENDER, pitchbend_value_8bit);

      switch( engine ) {
        case SID_SE_LEAD:      return SID_MIDI_L_Receive_PitchBender(sid, midi_package);
        case SID_SE_BASSLINE:  return SID_MIDI_B_Receive_PitchBender(sid, midi_package);
        case SID_SE_DRUM:      return SID_MIDI_D_Receive_PitchBender(sid, midi_package);
        case SID_SE_MULTI:     return SID_MIDI_M_Receive_PitchBender(sid, midi_package);
        default:               return -1; // unsupported engine
      }
    } break;

    case ProgramChange: {
      sid_patch_ref_t *pr = &sid_patch_ref[sid];
      pr->patch = midi_package.evnt1;
      SID_BANK_PatchRead(pr);
      SID_PATCH_Changed(sid);
    } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable Notestack Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_PushWT(sid_se_midi_voice_t *mv, u8 note)
{
  int i;
  for(i=0; i<4; ++i) {
    u8 stack_note = mv->wt_stack[i] & 0x7f;
    u8 push_stack = 0;

    if( !stack_note ) { // last entry?
      push_stack = 1;
    } else {
      // ignore if note is already in stack
      if( stack_note == note )
	return 0; // no error
      // push into stack if note >= current note
      if( stack_note >= note )
	push_stack = 1;
    }

    if( push_stack ) {
      if( i != 3 ) { // max note: no shift required
	int j;

	for(j=3; j>i; --j)
	  mv->wt_stack[j] = mv->wt_stack[j-1];
      }

      // insert note
      mv->wt_stack[i] = note;

      return 0; // no error
    }
  }

  return 0; // no error
}

s32 SID_MIDI_PopWT(sid_se_midi_voice_t *mv, u8 note)
{
  int i;

  // search for note entry with the same number, erase it and push the entries behind
  for(i=0; i<4; ++i) {
    u8 stack_note = mv->wt_stack[i] & 0x7f;

    if( note == stack_note ) {
      int j;

      // push the entries behind the found entry
      if( i != 3 ) {
	for(j=i; j<3; ++j)
	  mv->wt_stack[j] = mv->wt_stack[j+1];
      }

      // clear last entry
      mv->wt_stack[3] = 0;
    }
  }

  return -1; // note not found
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator Notestack Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_ArpNoteOn(sid_se_voice_t *v, u8 note, u8 velocity)
{
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  sid_se_voice_arp_speed_div_t arp_speed_div = (sid_se_voice_arp_speed_div_t)v->voice_patch->arp_speed_div;

  // store current notestack mode
  notestack_mode_t saved_mode = mv->notestack.mode;

  if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
    // easy chord entry:
    // even when HOLD mode not active, a note off doesn't remove notes in stack
    // the notes of released keys will be removed from stack once a *new* note is played
    NOTESTACK_RemoveNonActiveNotes(&mv->notestack);
  }

  // if no note is played anymore, clear stack again (so that new notes can be added in HOLD mode)
  if( NOTESTACK_CountActiveNotes(&mv->notestack) == 0 ) {
    // clear stack
    NOTESTACK_Clear(&mv->notestack);
    // synchronize the arpeggiator
    mv->arp_state.SYNC_ARP = 1;
  }

  // push note into stack - select mode depending on sort/hold mode
  if( arp_mode.HOLD )
    mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
  else
    mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
  NOTESTACK_Push(&mv->notestack, note, velocity);

  // activate note
  v->state.VOICE_ACTIVE = 1;

  // remember note
  v->played_note = note;

  // restore notestack mode
  mv->notestack.mode = saved_mode;

  return 0; // no error
}

s32 SID_MIDI_ArpNoteOff(sid_se_voice_t *v, u8 note)
{
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  sid_se_voice_arp_speed_div_t arp_speed_div = (sid_se_voice_arp_speed_div_t)v->voice_patch->arp_speed_div;

  // store current notestack mode
  notestack_mode_t saved_mode = mv->notestack.mode;

  if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
    // select mode depending on arp flags
    // always pop note in hold mode
    mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
  } else {
    // select mode depending on arp flags
    if( arp_mode.HOLD )
      mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    else
      mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
  }

  // remove note from stack
  NOTESTACK_Pop(&mv->notestack, note);

  // release voice if no note in queue anymore
  if( NOTESTACK_CountActiveNotes(&mv->notestack) == 0 )
    v->state.VOICE_ACTIVE = 0;

  // restore notestack mode
  mv->notestack.mode = saved_mode;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Normal Note Handling (valid for Lead/Bassline/Multi engine)
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_NoteOn(sid_se_voice_t *v, u8 note, u8 velocity, sid_se_v_flags_t v_flags)
{
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;

  // save note
  v->played_note = mv->notestack.note_items[0].note;
  if( !v_flags.WTO )
    v->note = mv->notestack.note_items[0].note;

  // ensure that note is not depressed anymore
  // TODO n->note_items[0].depressed = 0;

  if( v_flags.SUSKEY ) {
    // in SusKey mode, we activate portamento only if at least two keys are played
    // omit portamento if first key played after patch initialisation
    if( mv->notestack.len >= 2 && v->state.PORTA_INITIALIZED )
      v->state.PORTA_ACTIVE = 1;
  } else {
    // portamento always activated (will immediately finish if portamento value = 0)
    v->state.PORTA_ACTIVE = 1;
  }

  // next key will allow portamento
  v->state.PORTA_INITIALIZED = 1;

  // gate bit handling
  if( !v_flags.LEGATO || !v->state.VOICE_ACTIVE ) {
    // enable gate
    SID_MIDI_GateOn(v);

    // set accent flag depending on velocity (set when velocity >= 0x40)
    v->state.ACCENT = (velocity >= 0x40) ? 1 : 0;

    // trigger matrix
    SID_SE_TriggerNoteOn(v, 0);
  }

  return 0; // no error
}

s32 SID_MIDI_NoteOff(sid_se_voice_t *v, u8 note, u8 last_first_note, sid_se_v_flags_t v_flags)
{
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;

  // if there is still a note in the stack, play new note with NoteOn Function (checked by caller)
  if( mv->notestack.len && !v_flags.POLY ) {
    // if not in legato mode and current note-off number equat to last entry #0: gate off
    if( !v_flags.LEGATO && note == last_first_note )
      SID_MIDI_GateOff(v, mv, note);

    // activate portamento (will be ignored by pitch handler if no portamento active - important for SusKey function to have it here!)
    v->state.PORTA_ACTIVE = 1;
    return 1; // request Note On!
  }

  // request gate clear bit
  SID_MIDI_GateOff(v, mv, note);

  // trigger matrix
  SID_SE_TriggerNoteOff(v, 0);

  return 0; // no error, NO note on!
}


/////////////////////////////////////////////////////////////////////////////
// Gate Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_GateOn(sid_se_voice_t *v)
{
  v->state.VOICE_ACTIVE = 1;
  // gate requested via trigger matrix

  return 0; // no error
}


static s32 SID_MIDI_GateOff_SingleVoice(sid_se_voice_t *v)
{
  if( v->state.VOICE_ACTIVE ) {
    v->state.VOICE_ACTIVE = 0;
    v->state.GATE_SET_REQ = 0;

    // request gate off if not disabled via trigger matrix
    if( v->trg_mask_note_off ) {
      u8 *trg_mask_note_off = (u8 *)&v->trg_mask_note_off->ALL;
      if( trg_mask_note_off[0] & (1 << v->voice) )
	v->state.GATE_CLR_REQ = 1;
    } else
      v->state.GATE_CLR_REQ = 1;

    // remove gate set request
    v->note_restart_req = 0;
  }

  return 0; // no error
}


s32 SID_MIDI_GateOff(sid_se_voice_t *v, sid_se_midi_voice_t *mv, u8 note)
{
  if( v->engine == SID_SE_MULTI ) {
    // go through all voices which are assigned to the current instrument and note
    u8 instrument = mv->midi_voice;
    sid_se_voice_t *v_i = (sid_se_voice_t *)&sid_se_voice[mv->sid][0];
    int voice;
    for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v_i)
      if( v_i->assigned_instrument == instrument && v_i->played_note == note ) {
	SID_MIDI_GateOff_SingleVoice(v_i);
	SID_VOICE_Release(mv->sid, voice);
      }
  } else {
    SID_MIDI_GateOff_SingleVoice(v);
  }

  return 0; // no error
}
