// $Id$
/*
 * MIDI Functions for MIDIbox CV V2
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
#include <notestack.h>


#include "mbcv_midi.h"
#include "mbcv_patch.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define MBCV_MIDI_NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 mbcv_midi_dout_gate_sr[MIOS32_SRIO_NUM_SR];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// each channel has an own notestack
static notestack_t cv_notestack[MBCV_PATCH_NUM_CV];
static notestack_item_t cv_notestack_items[MBCV_PATCH_NUM_CV][MBCV_MIDI_NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MIDI_Init(u32 mode)
{
  int cv;
  for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv) {
    NOTESTACK_Init(&cv_notestack[cv],
		   NOTESTACK_MODE_PUSH_TOP,
		   &cv_notestack_items[cv][0],
		   MBCV_MIDI_NOTESTACK_SIZE);
  }

  int sr;
  for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
    mbcv_midi_dout_gate_sr[sr] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from APP_MIDI_NotifyPackage in app.c whenever a new MIDI event
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // Note Off -> Note On with velocity 0
  if( package.event == NoteOff ) {
    package.event = NoteOn;
    package.velocity = 0;
  }

  if( package.event == NoteOn ) {
    u8 cv = 0;

    // branch depending on Note On/Off event
    if( package.event == NoteOn && package.velocity > 0 ) {
      // push note into note stack
      NOTESTACK_Push(&cv_notestack[cv], package.note, package.velocity);
    } else {
      // remove note from note stack
      NOTESTACK_Pop(&cv_notestack[cv], package.note);
    }

    // still a note in stack?
    if( cv_notestack[cv].len ) {
      // take first note of stack
      // we have to convert the 7bit value to a 16bit value
      u16 note_cv = cv_notestack_items[cv][0].note << 9;
      u16 velocity_cv = cv_notestack_items[cv][0].tag << 9;

#if 0
      // change voltages
      AOUT_PinSet(cv, note_cv);
      if( aout_chn_vel >= 0 )
	AOUT_PinSet(aout_chn_vel, velocity_cv);

      // set gate pin
      if( gate_pin >= 0 )
	AOUT_DigitalPinSet(gate_pin, 1);
#endif
    } else {
#if 0
      // clear gate pin
      if( gate_pin >= 0 )
	AOUT_DigitalPinSet(gate_pin, 0);
#endif
    }
  } else if( package.event == CC ) {

  } else if( package.event == PitchBend ) {
    int pitch = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
    if( pitch > -100 && pitch < 100 )
      pitch = 0;
    //AOUT_PinPitchSet(cv, pitch);
  }

  return 0; // no error
}
