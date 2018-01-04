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
#include "seq_live.h"
#include "seq_ui.h"
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// select 1 by default to allow error messages (recommended)
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_record_options_t seq_record_options;
seq_record_state_t seq_record_state;

u8 seq_record_quantize;

// one bit for each note, 32*4 = 128 notes supported (covers all notes of one MIDI channel)
// also used by seq_core for SEQ_MIDI_OUT_ReSchedule in record mode, therefore global
u32 seq_record_played_notes[4];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// for step recording mode: timestamp to measure the length of the played
// notes (deluxe version: separate timestamp for each note!)
u16 seq_record_note_timestamp_ms[128];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Init(u32 mode)
{
  seq_record_options.ALL = 0;
  seq_record_options.AUTO_START = 0;
  seq_record_options.FWD_MIDI = 1;
  seq_record_options.POLY_RECORD = 1;
  seq_record_quantize = 10;

  seq_record_state.ALL = 0;

#ifndef MBSEQV4L
  // default for MBSEQ V4
  seq_record_state.ARMED_TRACKS = 0x0001; // first track -- currently not relevant, could be provided for MBSEQV4 later
#else
  // default for MBSEQ V4L
  seq_record_state.ARMED_TRACKS = 0x00ff; // first sequence
#endif

  SEQ_RECORD_ResetAllTracks();

  SEQ_RECORD_AllNotesOff();

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

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// resets the variables of all tracks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_ResetAllTracks(void)
{
  s32 status = 0;

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    status |= SEQ_RECORD_Reset(track);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// should be called whenever the MIDI channel or port is changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_AllNotesOff(void)
{
  int i;
  for(i=0; i<128; ++i)
    seq_record_note_timestamp_ms[i] = 0;

  for(i=0; i<4; ++i)
    seq_record_played_notes[i] = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function enables recording and takes over active live notes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Enable(u8 enable, u8 reset_timestamps)
{
  // take over live notes on transition 0->1
  if( !seq_record_state.ENABLED && enable ) {

    if( reset_timestamps ) {
      u32 timestamp = MIOS32_TIMESTAMP_Get();
      int i;
      for(i=0; i<128; ++i)
	seq_record_note_timestamp_ms[i] = timestamp;

      if( seq_record_options.FWD_MIDI ) {
	for(i=0; i<4; ++i) {
	  seq_record_played_notes[i] = seq_live_played_notes[i];
	}
      }
    }
  }

  // clear record notes if recording disabled
  if( seq_record_state.ENABLED && !enable ) {
    if( reset_timestamps ) {
      SEQ_RECORD_AllNotesOff();
    }
  }

  seq_record_state.ENABLED = enable;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function interacts with the UI to print the edit screen when an
// event has been recorded or changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_PrintEditScreen(void)
{
#ifndef MBSEQV4L
  // for 2 seconds
  ui_hold_msg_ctr = 2000;
  ui_hold_msg_ctr_drum_edit = 0; // ensure that drum triggers are displayed
#endif

  // select visible steps depending on record step
  ui_selected_step_view = ui_selected_step/16;

#ifndef MBSEQV4L
  // update immediately
  seq_ui_display_update_req = 1;
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_MIDI_IN_Receive() if MIDI event has been received on
// matching IN port and channel
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Receive(mios32_midi_package_t midi_package, u8 track)
{
  // step recording mode?
  // Note: if sequencer is not running, "Live Recording" will be handled like "Step Recording"
  u8 step_record_mode = seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning();

#if MBSEQV4L
  // extra for MBSEQ V4L: seq_record_state.ARMED_TRACKS and auto-assignment
  if( !seq_record_state.ARMED_TRACKS )
    return 0; // no track armed

  track = 0;
  if( seq_record_state.ARMED_TRACKS & 0xff00)
    track = 8;

  // search for free track/layer
  if( (midi_package.event == NoteOn) || (midi_package.event == NoteOff) ) {
    // fine, we will record Note in selected track
  } else if( midi_package.event == PitchBend ) {
    track += 3; // G1T4 resp. G3T4
  } else if( midi_package.event == CC ) {
    const u8 track_layer_cc_table[19][2] = {
      { 4, 0 },
      { 5, 0 },
      { 6, 0 },
      { 7, 0 },
      { 7, 1 }, { 7, 2 }, { 7, 3 },
      { 6, 1 }, { 6, 2 }, { 6, 3 },
      { 5, 1 }, { 5, 2 }, { 5, 3 },
      { 4, 1 }, { 4, 2 }, { 4, 3 },
      { 3, 1 }, { 3, 2 }, { 3, 3 },
    };

    // search for same (or free) CC entry
    // new track/layer search algorithm since V4L.082
    u8 seq_track_offset = track; // depends on sequence
    int par_layer = 0;
    int i;
    u8 free_layer_found = 0;
    for(i=0; i<19 && !free_layer_found; ++i) {
      track = seq_track_offset + track_layer_cc_table[i][0];
      par_layer = track_layer_cc_table[i][1];
      seq_cc_trk_t *tcc = &seq_cc_trk[track];
      u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16 + par_layer];
      u8 *layer_cc_ptr = (u8 *)&tcc->lay_const[1*16 + par_layer];

      if( *layer_type_ptr == SEQ_PAR_Type_CC &&
	  (*layer_cc_ptr >= 0x80 || *layer_cc_ptr == midi_package.cc_number) &&
	  (seq_record_state.ARMED_TRACKS & (1 << track)) ) {

	if( *layer_cc_ptr >= 0x80 ) {
	  *layer_cc_ptr = midi_package.cc_number; // assing CC number to free track

	  // initialize whole layer with invalid value 0xc0 (indicates: not recorded)
	  int num_p_steps = SEQ_PAR_NumStepsGet(track);
	  int instrument = 0;
	  int step;
	  for(step=0; step<num_p_steps; ++step)
	    SEQ_PAR_Set(track, step, par_layer, instrument, 0xc0);
#if DEBUG_VERBOSE_LEVEL >= 2
	  DEBUG_MSG("[SEQ_RECORD_Receive] free CC layer found for CC#%d in track #%d.%c\n", midi_package.cc_number, track+1, 'A'+par_layer);
#endif
	}

	free_layer_found = 1;
      }
    }

    if( !free_layer_found ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_RECORD_Receive] no free CC layer found for CC#%d\n", midi_package.cc_number);
#endif
      return 0; // no free layer
    }
  } else {
    return 0; // event not relevant
  }

  // exit if track not armed
  if( !(seq_record_state.ARMED_TRACKS & (1 << track)) )
    return 0;
#else
  // MBSEQV4 (without L)
  if( midi_package.event == CC && track == SEQ_UI_VisibleTrackGet() ) {
    // search for same (or free) CC entry
    seq_cc_trk_t *tcc = &seq_cc_trk[track];
    u8 free_layer_found = 0;
    {
      u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
      u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
      u8 *layer_cc_ptr = (u8 *)&tcc->lay_const[1*16];
      int par_layer;
      for(par_layer=0; par_layer<num_p_layers && !free_layer_found; ++par_layer, ++layer_type_ptr, ++layer_cc_ptr) {
	if( *layer_type_ptr == SEQ_PAR_Type_CC &&
	    (*layer_cc_ptr >= 0x80 || *layer_cc_ptr == midi_package.cc_number) ) {

	  if( *layer_cc_ptr >= 0x80 ) {
	    *layer_cc_ptr = midi_package.cc_number; // assing CC number to free track

	    // initialize whole layer with invalid value 0xc0 (indicates: not recorded)
	    int num_p_steps = SEQ_PAR_NumStepsGet(track);
	    int instrument = 0;
	    int step;
	    for(step=0; step<num_p_steps; ++step)
	      SEQ_PAR_Set(track, step, par_layer, instrument, 0xc0);
#if DEBUG_VERBOSE_LEVEL >= 2
	    DEBUG_MSG("[SEQ_RECORD_Receive] free CC layer found for CC#%d in track #%d.%c\n", midi_package.cc_number, track+1, 'A'+par_layer);
#endif
	  }

	  free_layer_found = 1;
	  break;
	}
      }
    }

    if( !free_layer_found ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_RECORD_Receive] no free CC layer found for CC#%d\n", midi_package.cc_number);
#endif
      return 0; // no free layer
    }
  }
#endif

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_RECORD_Receive] %02x %02x %02x -> track #%d\n", 
	    midi_package.evnt0, midi_package.evnt1, midi_package.evnt2, 
	    track+1);
#endif

  // exit if track number too high
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // unsupported track

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // Auto-Start: start with first step
  if( !SEQ_BPM_IsRunning() && seq_record_options.AUTO_START && !seq_record_options.STEP_RECORD ) {
    ui_selected_step = 0;
  }

  // branch depending on event
  u8 rec_event = 0;
  u8 send_note_off = 0;
  switch( midi_package.event ) {
    case NoteOff:
    case NoteOn: {
      midi_package.note &= 0x7f; // to avoid array overwrites
      u32 note_mask = 1 << (midi_package.note & 0x1f);

      // if Note Off and new note number matches with recorded note number
      if( midi_package.event == NoteOff || midi_package.velocity == 0 ) {
	if( seq_record_played_notes[midi_package.note>>5] & note_mask ) {
	  MIOS32_IRQ_Disable();
	  // note not active anymore
	  seq_record_played_notes[midi_package.note>>5] &= ~note_mask;

	  // determine duration in mS (for step recording function)
	  u16 duration_ms = MIOS32_TIMESTAMP_Get() - seq_record_note_timestamp_ms[midi_package.note];
	  // map to BPM
	  int duration = (int)((float)duration_ms / ((1000.0*60.0) / SEQ_BPM_EffectiveGet() / (float)SEQ_BPM_PPQN_Get()));
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[SEQ_RECORD_Receive] duration of note 0x%02x was %d mS (%d ticks)\n",
		    midi_package.note, duration_ms, duration);
#endif	  

	  // insert length into current step
	  u8 instrument = 0;
	  int len;
	  if( step_record_mode ) {
	    len = 71; // 75%
	    if( tcc->event_mode != SEQ_EVENT_MODE_Drum )
	      len = (duration <= 96) ? duration : 96; // for duration >= 96 the length will be stretched after record
	  } else {
	    len = SEQ_BPM_TickGet() - t->rec_timestamp;

	    if( len < 1 )
	      len = 1;
	    else if( len > 95 )
	      len = 95;
	  }

	  int len_step = step_record_mode ? ui_selected_step : t->step;
	  u8 num_p_layers = SEQ_PAR_NumLayersGet(track);

	  while( 1 ) {
	    if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	      // extra for MBSEQ V4L:
	      // search for note in track 1/8, insert length into track 3/10
	      int par_layer;
	      for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
		if( SEQ_PAR_Get(track, len_step, par_layer, instrument) == midi_package.note ) {
		  SEQ_PAR_Set(track+2, len_step, par_layer, instrument, len);
		  break;
		}
	      }
	    } else {
	      if( tcc->link_par_layer_length >= 0 )
		SEQ_PAR_Set(track, len_step, tcc->link_par_layer_length, instrument, len);
	    }

	    if( !step_record_mode )
	      break;
	    if( tcc->event_mode == SEQ_EVENT_MODE_Drum )
	      break;
	    if( duration <= 0 )
	      break;
	    duration -= 96;

	    // insert length into all following steps until a gate is set
	    if( ++len_step > tcc->length ) // TODO: handle this correctly if track is played backwards
	      len_step = tcc->loop;

	    if( SEQ_TRG_GateGet(track, len_step, instrument) )
	      break;

	    len = (duration > 0) ? 96 : -duration;

	    // copy notes
	    u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
	    int par_layer;
	    for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {
	      if( *layer_type_ptr == SEQ_PAR_Type_Note || *layer_type_ptr == SEQ_PAR_Type_Chord1 || *layer_type_ptr == SEQ_PAR_Type_Chord2 || *layer_type_ptr == SEQ_PAR_Type_Chord3 ) {
		u8 note = SEQ_PAR_Get(track, ui_selected_step, par_layer, instrument);
		SEQ_PAR_Set(track, len_step, par_layer, instrument, note);
	      }
	    }
	  }

	  MIOS32_IRQ_Enable();
	}

	if( step_record_mode && seq_record_options.FWD_MIDI ) {
	  // send Note Off events of current track if no key is played anymore
	  u8 any_note_played = seq_record_played_notes[0] || seq_record_played_notes[1] || seq_record_played_notes[2] || seq_record_played_notes[3];
	  if( !any_note_played )
	    send_note_off = 1;     
	}
      } else {
	MIOS32_IRQ_Disable();

	if( step_record_mode && tcc->event_mode != SEQ_EVENT_MODE_Drum ) {
	  // check if another note is already played
	  u8 any_note_played = seq_record_played_notes[0] || seq_record_played_notes[1] || seq_record_played_notes[2] || seq_record_played_notes[3];

	  // if not: clear poly counter and all notes (so that new chord can be entered if all keys were released)
	  if( !any_note_played ) {
	    t->rec_poly_ctr = 0;

	    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
	    u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
	    int par_layer;
	    u8 instrument = 0;
	    for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {
	      if( *layer_type_ptr == SEQ_PAR_Type_Note || *layer_type_ptr == SEQ_PAR_Type_Chord1 || *layer_type_ptr == SEQ_PAR_Type_Chord2 || *layer_type_ptr == SEQ_PAR_Type_Chord3 )
		SEQ_PAR_Set(track, ui_selected_step, par_layer, instrument, 0x00);
	    }
	  }
	}

	// note is active
	seq_record_played_notes[midi_package.note>>5] |= note_mask;
	// start measuring length
	t->rec_timestamp = SEQ_BPM_TickGet();
	// for step record function: independent from BPM
	seq_record_note_timestamp_ms[midi_package.note & 0x7f] = MIOS32_TIMESTAMP_Get(); // note: 16bit only

	MIOS32_IRQ_Enable();

	// record event
	rec_event = 1;
      }
    } break;

    case CC:
    case PitchBend:
    case PolyPressure:
    case Aftertouch: {
      rec_event = 1;
    } break;

    default: {
#if DEBUG_VERBOSE_LEVEL >= 2
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

  // take MIDI Out/Sequencer semaphore
  MUTEX_MIDIOUT_TAKE;

  if( send_note_off ) {
    if( SEQ_BPM_IsRunning() ) {
      SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, 0, seq_record_played_notes);
    } else {
      SEQ_MIDI_OUT_FlushQueue();
    }

    // also for the live function
    if( seq_record_options.FWD_MIDI )
      SEQ_LIVE_AllNotesOff();
  }

  if( rec_event ) {
    // start sequencer if not running and auto start enabled
    if( !SEQ_BPM_IsRunning() && seq_record_options.AUTO_START ) {
      // if in auto mode and BPM generator is clocked in slave mode:
      // change to master mode
      SEQ_BPM_CheckAutoMaster();
      // start generator
      SEQ_BPM_Start();
    }

    if( !step_record_mode ) {
      u8 prev_step = ui_selected_step;
      u8 new_step = t->step;

      ui_selected_step = new_step;

      // take next step if it will be reached "soon" (>80% of current step)
      if( SEQ_BPM_IsRunning() ) {
	u32 timestamp = SEQ_BPM_TickGet();
	u8 shift_event = 0;
	if( t->timestamp_next_step_ref <= timestamp )
	  shift_event = 1;
	else {
	  s32 diff = (s32)t->timestamp_next_step_ref - (s32)timestamp;
	  u32 tolerance = (t->step_length * seq_record_quantize) / 100; // usually 20% of 96 ticks -> 19 ticks
	  // TODO: we could vary the tolerance depending on the BPM rate: than slower the clock, than lower the tolerance
	  // as a simple replacement for constant time measuring
	  if( diff < tolerance)
	    shift_event = 1;
	}

	if( shift_event ) {
	  int next_step = ui_selected_step + 1; // tmp. variable used for u8 -> u32 conversion to handle 256 steps properly
	  if( next_step > tcc->length ) // TODO: handle this correctly if track is played backwards
	    next_step = tcc->loop;
#if DEBUG_VERBOSE_LEVEL >= 2
	  MIOS32_MIDI_SendDebugMessage("Shifted step %d -> %d\n", ui_selected_step, next_step);
#endif
	  ui_selected_step = next_step;
	  t->state.REC_DONT_OVERWRITE_NEXT_STEP = 1; // next step won't be overwritten
	}
      }

      if( ui_selected_step != prev_step )
	t->rec_poly_ctr = 0;
    }

    // insert event into track (function will return >= 0 if event
    // has been inserted into the (returned) layer.
    // if < 0, no event has been inserted.
    int insert_layer;
    if( (insert_layer=SEQ_LAYER_RecEvent(track, ui_selected_step, layer_event)) < 0 )
      rec_event = 0;
    else {
#ifndef MBSEQV4L
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
#endif

      if( step_record_mode && seq_record_options.FWD_MIDI ) {
	if( tcc->event_mode == SEQ_EVENT_MODE_Drum || midi_package.type != NoteOn ) {
	  // Drum mode or no note: play only the single event
	  SEQ_LIVE_PlayEvent(track, midi_package);
	} else {
	  u8 record_step = ui_selected_step;
#ifdef MBSEQV4L
	  // extra MBSEQ V4L if CC track: read 16th step in step record mode
	  // could also be provided for normal MBSEQ V4 later (has to be made more flexible!)
	  if( step_record_mode && tcc->clkdiv.value == 0x03 )
	    record_step *= 4;
#endif

#ifdef MBSEQV4P
	  seq_layer_evnt_t layer_events[83];
	  s32 number_of_events = SEQ_LAYER_GetEventsPlus(track, record_step, layer_events, 0);
#else
	  seq_layer_evnt_t layer_events[16];
	  s32 number_of_events = SEQ_LAYER_GetEvents(track, record_step, layer_events, 0);
#endif
	  if( number_of_events > 0 ) {
	    int i;
	    seq_layer_evnt_t *e = &layer_events[0];
	    for(i=0; i<number_of_events; ++e, ++i)
	      SEQ_LIVE_PlayEvent(track, e->midi_package);
	  }
	}
      }

      if( step_record_mode && !seq_record_options.POLY_RECORD ) {
	int next_step = (ui_selected_step + seq_record_options.STEPS_PER_KEY) % ((int)tcc->length+1);

	int i;
	for(i=0; i<SEQ_CORE_NUM_TRACKS; ++i)
	  SEQ_RECORD_Reset(i);

	ui_selected_step = next_step;
      }
    }
  }

  if( seq_record_options.FWD_MIDI && (!rec_event || !step_record_mode) ) {
    // forward event directly in live mode or if it hasn't been recorded
    SEQ_LIVE_PlayEvent(track, layer_event.midi_package);
  }

  // give MIDI Out/Sequencer semaphore
  MUTEX_MIDIOUT_GIVE;

  // temporary print edit screen
  // MBSEQV4L: only if a note ON event has been played
#ifdef MBSEQV4L
  if( rec_event && midi_package.event == NoteOn ) {
#else
  if( rec_event ) {
#endif
    SEQ_RECORD_PrintEditScreen();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from SEQ_CORE_Tick() when a new step is played
// It can modify the current and previous step depending on recording mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_NewStep(u8 track, u8 prev_step, u8 new_step, u32 bpm_tick)
{
  if( !seq_record_state.ENABLED )
    return -1; // not in recording page

  if( seq_record_options.STEP_RECORD )
    return -2; // only in live record mode

  // this is a proper workaround for Auto Start feature in Live Recording mode
  // t->state.REC_DONT_OVERWRITE_NEXT_STEP has been cleared by SEQ_CORE_Reset(), this
  // will cause that the first step will be overwritten
  if( bpm_tick == 0 )
    return 0; // silently ignore (we don't expect a live recording event at the very first step)

#ifndef MBSEQV4L
  if( track != SEQ_UI_VisibleTrackGet() )
    return -3; // only for visible track
#else
  if( !(seq_record_state.ARMED_TRACKS & (1 << track)) )
    return -3; // MBSEQV4L: only for armed track
#endif

  if( track >= SEQ_CORE_NUM_TRACKS )
    return -4; // unsupported track

#ifndef MBSEQV4L
  // new: if CLEAR key pressed, clear the step
  if( seq_ui_button_state.CLEAR ) {
    SEQ_UI_UTIL_ClearStep(track, new_step, ui_selected_instrument);
    ui_selected_step = new_step;
    ui_selected_step_view = ui_selected_step/16;
    return 0; // no error
  }
#endif

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // take over new timestamp
  t->rec_timestamp = bpm_tick;

  // branch depending on drum/normal mode
  u8 instrument = 0;
  u8 any_note_played = seq_record_played_notes[0] || seq_record_played_notes[1] || seq_record_played_notes[2] || seq_record_played_notes[3];

  // if at least one note is played and step can be overwritten
  if( any_note_played && !t->state.REC_DONT_OVERWRITE_NEXT_STEP ) {

    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
      // check for matching notes
      u8 num_instruments = SEQ_TRG_NumInstrumentsGet(track);
      for(instrument=0; instrument<num_instruments; ++instrument) {
	u8 note = tcc->lay_const[0*16 + instrument];
	if( seq_record_played_notes[note>>5] & (1 << (note & 0x1f)) ) {
	  u8 gate = 0;
	  u8 accent = 0;
	  // BEGIN live pattern insertion
	  if( !seq_record_options.STEP_RECORD ) {
	    seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[1+instrument];
	    if( slot->enabled ) {
	      seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
	      u16 mask = 1 << (new_step % 16);
	      gate = (pattern->gate & mask) ? 1 : 0;
	      accent = (pattern->accent & mask) ? 1 : 0;

	      if( tcc->link_par_layer_velocity >= 0 ) {
		SEQ_PAR_Set(track, new_step, tcc->link_par_layer_velocity, instrument, slot->velocity);
	      }
	    }
	  }
	  // END live pattern insertion

	  SEQ_TRG_GateSet(track, new_step, instrument, gate);
	  SEQ_TRG_AccentSet(track, new_step, instrument, accent);

#ifndef MBSEQV4L
	  // UI: display recorded step
	  ui_selected_step = new_step;
	  ui_selected_step_view = ui_selected_step/16;
#endif
	}
      }
    } else {
      u8 gate = 0;
      u8 accent = 0;
      u8 length_prev_step = 95;
      u8 length_new_step = 1;
      int velocity = -1; // take over new velocity if >= 0
      // BEGIN live pattern insertion
      if( !seq_record_options.STEP_RECORD ) {
	seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[0];
	if( slot->enabled ) {
	  seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
	  u16 mask = 1 << (new_step % 16);
	  gate = (pattern->gate & mask) ? 1 : 0;
	  accent = (pattern->accent & mask) ? 1 : 0;
	  velocity = slot->velocity;
	  length_prev_step = slot->len;
	  length_new_step = slot->len;
	}
      }
      // END live pattern insertion

      // disable gate of new step
      SEQ_TRG_GateSet(track, new_step, instrument, gate);
      SEQ_TRG_AccentSet(track, new_step, instrument, accent);

#ifndef MBSEQV4L
      // UI: display recorded step
      ui_selected_step = new_step;
      ui_selected_step_view = ui_selected_step/16;
#endif

      // copy notes of previous step to new step
      u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
      u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
      int par_layer;
      for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {
	if( *layer_type_ptr == SEQ_PAR_Type_Note || *layer_type_ptr == SEQ_PAR_Type_Chord1 || *layer_type_ptr == SEQ_PAR_Type_Chord2 || *layer_type_ptr == SEQ_PAR_Type_Chord3 ) {
	  u8 note = SEQ_PAR_Get(track, prev_step, par_layer, instrument);
	  SEQ_PAR_Set(track, new_step, par_layer, instrument, note);
	}
      }

      // set length of previous step to maximum, and of the new step to minimum
      if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	// extra for MBSEQ V4L:
	// search for note in track 1/8, insert length into track 3/10
	for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	  u8 note = SEQ_PAR_Get(track, prev_step, par_layer, instrument);
	  if( seq_record_played_notes[note>>5] & (1 << (note&0x1f)) ) {
	    SEQ_PAR_Set(track+2, prev_step, par_layer, instrument, length_prev_step);
	    SEQ_PAR_Set(track+2, new_step, par_layer, instrument, length_new_step);
	  }
	}

	// insert velocity into track 2/9
	if( velocity >= 0 ) {
	  SEQ_PAR_Set(track+1, new_step, par_layer, instrument, velocity);
	}
      } else {
	if( tcc->link_par_layer_length >= 0 ) {
	  SEQ_PAR_Set(track, prev_step, tcc->link_par_layer_length, instrument, length_prev_step);
	  SEQ_PAR_Set(track, new_step, tcc->link_par_layer_length, instrument, length_new_step);
	}
	if( velocity >= 0 && tcc->link_par_layer_velocity >= 0 ) {
	  SEQ_PAR_Set(track, new_step, tcc->link_par_layer_velocity, instrument, velocity);
	}
      }
    }
  }

  t->state.REC_DONT_OVERWRITE_NEXT_STEP = 0;

  return 0; // no error
}
