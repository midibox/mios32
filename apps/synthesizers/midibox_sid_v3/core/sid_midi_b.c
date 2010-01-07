// $Id$
/*
 * MBSID MIDI Parser for Bassline Engine
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
#include "sid_se_b.h"

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

static s32 SID_MIDI_B_SetSeq(sid_se_voice_t *v);


/////////////////////////////////////////////////////////////////////////////
// NoteOn/Off Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_B_Receive_Note(u8 sid, mios32_midi_package_t midi_package)
{
  // go through four midi voices
  // 1 and 2 used to play the bassline/select the sequence
  // 3 and 4 used to transpose a sequence
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  int midi_voice;
  for(midi_voice=0; midi_voice<4; ++midi_voice, ++mv) {
    // check if MIDI channel and splitzone matches
    if( midi_package.chn != mv->midi_channel ||
	midi_package.note < mv->split_lower ||
	(mv->split_upper && midi_package.note > mv->split_upper) )
      continue; // next MIDI voice

    if( midi_voice >= 2 ) { // Transposer
      if( midi_package.velocity ) { // Note On
	// push note into stack
	NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);
      } else { // Note Off
	// pop note from stack
	NOTESTACK_Pop(&mv->notestack, midi_package.note);
      }

    } else { // Voice Control
      sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[sid][midi_voice ? 3 : 0]; // either select left/right voice1
      sid_se_v_flags_t v_flags = (sid_se_v_flags_t)v->voice_patch->B.v_flags;
      sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;

      if( midi_package.velocity ) { // Note On
	// copy velocity into mod matrix source
	SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

	// branch depending on normal/arp/seq mode
	if( v_flags.WTO ) {
	  // push note into WT stack
	  SID_MIDI_PushWT(mv, midi_package.note);
	  // push note into normal note stack
	  NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);

	  // reset sequencer if voice was not active before
	  // do this with both voices for proper synchronisation!
	  // (only done in Clock Master mode)
	  if( !sid_se_clk.state.SLAVE ) {
	    if( !sid_se_voice[sid][0].state.VOICE_ACTIVE )
	      sid_se_seq[sid][0].restart_req = 1;
	    if( !sid_se_voice[sid][3].state.VOICE_ACTIVE )
	      sid_se_seq[sid][1].restart_req = 1;
	  }

	  // change sequence
	  SID_MIDI_B_SetSeq(v);

	  // always set voice active flag of both voices to ensure that they are in sync
	  // ensure that this is only done for instruments where WTO (sequence) is selected
	  {
	    sid_se_v_flags_t v_flags = (sid_se_v_flags_t)sid_se_voice[sid][0].voice_patch->B.v_flags;
	    if( v_flags.WTO )
	      sid_se_voice[sid][0].state.VOICE_ACTIVE = 1;
	  }
	  {
	    sid_se_v_flags_t v_flags = (sid_se_v_flags_t)sid_se_voice[sid][3].voice_patch->B.v_flags;
	    if( v_flags.WTO )
	      sid_se_voice[sid][3].state.VOICE_ACTIVE = 1;
	  }
	} else if( arp_mode.ENABLE ) {
	  // call Arp Note On Handler
	  SID_MIDI_ArpNoteOn(v, midi_package.note, midi_package.velocity);
	} else {
	  // push note into stack
	  NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);

	  // switch off gate if not in legato or WTO mode
	  if( !v_flags.LEGATO && !v_flags.WTO )
	    SID_MIDI_GateOff(v, mv, midi_package.note);

	  // call Note On Handler
	  SID_MIDI_NoteOn(v, midi_package.note, midi_package.velocity, v_flags);
	}
      } else { // Note Off
	// TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
	// if not in arp mode: sustain only relevant if only one active note in stack

	// branch depending on normal/arp/seq mode
	if( v_flags.WTO ) {
	  // pop from WT stack
	  SID_MIDI_PopWT(mv, midi_package.note);
	  // pop note from normal note stack
	  if( NOTESTACK_Pop(&mv->notestack, midi_package.note) > 0 ) {
	    // change sequence if there is still a note in stack
	    if( mv->notestack.len )
	      SID_MIDI_B_SetSeq(v);

	    // disable voice active flag of both voices if both are playing invalid sequences (seq off)
	    // only used in master mode
	    if( !sid_se_clk.state.SLAVE ) {
	      if( sid_se_seq[v->sid][0].seq_patch->num >= 8 && 
		  sid_se_seq[v->sid][1].seq_patch->num >= 8 ) {
		// this should only be done for instruments where WTO (sequence) is selected
		{
		  sid_se_v_flags_t v_flags = (sid_se_v_flags_t)sid_se_voice[sid][0].voice_patch->B.v_flags;
		  if( v_flags.WTO )
		    sid_se_voice[sid][0].state.VOICE_ACTIVE = 0;
		}
		{
		  sid_se_v_flags_t v_flags = (sid_se_v_flags_t)sid_se_voice[sid][3].voice_patch->B.v_flags;
		  if( v_flags.WTO )
		    sid_se_voice[sid][3].state.VOICE_ACTIVE = 0;
		}
	      }
	    }
	  }
	} else if( arp_mode.ENABLE ) {
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
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// CC Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_B_Receive_CC(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  int midi_voice;
  for(midi_voice=0; midi_voice<2; ++midi_voice, ++mv) {
    if( midi_package.chn != mv->midi_channel )
      continue; // filtered

    // TODO
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Pitchbender Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_B_Receive_PitchBender(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  int midi_voice;
  for(midi_voice=0; midi_voice<2; ++midi_voice, ++mv) {
    if( midi_package.chn != mv->midi_channel )
      continue; // filtered

    // TODO
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function which selects the sequence based on played note
/////////////////////////////////////////////////////////////////////////////
static s32 SID_MIDI_B_SetSeq(sid_se_voice_t *v)
{
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;

  // get first note from note stack
  s32 note = mv->notestack.note_items[0].note;

  // add MIDI voice based transpose value
  note += (s32)mv->transpose - 64;

  // octave-wise saturation
  while( note < 0 )
    note += 12;
  while( note >= 128 )
    note -= 12;

  // store note in voice
  v->played_note = note;

  // determine sequence based on note number
  u8 seq_number = note % 12;

  // if number >= 8: set to 8 (sequence off)
  if( seq_number >= 8 )
    seq_number = 8;

  // set new sequence in patch/shadow buffer
  sid_se_seq[v->sid][mv->midi_voice].seq_patch->num = seq_number;
  sid_se_seq[v->sid][mv->midi_voice].seq_patch_shadow->num = seq_number;
  
  return 0; // no error
}

