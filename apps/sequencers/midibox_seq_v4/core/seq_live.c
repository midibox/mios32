// $Id$
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

#include "seq_ui.h"
#include "seq_live.h"
#include "seq_core.h"
#include "seq_bpm.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_lfo.h"
#include "seq_scale.h"
#include "seq_humanize.h"
#include "seq_robotize.h"
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
seq_live_repeat_t seq_live_repeat[SEQ_LIVE_REPEAT_SLOTS];


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
  seq_live_options.KEEP_CHANNEL = 0;

  {
    int i;
    seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];    
    for(i=0; i<SEQ_LIVE_REPEAT_SLOTS; ++i, ++slot) {
      slot->enabled = 0;
      slot->repeat_ticks = 64;
      slot->chn = 0;
      slot->note = 0;
      slot->velocity = 0;
      slot->len = 71;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays an event over the given track (internal, also used by repeat function)
// bpm_tick == 0xffffffff will play the step immediately, all other ticks will schedule it
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_LIVE_PlayEventInternal(u8 track, seq_layer_evnt_t e, u32 bpm_tick)
{
  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // transpose
  //SEQ_CORE_Transpose(t, tcc, &e.midi_package);
  // conflict if same keyboard is assigned to transposer

  //initialize robotize flags
  seq_robotize_flags_t robotize_flags;
  robotize_flags.ALL = 0;

  // adding FX?
  u8 apply_force_to_scale = tcc->event_mode != SEQ_EVENT_MODE_Drum;
  if( seq_live_options.FX ) {
    SEQ_HUMANIZE_Event(track, t->step, &e);
    robotize_flags = SEQ_ROBOTIZE_Event(track, t->step, &e);
    if ( !robotize_flags.NOFX )
      SEQ_LFO_Event(track, &e);

    if( apply_force_to_scale &&
	(seq_live_options.FORCE_SCALE
#ifdef MBSEQV4L
	 || tcc->mode.FORCE_SCALE
#endif
	 )
	) {
      u8 scale, root_selection, root;
      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
      SEQ_SCALE_Note(&e.midi_package, scale, root);
    }
    SEQ_CORE_Limit(t, tcc, &e); // should be the last Fx in the chain!

  } else {
    if( apply_force_to_scale &&
	(seq_live_options.FORCE_SCALE
#ifdef MBSEQV4L
	 || tcc->mode.FORCE_SCALE
#endif
	 )
	) {
      u8 scale, root_selection, root;
      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
      SEQ_SCALE_Note(&e.midi_package, scale, root);
    }
  }

  if( bpm_tick == 0xffffffff ) {
    mios32_midi_port_t port = tcc->midi_port;
    MUTEX_MIDIOUT_TAKE;
    SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
    MIOS32_MIDI_SendPackage(port, e.midi_package);
    SEQ_MIDI_PORT_FilterOscPacketsSet(0);
    MUTEX_MIDIOUT_GIVE;
  } else {
    // Note On (the Note Off will be prepared as well in SEQ_CORE_ScheduleEvent)
    u32 scheduled_tick = bpm_tick + t->bpm_tick_delay;
    SEQ_CORE_ScheduleEvent(t, tcc, e.midi_package, SEQ_MIDI_OUT_OnOffEvent, scheduled_tick, e.len, 0, robotize_flags);
  }


  // adding echo?
  if( seq_live_options.FX ) {
    if( SEQ_BPM_IsRunning() )
      if ( !robotize_flags.NOFX )
	SEQ_CORE_Echo(t, tcc, e.midi_package, (bpm_tick == 0xffffffff) ? SEQ_BPM_TickGet() : bpm_tick, e.len, robotize_flags);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays an event over the given track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LIVE_PlayEvent(u8 track, mios32_midi_package_t p)
{
  mios32_midi_port_t port = seq_cc_trk[track].midi_port;
  u8 chn = seq_live_options.KEEP_CHANNEL ? p.chn : seq_cc_trk[track].midi_chn;

  // temporary mute matching events from the sequencer
  SEQ_CORE_NotifyIncomingMIDIEvent(track, p);

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
      u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
#if KEYBOARD_DRUM_MAPPING
	effective_note = (int)p.note % 24;
	effective_note %= SEQ_TRG_NumInstrumentsGet(track);
	// each instrument assigned to a button
	effective_note = SEQ_CC_Get(track, SEQ_CC_LAY_CONST_A1 + effective_note);
#else
	effective_note = (int)p.note; // transpose disabled in UI
#endif
      } else {
	effective_note = (int)p.note + 12*seq_live_options.OCT_TRANSPOSE;

	// ensure that note is in the 0..127 range
	effective_note = SEQ_CORE_TrimNote(effective_note, 0, 127);
      }

      // Track Repeat function:
      u8 play_note = 1;
      if( track == SEQ_UI_VisibleTrackGet() ) {
	if( event_mode != SEQ_EVENT_MODE_Drum ) {
	  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];
	  // take over note value and velocity
	  slot->chn = chn;
	  slot->note = effective_note;
	  slot->velocity = p.velocity;

	  // don't play note immediately if repeat function enabled and sequencer is running
	  if( slot->enabled && SEQ_BPM_IsRunning() )
	    play_note = 0;
	} else {
	  int i;
	  u8 num_drums = SEQ_TRG_NumInstrumentsGet(track);
	  u8 *drum_note_ptr = (u8 *)&seq_cc_trk[track].lay_const[0]; // is SEQ_CC_LAY_CONST_A1
	  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];
	  for(i=0; i<num_drums; ++i, ++slot, ++drum_note_ptr ) {
	    if( effective_note == *drum_note_ptr ) {
	      // auto-select trg layer in drum mode
	      // (also useful for other pages?)
	      if( ui_page == SEQ_UI_PAGE_TRKREPEAT ) {
		ui_selected_instrument = i;
		seq_ui_display_update_req = 1;
	      }

	      // don't play note immediately if repeat function enabled and sequencer is running
	      if( slot->enabled && SEQ_BPM_IsRunning() )
		play_note = 0;

	      // take over note value and velocity
	      slot->chn = chn;
	      slot->note = effective_note;
	      slot->velocity = p.velocity;

	      break;
	    }
	  }
	}
      }

      live_keyboard_port[p.note] = port;
      live_keyboard_chn[p.note] = chn;
      live_keyboard_note[p.note] = effective_note;

      if( play_note ) {
	seq_layer_evnt_t e;
	e.midi_package = p;
	e.midi_package.chn = chn;
	e.midi_package.note = effective_note;
	e.len = 95; // full note (only used for echo effects)
	e.layer_tag = 0;

	SEQ_LIVE_PlayEventInternal(track, e, 0xffffffff);
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

    // Aftertouch: take over velocity in repeat slot
    if( p.type == PolyPressure ) {
      int i;
      seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];    
      for(i=0; i<SEQ_LIVE_REPEAT_SLOTS; ++i, ++slot) {
	if( slot->note == p.note )
	  slot->velocity = p.velocity;
      }
    } else if( p.type == Aftertouch ) {
      int i;
      seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];    
      for(i=0; i<SEQ_LIVE_REPEAT_SLOTS; ++i, ++slot) {
	slot->velocity = p.evnt1;
      }
    }
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Plays OFF events for all notes which haven't been released yet
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LIVE_AllNotesOff(void)
{
  u8 note;
  for(note=0; note<128; ++note) {
    u32 note_ix32 = note / 32;
    u32 note_mask = (1 << (note % 32));

    if( live_note_played[note_ix32] & note_mask ) {
      // send velocity off
      MUTEX_MIDIOUT_TAKE;
      SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
      MIOS32_MIDI_SendNoteOn(live_keyboard_port[note],
			     live_keyboard_chn[note],
			     live_keyboard_note[note],
			     0x00);
      SEQ_MIDI_PORT_FilterOscPacketsSet(0);
      MUTEX_MIDIOUT_GIVE;
    }

    live_note_played[note_ix32] &= ~note_mask;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_CORE_Tick() when a new step is played
// Used for repeat function
// If this function returns 1, SEQ_CORE_Tick() shouldn't play the step (it's played here instead)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LIVE_NewStep(u8 track, u8 prev_step, u8 new_step, u32 bpm_tick)
{
  if( track != SEQ_UI_VisibleTrackGet() )
    return 0; // only for visible track

  seq_cc_trk_t *tcc = (seq_cc_trk_t *)&seq_cc_trk[track];
  u32 step_length = ((tcc->clkdiv.value+1) * (tcc->clkdiv.TRIPLETS ? 4 : 6));
  u32 step_ticks = new_step * step_length;
  u8 play_step = 1;

  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[0];    
  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    int i;

    for(i=0; i<SEQ_LIVE_REPEAT_SLOTS; ++i, ++slot) {
      if( slot->enabled && slot->note && slot->velocity ) {
	u32 repeat_ticks = slot->repeat_ticks * (tcc->clkdiv.TRIPLETS ? 4 : 6);
	if( (step_ticks % repeat_ticks) == 0 ) {

	  // note played on keyboard?
	  u32 note_ix32 = slot->note / 32;
	  u32 note_mask = (1 << (slot->note % 32));
	  if( live_note_played[note_ix32] & note_mask ) {
	    seq_layer_evnt_t e;
	    e.midi_package.ALL = 0;
	    e.midi_package.cin = NoteOn;
	    e.midi_package.event = NoteOn;
	    e.midi_package.chn = slot->chn;
	    e.midi_package.note = slot->note;
	    e.midi_package.velocity = slot->velocity;
	    e.len = slot->len + 1;
	    e.layer_tag = 0;

	    SEQ_LIVE_PlayEventInternal(track, e, bpm_tick);

	    play_step = 0; // sequencer shouldn't play the step
	  }
	}
      }
    }
  } else {
    if( slot->enabled && slot->note && slot->velocity ) {
      u32 repeat_ticks = slot->repeat_ticks * (tcc->clkdiv.TRIPLETS ? 4 : 6);
      if( (step_ticks % repeat_ticks) == 0 ) {

	// trigger all played notes
	u32 note_ix32;
	for(note_ix32=0; note_ix32<4; ++note_ix32) {
	  u32 played = live_note_played[note_ix32];
	  if( played ) {
	    u32 note_mask = 1;
	    int i;
	    for(i=0; i<32; ++i, note_mask <<= 1) {
	      if( played & note_mask ) {
		seq_layer_evnt_t e;
		e.midi_package.ALL = 0;
		e.midi_package.cin = NoteOn;
		e.midi_package.event = NoteOn;
		e.midi_package.chn = slot->chn;
		e.midi_package.note = note_ix32*32 + i;
		e.midi_package.velocity = slot->velocity;
		e.len = slot->len + 1;
		e.layer_tag = 0;

		SEQ_LIVE_PlayEventInternal(track, e, bpm_tick);

		play_step = 0; // sequencer shouldn't play the step
	      }
	    }
	  }
	}
      }
    }
  }

  //return play_step ? 0 : 1;
  // TK: disable overdubbing so that remaining trigger layers are played
  return play_step ? 0 : 0; // 0 in both cases
}
