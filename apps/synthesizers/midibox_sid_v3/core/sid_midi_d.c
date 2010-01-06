// $Id$
/*
 * MBSID MIDI Parser for Drum Engine
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
#include "sid_midi_d.h"

#include "sid_se.h"
#include "sid_se_d.h"

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

static s32 SID_MIDI_D_SetSeq(sid_se_midi_voice_t *mv);


/////////////////////////////////////////////////////////////////////////////
// NoteOn/Off Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_D_Receive_Note(u8 sid, mios32_midi_package_t midi_package)
{
  // drum engine works only with settings of first MIDI voice
  sid_se_midi_voice_t *mv = &sid_se_midi_voice[sid][0];

  // check if MIDI channel and splitzone matches
  if( midi_package.chn != mv->midi_channel ||
      midi_package.note < mv->split_lower ||
      (mv->split_upper && midi_package.note > mv->split_upper) )
    return 0; // nothing to do

  // operation must be atomic!
  MIOS32_IRQ_Disable();

  if( midi_package.velocity ) { // Note On

    // push note into stack
    NOTESTACK_Push(&mv->notestack, midi_package.note, midi_package.velocity);

    // branch depending on SEQ/normal mode
    if( ((sid_se_seq_speed_par_t)sid_patch[sid].D.seq_speed).SEQ_ON ) {
      // copy velocity into mod matrix source
      SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

      // reset sequencer if voice was not active before
      // (only done in Clock Master mode)
      if( !sid_se_clk.state.SLAVE ) {
	if( !sid_se_seq[sid][0].state.RUNNING )
	  sid_se_vars[sid].triggers.W1R = 0; // WT Reset
      }

      // change sequence
      SID_MIDI_D_SetSeq(mv);

      // enable sequencer
      sid_se_seq[sid][0].state.RUNNING = 1;
    } else {
      int note = mv->notestack.note_items[0].note;

      // add MIDI voice based transpose value
      note += (s32)mv->transpose - 64;

      // octave-wise saturation
      while( note < 0 )
	note += 12;
      while( note >= 128 )
	note -= 12;

      // normalize note number to 24 (2 octaves)
      u8 drum = note % 24;

      // ignore drums >= 16
      if( drum < 16 ) {
	sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].D.voice[drum];
	u8 velocity_asg = voice_patch->D.velocity_asg;

	// forward velocity to parameter (if assigned)
	if( velocity_asg )
	  SID_PAR_Set16(sid, velocity_asg, midi_package.velocity << 9, 3, drum);

	// copy velocity into mod matrix source
	SID_KNOB_SetValue(sid, SID_KNOB_VELOCITY, midi_package.velocity << 1);

	// call Note On handler (located in sound engine, since it's also used by sequencer)
	SID_SE_D_NoteOn(sid, drum, midi_package.velocity);
      }
    }

  } else { // Note Off
    // branch depending on SEQ/normal mode
    if( ((sid_se_seq_speed_par_t)sid_patch[sid].D.seq_speed).SEQ_ON ) {
      if( NOTESTACK_Pop(&mv->notestack, midi_package.note) > 0 ) {
	// change sequence if there is still a note in stack
	if( mv->notestack.len )
	  SID_MIDI_D_SetSeq(mv);

	// disable voice active flag of both voices if both are playing invalid sequences (seq off)
	// only used in master mode
	if( !sid_se_clk.state.SLAVE && sid_patch[sid].D.seq_num >= 8 )
	  sid_se_seq[sid][0].state.RUNNING = 0;
      }
    } else {
      // we are able to play more drums than note stack depth, therefore no check if note in stack
      NOTESTACK_Pop(&mv->notestack, midi_package.note);

      int note = mv->notestack.note_items[0].note;

      // add MIDI voice based transpose value
      note += (s32)mv->transpose - 64;

      // octave-wise saturation
      while( note < 0 )
	note += 12;
      while( note >= 128 )
	note -= 12;

      // normalize note number to 24 (2 octaves)
      u8 drum = note % 24;

      // ignore drums >= 16
      if( drum < 16 ) {
	// call Note Off handler (located in sound engine, since it's also used by sequencer)
	SID_SE_D_NoteOff(sid, drum);
      }
    }
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// CC Handling
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_D_Receive_CC(u8 sid, mios32_midi_package_t midi_package)
{
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
  if( midi_package.chn != mv->midi_channel )
    return 0; // CC filtered

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function which selects the sequence based on played note
/////////////////////////////////////////////////////////////////////////////
static s32 SID_MIDI_D_SetSeq(sid_se_midi_voice_t *mv)
{
  // get first note from note stack
  s32 note = mv->notestack.note_items[0].note;

  // add MIDI voice based transpose value
  note += (s32)mv->transpose - 64;

  // octave-wise saturation
  while( note < 0 )
    note += 12;
  while( note >= 128 )
    note -= 12;

  // determine sequence based on note number
  u8 seq_number = note % 12;

  // if number >= 8: set to 8 (sequence off)
  if( seq_number >= 8 )
    seq_number = 8;

  // set new sequence in patch/shadow buffer
  sid_patch[mv->sid].D.seq_num = seq_number;
  sid_patch_shadow[mv->sid].D.seq_num = seq_number;
  
  return 0; // no error
}
