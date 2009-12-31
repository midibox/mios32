// $Id$
/*
 * MBSID MIDI Parser for Lead Engine
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
#include "sid_midi_l.h"

#include "sid_se.h"
#include "sid_se_l.h"

#include "sid_patch.h"
#include "sid_knob.h"



/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 SID_MIDI_L_NoteOn(sid_se_voice_t *v, u8 note, sid_se_l_flags_t l_flags);
static s32 SID_MIDI_L_NoteOff(sid_se_voice_t *v, u8 note, u8 last_first_note, sid_se_l_flags_t l_flags);
static s32 SID_MIDI_L_ArpNoteOn(sid_se_voice_t *v, u8 note, u8 velocity);
static s32 SID_MIDI_L_ArpNoteOff(sid_se_voice_t *v, u8 note);
static s32 SID_MIDI_L_GateOn(sid_se_voice_t *v);
static s32 SID_MIDI_L_GateOff(sid_se_voice_t *v);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_L_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// NoteOn/Off Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_L_Receive_Note(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  sid_patch_t *p = &sid_patch[sid];
  sid_se_l_flags_t l_flags = (sid_se_l_flags_t)p->L.flags;

  // check if MIDI channel and splitzone matches
  if( midi_package.chn != v->midi_channel ||
      midi_package.note < v->split_lower ||
      (v->split_upper && midi_package.note > v->split_upper) )
    return 0; // note filtered

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  if( midi_package.velocity ) { // Note On
    // copy velocity into mod matrix source
    SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

    // go through all voices
    int voice;
    for(voice=0; voice<6; ++voice, ++v) {
      sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;

      // push note into WT stack
      SID_MIDI_PushWT(v, midi_package.note);
#if DEBUG_VERBOSE_LEVEL >= 2
      if( v->voice == 0 )
	DEBUG_MSG("WT_STACK>%02x %02x %02x %02x\n", v->wt_stack[0], v->wt_stack[1], v->wt_stack[2], v->wt_stack[3]);
#endif

      // branch depending on normal/arp mode
      if( !arp_mode.ENABLE ) {
	// push note into stack
	NOTESTACK_Push(&v->notestack, midi_package.note, midi_package.velocity);

	// switch off gate if not in legato or WTO mode
	if( !l_flags.LEGATO && !l_flags.WTO )
	  SID_MIDI_L_GateOff(v);

	// call Note On Handler
	SID_MIDI_L_NoteOn(v, midi_package.note, l_flags);
      } else {
	// call Arp Note On Handler
	SID_MIDI_L_ArpNoteOn(v, midi_package.note, midi_package.velocity);
      }
    }
  } else { // Note Off
    // go through all voices
    int voice;
    sid_se_voice_t *v = &sid_se_voice[sid][0];
    for(voice=0; voice<6; ++voice, ++v) {
      sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;

      // pop from WT stack if sustain not active (TODO: sustain switch)
      SID_MIDI_PopWT(v, midi_package.note);
#if DEBUG_VERBOSE_LEVEL >= 2
      if( v->voice == 0 )
	DEBUG_MSG("WT_STACK<%02x %02x %02x %02x\n", v->wt_stack[0], v->wt_stack[1], v->wt_stack[2], v->wt_stack[3]);
#endif

      // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
      // if not in arp mode: sustain only relevant if only one active note in stack

      // branch depending on normal/arp mode
      if( !arp_mode.ENABLE ) {
	u8 last_first_note = v->notestack.note_items[0].note;
	// pop note from stack
	if( NOTESTACK_Pop(&v->notestack, midi_package.note) > 0 ) {
	  // call Note Off Handler
	  if( SID_MIDI_L_NoteOff(v, midi_package.note, last_first_note, l_flags) > 0 ) // retrigger requested?
	    SID_MIDI_L_NoteOn(v, v->notestack.note_items[0].note, l_flags); // new note
	}
      } else {
	// call Arp Note Off Handler
	SID_MIDI_L_ArpNoteOff(v, midi_package.note);
      }
    }
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// CC Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_L_Receive_CC(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  if( midi_package.chn != v->midi_channel )
    return 0; // CC filtered

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Help Routines
/////////////////////////////////////////////////////////////////////////////
static s32 SID_MIDI_L_NoteOn(sid_se_voice_t *v, u8 note, sid_se_l_flags_t l_flags)
{
  // save note
  v->played_note = v->notestack.note_items[0].note;
  if( !l_flags.WTO )
    v->note = v->notestack.note_items[0].note;

  // ensure that note is not depressed anymore
  // TODO n->note_items[0].depressed = 0;

  if( l_flags.SUSKEY ) {
    // in SusKey mode, we activate portamento only if at least two keys are played
    // omit portamento if first key played after patch initialisation
    if( v->notestack.len >= 2 && v->state.PORTA_INITIALIZED )
      v->state.PORTA_ACTIVE = 1;
  } else {
    // portamento always activated (will immediately finish if portamento value = 0)
    v->state.PORTA_ACTIVE = 1;
  }

  // next key will allow portamento
  v->state.PORTA_INITIALIZED = 1;

  // gate bit handling
  if( !l_flags.LEGATO || !v->state.VOICE_ACTIVE ) {
    SID_MIDI_L_GateOn(v);

    // trigger matrix
    if( !l_flags.LEGATO || v->notestack.len == 1 ) {
      v->trg_dst[0] |= v->trg_mask_note_on[0] & (0xc0 | (1 << v->voice)); // only current voice can trigger NoteOn
      v->trg_dst[1] |= v->trg_mask_note_on[1];
      v->trg_dst[2] |= v->trg_mask_note_on[2];
    }
  }

  return 0; // no error
}

static s32 SID_MIDI_L_NoteOff(sid_se_voice_t *v, u8 note, u8 last_first_note, sid_se_l_flags_t l_flags)
{
  // if there is still a note in the stack, play new note with NoteOn Function (checked by caller)
  if( v->notestack.len ) {
    // if not in legato mode and current note-off number equat to last entry #0: gate off
    if( !l_flags.LEGATO && note == last_first_note )
      SID_MIDI_L_GateOff(v);

    // activate portamento (will be ignored by pitch handler if no portamento active - important for SusKey function to have it here!)
    v->state.PORTA_ACTIVE = 1;
    return 1; // request Note On!
  }

  // request gate clear bit
  SID_MIDI_L_GateOff(v);

  // trigger matrix
  v->trg_dst[0] |= v->trg_mask_note_off[0] & 0xc0; // gates handled separately in SID_MIDI_L_GateOff
  v->trg_dst[1] |= v->trg_mask_note_off[1];
  v->trg_dst[2] |= v->trg_mask_note_off[2];

  return 0; // no error, NO note on!
}

static s32 SID_MIDI_L_ArpNoteOn(sid_se_voice_t *v, u8 note, u8 velocity)
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

static s32 SID_MIDI_L_ArpNoteOff(sid_se_voice_t *v, u8 note)
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

static s32 SID_MIDI_L_GateOn(sid_se_voice_t *v)
{
  v->state.VOICE_ACTIVE = 1;
  // gate requested via trigger matrix

  return 0; // no error
}

static s32 SID_MIDI_L_GateOff(sid_se_voice_t *v)
{
  if( v->state.VOICE_ACTIVE ) {
    v->state.VOICE_ACTIVE = 0;
    v->state.GATE_SET_REQ = 0;

    // request gate off if not disabled via trigger matrix
    u8 *triggers = (u8 *)&sid_patch[v->sid].L.trg_matrix[SID_SE_TRG_NOff][0];
    if( triggers[0] & (1 << v->voice) )
      v->state.GATE_CLR_REQ = 1;

    // remove gate set request
    v->trg_dst[0] &= ~(1 << v->voice);
  }

  return 0; // no error
}
