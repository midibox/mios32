// $Id$
/*
 * Sequencer Recording Routines
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
#include <string.h>
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "seq_record.h"
#include "seq_core.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_cc.h"
#include "seq_ui.h"
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_record_options_t seq_record_options;
seq_record_state_t seq_record_state;

u8 seq_record_step;


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// one bit for each note, 32*4 = 128 notes supported (covers all notes of one MIDI channel)
u32 played_notes[4];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Init(u32 mode)
{
  seq_record_options.ALL = 0;
  seq_record_options.AUTO_START = 1;

  seq_record_state.ALL = 0;

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    SEQ_RECORD_Reset(track);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called whenever record variables should be reseted (e.g. on track restart)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Reset(u8 track)
{
  // exit if track number too high
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // unsupported track

  seq_core_trk_t *t = &seq_core_trk[track];

  // init track specific variables
  MIOS32_IRQ_Disable();
  t->state.REC_DONT_OVERWRITE_NEXT_STEP = 0;
  t->rec_timestamp = 0;
  t->rec_poly_ctr = 0;
  MIOS32_IRQ_Enable();

  // used for all tracks
  seq_record_step = 0;

  int i;
  for(i=0; i<4; ++i)
    played_notes[i] = 0;
 
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function interacts with the UI to print the edit screen when an
// event has been recorded or changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_PrintEditScreen(void)
{
  // for 2 seconds
  ui_hold_msg_ctr = 2000;

  // select visible steps depending on record step
  ui_selected_step = seq_record_step;
  ui_selected_step_view = ui_selected_step/16;

  // update immediately
  seq_ui_display_update_req = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_MIDI_IN_Receive() if MIDI event has been received on
// matching IN port and channel
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Receive(mios32_midi_package_t midi_package, u8 track)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_RECORD_Receive] %02x %02x %02x -> track #%d\n", 
	    midi_package.evnt0, midi_package.evnt1, midi_package.evnt2, 
	    track+1);
#endif

  // exit if track number too high
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // unsupported track

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // branch depending on event
  u8 rec_event = 0;
  u8 send_note_off = 0;
  switch( midi_package.event ) {
    case NoteOff:
    case NoteOn: {
      midi_package.note &= 0x7f; // to avoid array overwrites
      u8 note_mask = 1 << (midi_package.note & 0x1f);

      // if Note Off and new note number matches with recorded note number
      if( midi_package.event == NoteOff || midi_package.velocity == 0 ) {
	if( played_notes[midi_package.note>>5] & note_mask ) {
	  MIOS32_IRQ_Disable();
	  // note not active anymore
	  played_notes[midi_package.note>>5] &= ~note_mask;

	  // insert length into current step
	  u8 instrument = 0;
	  if( tcc->link_par_layer_length >= 0 ) {
	    int len = SEQ_BPM_TickGet() - t->rec_timestamp;
	    if( len < 1 )
	      len = 1;
	    else if( len > 95 )
	      len = 95;
	    SEQ_PAR_Set(track, t->step, tcc->link_par_layer_length, instrument, len);
	  }
	  MIOS32_IRQ_Enable();
	}

	// send Note Off events of current track
	send_note_off = 1;
      } else {
	MIOS32_IRQ_Disable();
	// note is active
	played_notes[midi_package.note>>5] |= note_mask;
	// start measuring length
	t->rec_timestamp = SEQ_BPM_TickGet();
	MIOS32_IRQ_Enable();

	// send Note Off events of current track
	send_note_off = 1;

	// record event
	rec_event = 1;
      }
    } break;

    case CC:
    case PitchBend: {
      rec_event = 1;
    } break;

    default: {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_RECORD_Receive] event %x not supported.\n", midi_package.event);
#endif
      return -2; // unsupported event
    }
  }

  // create step event
  seq_layer_evnt_t layer_event;
  layer_event.midi_package = midi_package;
  layer_event.midi_package.cable = track; // used as tag
  layer_event.len = 71; // 75%
  layer_event.midi_package.chn = tcc->midi_chn;
  mios32_midi_port_t rec_port = tcc->midi_port;

  // take MIDI Out/Sequencer semaphore
  MUTEX_MIDIOUT_TAKE;

  if( send_note_off ) {
    if( SEQ_BPM_IsRunning() )
      SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, 0);
    else
      SEQ_MIDI_OUT_FlushQueue();
  }

  if( rec_event ) {
    u8 dont_play_step_now = 0;
    if( !seq_record_options.STEP_RECORD ) {
      u8 prev_step = seq_record_step;
      u8 new_step = t->step;

      seq_record_step = new_step;

      // take next step if it will be reached "soon" (>80% of current step)
      if( SEQ_BPM_IsRunning() ) {
	u32 timestamp = SEQ_BPM_TickGet();
	u8 shift_event = 0;
	if( t->timestamp_next_step_ref <= timestamp )
	  shift_event = 1;
	else {
	  s32 diff = (s32)t->timestamp_next_step_ref - (s32)timestamp;
	  u32 tolerance = (t->step_length * 20) / 100; // usually 20% of 96 ticks -> 19 ticks
	  // TODO: we could vary the tolerance depending on the BPM rate: than slower the clock, than lower the tolerance
	  // as a simple replacement for constant time measuring
	  if( diff < tolerance)
	    shift_event = 1;
	}

	if( shift_event ) {
	  int next_step = seq_record_step + 1; // tmp. variable used for u8 -> u32 conversion to handle 256 steps properly
	  if( next_step > tcc->length ) // TODO: handle this correctly if track is played backwards
	    next_step = tcc->loop;
#if DEBUG_VERBOSE_LEVEL >= 0
	  MIOS32_MIDI_SendDebugMessage("Shifted step %d -> %d\n", seq_record_step, next_step);
#endif
	  seq_record_step = next_step;
	  t->state.REC_DONT_OVERWRITE_NEXT_STEP = 1; // next step won't be overwritten
	  dont_play_step_now = 1; // next step will be played be sequencer, and not immediately
	}
      }

      if( seq_record_step != prev_step )
	t->rec_poly_ctr = 0;
    }

    // insert event into track (function will return >= 0 if event
    // has been inserted into the (returned) layer.
    // if < 0, no event has been inserted.
    int insert_layer;
    if( (insert_layer=SEQ_LAYER_RecEvent(track, seq_record_step, layer_event)) < 0 )
      rec_event = 0;
    else {
      // change layer on UI
      if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
	ui_selected_instrument = insert_layer;
	ui_selected_par_layer = 0;
	ui_selected_trg_layer = 0;
      } else {
	ui_selected_instrument = 0;
	ui_selected_par_layer = insert_layer;
	ui_selected_trg_layer = 0;
      }

      // start sequencer if not running and auto start enabled
      if( !SEQ_BPM_IsRunning() && seq_record_options.AUTO_START ) {
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();
	// start generator
	SEQ_BPM_Start();
      }

      if( !dont_play_step_now ) {
        seq_layer_evnt_t layer_events[16];
        s32 number_of_events = SEQ_LAYER_GetEvents(track, seq_record_step, layer_events, 0);
        if( number_of_events > 0 ) {
	  int i;
          seq_layer_evnt_t *e = &layer_events[0];
          for(i=0; i<number_of_events; ++e, ++i) {
	    // send event immediately
	    MIOS32_MIDI_SendPackage(rec_port, e->midi_package);

	    // if note: queue off event
	    if( e->midi_package.event == NoteOn ) {
	      e->midi_package.velocity = 0;
	      SEQ_MIDI_OUT_Send(rec_port, e->midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
	    }
	  }
	}
      }

      if( seq_record_options.STEP_RECORD && !seq_record_options.POLY_RECORD ) {
	int next_step = seq_record_step + 1; // tmp. variable used for u8 -> u32 conversion to handle 256 steps properly
	if( next_step > tcc->length )
	  next_step = tcc->loop;

	int i;
	for(i=0; i<SEQ_CORE_NUM_TRACKS; ++i)
	  SEQ_RECORD_Reset(i);

	seq_record_step = next_step;
      }
    }

  } else { // !rec_event
    // forward event directly if it hasn't been recorded
    MIOS32_MIDI_SendPackage(rec_port, layer_event.midi_package);
  }

  // give MIDI Out/Sequencer semaphore
  MUTEX_MIDIOUT_GIVE;

  // temporary print edit screen
  SEQ_RECORD_PrintEditScreen();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_CORE_Tick() when a new step is played
// It can modify the current and previous step depending on recording mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_NewStep(u8 track, u8 prev_step, u8 new_step, u32 bpm_tick)
{
  if( ui_page != SEQ_UI_PAGE_TRKREC )
    return -1; // not in recording page

  if( seq_record_options.STEP_RECORD )
    return -2; // only in live record mode

  if( track != SEQ_UI_VisibleTrackGet() )
    return -3; // only for visible track

  if( track >= SEQ_CORE_NUM_TRACKS )
    return -4; // unsupported track

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // take over new timestamp
  t->rec_timestamp = bpm_tick;

  // branch depending on drum/normal mode
  u8 instrument = 0;
  u8 any_note_played = played_notes[0] || played_notes[1] || played_notes[2] || played_notes[3];

  // if at least one note is played and step can be overwritten
  if( any_note_played && !t->state.REC_DONT_OVERWRITE_NEXT_STEP ) {

    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
      // check for matching notes
      u8 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
      for(instrument=0; instrument<num_instruments; ++instrument) {
	u8 note = tcc->lay_const[0*16 + instrument];
	if( played_notes[note>>5] & (1 << (note & 0x1f)) ) {
	  // disable gate of new step
	  SEQ_TRG_GateSet(track, new_step, instrument, 0);
	}
      }
    } else {
      // set length of previous step to maximum, and of the new step to minimum
      if( tcc->link_par_layer_length >= 0 ) {
	SEQ_PAR_Set(track, prev_step, tcc->link_par_layer_length, instrument, 95);
	SEQ_PAR_Set(track, new_step, tcc->link_par_layer_length, instrument, 1);
      }

      // disable gate of new step
      SEQ_TRG_GateSet(track, new_step, instrument, 0);
    }

  }

  t->state.REC_DONT_OVERWRITE_NEXT_STEP = 0;

  return 0; // no error
}
