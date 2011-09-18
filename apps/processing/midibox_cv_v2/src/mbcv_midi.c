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

// these variables track MIDI events and are mapped to CV channels in MBCV_MAP_UpdateChannels()

u32 mbcv_midi_gates; // prepared for up to 32 channels
u8  mbcv_midi_gateclr_ctr[MBCV_PATCH_NUM_CV]; // for mono/poly mode: gate cleared for given time - decremented in MBCV_MAP_UpdateChannels()

u8  mbcv_midi_note[MBCV_PATCH_NUM_CV];
u8  mbcv_midi_velocity[MBCV_PATCH_NUM_CV];
u8  mbcv_midi_cc[MBCV_PATCH_NUM_CV];
u16 mbcv_midi_nrpn[MBCV_PATCH_NUM_CV];
u8  mbcv_midi_aftertouch[MBCV_PATCH_NUM_CV];
s16 mbcv_midi_pitch[MBCV_PATCH_NUM_CV];


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

    mbcv_midi_gateclr_ctr[cv] = 0;
    mbcv_midi_note[cv] = 0x3c; // "mid" value (C-3)
    mbcv_midi_velocity[cv] = 0x40; // mid value
    mbcv_midi_cc[cv] = 0x40; // mid value
    mbcv_midi_nrpn[cv] = 0x2000; // mid value
    mbcv_midi_aftertouch[cv] = 0x40; // mid value
    mbcv_midi_pitch[cv] = 0; // mid value
  }

  mbcv_midi_gates = 0;

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

  // create port mask
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0xc)>>2;
  u8 port_mask = subport_mask << (4*port_class);

  if( package.event == NoteOn ) {

    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      // check for matching port/channel/note range
      if( !(cv_cfg->enabled_ports & port_mask) )
	continue;
      if( !cv_cfg->chn || (package.chn != (cv_cfg->chn-1)) )
	continue;
      if( package.note < cv_cfg->split_l || package.note > cv_cfg->split_u )
	continue;

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
	mbcv_midi_note[cv] = cv_notestack_items[cv][0].note;

	// store velocity if > 0
	u8 velocity = cv_notestack_items[cv][0].tag;
	if( velocity )
	  mbcv_midi_velocity[cv] = velocity;

	// gate already set and MONO or POLY mode: trigger gate for given number of cycles
	if( (mbcv_midi_gates & (1 << cv)) &&
	    (!cv_cfg->midi_mode.LEGATO || cv_cfg->midi_mode.POLY) )
	  mbcv_midi_gateclr_ctr[cv] = mbcv_patch_gateclr_cycles;

	// set gate pin
        mbcv_midi_gates |= (1 << cv);
      } else {
	// clear gate pin
	mbcv_midi_gates &= ~(1 << cv);
      }
    }

  } else if( package.event == PolyPressure ) {
    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      // check for matching port/channel/note range
      if( !(cv_cfg->enabled_ports & port_mask) )
	continue;
      if( !cv_cfg->chn || (package.chn != (cv_cfg->chn-1)) )
	continue;
      if( package.note < cv_cfg->split_l || package.note > cv_cfg->split_u )
	continue;

      mbcv_midi_aftertouch[cv] = package.value2;
    }

  } else if( package.event == CC ) {
    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      // check for matching port/channel/CC number
      if( !(cv_cfg->enabled_ports & port_mask) )
	continue;
      if( !cv_cfg->chn || (package.chn != (cv_cfg->chn-1)) )
	continue;
      if( package.cc_number != cv_cfg->cc_number )
	continue;

      mbcv_midi_cc[cv] = package.value;
    }

  } else if( package.event == Aftertouch ) {
    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      // check for matching port/channel
      if( !(cv_cfg->enabled_ports & port_mask) )
	continue;
      if( !cv_cfg->chn || (package.chn != (cv_cfg->chn-1)) )
	continue;

      mbcv_midi_aftertouch[cv] = package.value1;
    }

  } else if( package.event == PitchBend ) {
    int cv;
    mbcv_patch_cv_entry_t *cv_cfg = (mbcv_patch_cv_entry_t *)&mbcv_patch_cv[0];
    for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv, ++cv_cfg) {
      // check for matching port/channel
      if( !(cv_cfg->enabled_ports & port_mask) )
	continue;
      if( !cv_cfg->chn || (package.chn != (cv_cfg->chn-1)) )
	continue;

      int pitch = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
      if( pitch > -100 && pitch < 100 )
	pitch = 0;

      mbcv_midi_pitch[cv] = pitch;
    }
  }

  return 0; // no error
}
