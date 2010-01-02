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

#include "sid_patch.h"
#include "sid_se.h"
#include "sid_se_l.h"
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
  s32 status = 0;

  status |= SID_MIDI_L_Init(mode);

  return status;
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
        case SID_SE_BASSLINE:  return -2; // TODO
        case SID_SE_DRUM:      return -2; // TODO
        case SID_SE_MULTI:     return -2; // TODO
        default:               return -1; // unsupported engine
      }
      break;

    case CC:
      // knob values are available for all engines
      if( midi_package.chn == sid_se_voice[sid][0].midi_channel ) {
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
        case SID_SE_BASSLINE:  return -2; // TODO
        case SID_SE_DRUM:      return -2; // TODO
        case SID_SE_MULTI:     return -2; // TODO
        default:               return -1; // unsupported engine
      }
      break;

    case PitchBend: {
      u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
      u16 pitchbend_value_8bit = pitchbend_value_14bit >> 6;

      // copy pitchbender value into mod matrix source
      SID_KNOB_SetValue(sid, SID_KNOB_PITCHBENDER, pitchbend_value_8bit);
    } break;


  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable Notestack Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_PushWT(sid_se_voice_t *v, u8 note)
{
  int i;
  for(i=0; i<4; ++i) {
    u8 stack_note = v->wt_stack[i] & 0x7f;
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
	  v->wt_stack[j] = v->wt_stack[j-1];
      }

      // insert note
      v->wt_stack[i] = note;

      return 0; // no error
    }
  }

  return 0; // no error
}

s32 SID_MIDI_PopWT(sid_se_voice_t *v, u8 note)
{
  int i;

  // search for note entry with the same number, erase it and push the entries behind
  for(i=0; i<4; ++i) {
    u8 stack_note = v->wt_stack[i] & 0x7f;

    if( note == stack_note ) {
      int j;

      // push the entries behind the found entry
      if( i != 3 ) {
	for(j=i; j<3; ++j)
	  v->wt_stack[j] = v->wt_stack[j+1];
      }

      // clear last entry
      v->wt_stack[3] = 0;
    }
  }

  return -1; // note not found
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator Notestack Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_ArpNoteOn(sid_se_voice_t *v, u8 note, u8 velocity)
{
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  sid_se_voice_arp_speed_div_t arp_speed_div = (sid_se_voice_arp_speed_div_t)v->voice_patch->arp_speed_div;

  // store current notestack mode
  notestack_mode_t saved_mode = v->notestack.mode;

  if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
    // easy chord entry:
    // even when HOLD mode not active, a note off doesn't remove notes in stack
    // the notes of released keys will be removed from stack once a *new* note is played
    NOTESTACK_RemoveNonActiveNotes(&v->notestack);
  }

  // if no note is played anymore, clear stack again (so that new notes can be added in HOLD mode)
  if( NOTESTACK_CountActiveNotes(&v->notestack) == 0 ) {
    // clear stack
    NOTESTACK_Clear(&v->notestack);
    // synchronize the arpeggiator
    v->arp_state.SYNC_ARP = 1;
  }

  // push note into stack - select mode depending on sort/hold mode
  if( arp_mode.HOLD )
    v->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
  else
    v->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
  NOTESTACK_Push(&v->notestack, note, velocity);

  // activate note
  v->state.VOICE_ACTIVE = 1;

  // remember note
  v->played_note = note;

  // restore notestack mode
  v->notestack.mode = saved_mode;

  return 0; // no error
}

s32 SID_MIDI_ArpNoteOff(sid_se_voice_t *v, u8 note)
{
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  sid_se_voice_arp_speed_div_t arp_speed_div = (sid_se_voice_arp_speed_div_t)v->voice_patch->arp_speed_div;

  // store current notestack mode
  notestack_mode_t saved_mode = v->notestack.mode;

  if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
    // select mode depending on arp flags
    // always pop note in hold mode
    v->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
  } else {
    // select mode depending on arp flags
    if( arp_mode.HOLD )
      v->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    else
      v->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
  }

  // remove note from stack
  NOTESTACK_Pop(&v->notestack, note);

  // release voice if no note in queue anymore
  if( NOTESTACK_CountActiveNotes(&v->notestack) == 0 )
    v->state.VOICE_ACTIVE = 0;

  // restore notestack mode
  v->notestack.mode = saved_mode;

  return 0; // no error
}

