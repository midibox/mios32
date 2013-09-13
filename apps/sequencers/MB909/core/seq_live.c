// $Id: seq_live.c 1470 2012-04-16 21:30:30Z tk $
/*
 * Live Play Functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"

#include "seq_live.h"
#include "seq_core.h"
#include "seq_bpm.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_lfo.h"
#include "seq_scale.h"
#include "seq_humanize.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
// TK: Mapping disabled, it confuses too much and doesn't work in conjunction with recording
#define KEYBOARD_DRUM_MAPPING 0


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
seq_live_options_t seq_live_options;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
// for tracking played notes
#define LIVE_NUM_KEYS 128
static u32 live_note_played[LIVE_NUM_KEYS / 32];
static mios32_midi_port_t live_keyboard_port[LIVE_NUM_KEYS];
static u8 live_keyboard_chn[LIVE_NUM_KEYS];
static u8 live_keyboard_note[LIVE_NUM_KEYS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LIVE_Init(u32 mode)
{
  seq_live_options.ALL = 0;
  seq_live_options.OCT_TRANSPOSE = 0;
  seq_live_options.VELOCITY = 100;
  seq_live_options.FORCE_SCALE = 0;
  seq_live_options.FX = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays an event over the given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LIVE_PlayEvent(u8 track, mios32_midi_package_t p)
{
  mios32_midi_port_t port = seq_cc_trk[track].midi_port;
  u8 chn = seq_cc_trk[track].midi_chn;

  // Note Events:
  if( p.type == NoteOff ) {
    p.type = NoteOn;
    p.velocity = 0;
  }

  if( p.type == NoteOn ) {
    u32 note_ix32 = p.note / 32;
    u32 note_mask = (1 << (p.note % 32));

    // in any case (key depressed or not), play note off if note is active!
    // this ensures that note off event is sent if for example the OCT_TRANSPOSE has been changed
    if( live_note_played[note_ix32] & note_mask ) {
      // send velocity off
      MUTEX_MIDIOUT_TAKE;
      SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
      MIOS32_MIDI_SendNoteOn(live_keyboard_port[p.note],
			     live_keyboard_chn[p.note],
			     live_keyboard_note[p.note],
			     0x00);
      SEQ_MIDI_PORT_FilterOscPacketsSet(0);
      MUTEX_MIDIOUT_GIVE;
    }

    if( p.velocity == 0 ) {
      live_note_played[note_ix32] &= ~note_mask;
    } else {
      live_note_played[note_ix32] |= note_mask;

      int effective_note;
#if KEYBOARD_DRUM_MAPPING
      u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	effective_note = (int)p.note % 24;
	effective_note %= SEQ_TRG_NumInstrumentsGet(track);
	// each instrument assigned to a button
	effective_note = SEQ_CC_Get(track, SEQ_CC_LAY_CONST_A1 + effective_note);
      } else {
#endif
	effective_note = (int)p.note + 12*seq_live_options.OCT_TRANSPOSE;
	while( effective_note < 0   ) effective_note += 12;
	while( effective_note > 127 ) effective_note -= 12;
#if KEYBOARD_DRUM_MAPPING
      }
#endif

      seq_layer_evnt_t e;
      e.midi_package = p;
      e.midi_package.chn = chn;
      e.midi_package.note = effective_note;
      e.len = 95; // full note (only used for echo effects)
      e.layer_tag = 0;

      seq_core_trk_t *t = &seq_core_trk[track];
      seq_cc_trk_t *tcc = &seq_cc_trk[track];

      // transpose
      //SEQ_CORE_Transpose(t, tcc, &e.midi_package);
      // conflict if same keyboard is assigned to transposer

      // adding FX?
      if( seq_live_options.FX ) {
	SEQ_HUMANIZE_Event(track, t->step, &e);
	SEQ_LFO_Event(track, &e);

	if( seq_live_options.FORCE_SCALE
#ifdef MBSEQV4L
	    || tcc->mode.FORCE_SCALE
#endif
	    ) {
	  u8 scale, root_selection, root;
	  SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
	  SEQ_SCALE_Note(&e.midi_package, scale, root);
	}
	SEQ_CORE_Limit(t, tcc, &e); // should be the last Fx in the chain!

      } else {
	if( seq_live_options.FORCE_SCALE
#ifdef MBSEQV4L
	    || tcc->mode.FORCE_SCALE
#endif
	    ) {
	  u8 scale, root_selection, root;
	  SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
	  SEQ_SCALE_Note(&e.midi_package, scale, root);
	}
      }

      live_keyboard_port[p.note] = port;
      live_keyboard_chn[p.note] = chn;
      live_keyboard_note[p.note] = e.midi_package.note;

      MUTEX_MIDIOUT_TAKE;
      SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
      MIOS32_MIDI_SendPackage(port, e.midi_package);
      SEQ_MIDI_PORT_FilterOscPacketsSet(0);
      MUTEX_MIDIOUT_GIVE;

      // adding echo?
      if( seq_live_options.FX ) {
	if( SEQ_BPM_IsRunning() )
	  SEQ_CORE_Echo(t, tcc, e.midi_package, SEQ_BPM_TickGet(), e.len);
      }

    }
  } else if( p.type >= 0x8 && p.type <= 0xe ) {

    // just forward event over right channel...
    p.chn = chn;
    MUTEX_MIDIOUT_TAKE;
    SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
    MIOS32_MIDI_SendPackage(port, p);
    SEQ_MIDI_PORT_FilterOscPacketsSet(0);
    MUTEX_MIDIOUT_GIVE;
  }


  return 0; // no error
}
