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
// NoteOn/Off Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_L_Receive_Note(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
  sid_patch_t *p = &sid_patch[sid];
  sid_se_v_flags_t v_flags = (sid_se_v_flags_t)p->L.v_flags;

  // check if MIDI channel and splitzone matches
  if( midi_package.chn != mv->midi_channel ||
      midi_package.note < mv->split_lower ||
      (mv->split_upper && midi_package.note > mv->split_upper) )
    return 0; // note filtered

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  if( midi_package.velocity ) { // Note On
    // copy velocity into mod matrix source
    SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

    // go through all voices
    int voice;
    for(voice=0; voice<6; ++voice, ++v) {
      sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
      sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;

      // push note into WT stack
      SID_MIDI_PushWT(v, midi_package.note);
#if DEBUG_VERBOSE_LEVEL >= 2
      if( mv->midi_voice == 0 )
	DEBUG_MSG("WT_STACK>%02x %02x %02x %02x\n", mv->wt_stack[0], mv->wt_stack[1], mv->wt_stack[2], mv->wt_stack[3]);
#endif

      // branch depending on normal/arp mode
      if( arp_mode.ENABLE ) {
	// call Arp Note On Handler
	SID_MIDI_ArpNoteOn(v, midi_package.note, midi_package.velocity);
      } else {
	// push note into stack
	NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);

	// switch off gate if not in legato or WTO mode
	if( !v_flags.LEGATO && !v_flags.WTO )
	  SID_MIDI_GateOff(v);

	// call Note On Handler
	SID_MIDI_NoteOn(v, midi_package.note, midi_package.velocity, v_flags);
      }
    }
  } else { // Note Off
    // go through all voices
    int voice;
    sid_se_voice_t *v = &sid_se_voice[sid][0];
    for(voice=0; voice<6; ++voice, ++v) {
      sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
      sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;

      // pop from WT stack if sustain not active (TODO: sustain switch)
      SID_MIDI_PopWT(v, midi_package.note);
#if DEBUG_VERBOSE_LEVEL >= 2
      if( mv->midi_voice == 0 )
	DEBUG_MSG("WT_STACK<%02x %02x %02x %02x\n", mv->wt_stack[0], mv->wt_stack[1], mv->wt_stack[2], mv->wt_stack[3]);
#endif

      // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
      // if not in arp mode: sustain only relevant if only one active note in stack

      // branch depending on normal/arp mode
      if( arp_mode.ENABLE ) {
	// call Arp Note Off Handler
	SID_MIDI_ArpNoteOff(v, midi_package.note);
      } else {
	u8 last_first_note = mv->notestack.note_items[0].note;
	// pop note from stack
	if( NOTESTACK_Pop(&mv->notestack, midi_package.note) > 0 ) {
	  // call Note Off Handler
	  if( SID_MIDI_NoteOff(v, midi_package.note, last_first_note, v_flags) > 0 ) // retrigger requested?
	    SID_MIDI_NoteOn(v, mv->notestack.note_items[0].note, mv->notestack.note_items[0].tag, v_flags); // new note
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
s32 SID_MIDI_L_Receive_CC(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
  if( midi_package.chn != mv->midi_channel )
    return 0; // CC filtered

  return 0; // no error
}
