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

#include "seq_core.h"
#include "seq_record.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
#define DEBUG_VERBOSE_LEVEL 2


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_record_options_t seq_record_options;
seq_record_state_t seq_record_state;

u8 seq_record_step;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 last_recorded_note;
static u16 length_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Init(u32 mode)
{
  seq_record_options.ALL = 0;
  seq_record_options.AUTO_START = 1;

  seq_record_state.ALL = 0;

  SEQ_RECORD_Reset();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called whenever record variables should be reseted (e.g. on track restart)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_RECORD_Reset(void)
{
  seq_record_step = 0;
  last_recorded_note = 0;
  length_ctr = 0;
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
  u8 rec_event = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_RECORD_Receive] %02x %02x %02x -> track #%d\n", 
	    midi_package.evnt0, midi_package.evnt1, midi_package.evnt2, 
	    track+1);
#endif

  // exit if track number too high
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // unsupported track

  // branch depending on event
  switch( midi_package.event ) {
    case NoteOff:
    case NoteOn: {
      // if Note Off and new note number matches with last recorded note number
      if( midi_package.event == NoteOff || midi_package.velocity == 0 ) {
	if(  midi_package.note == last_recorded_note ) {
	  MIOS32_IRQ_Disable();
	  // event not active anymore
	  seq_core_trk[track].state.REC_EVENT_ACTIVE = 0;
	  // next played step should be muted
	  seq_core_trk[track].state.REC_MUTE_NEXT_STEP = 1;
	  MIOS32_IRQ_Enable();
	}

	// send Note Off events of current track
	SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, 0);
      } else {
	MIOS32_IRQ_Disable();
	// store note number
	last_recorded_note = midi_package.note;
	// event is active
	seq_core_trk[track].state.REC_EVENT_ACTIVE = 1;
	// start measuring length
	length_ctr = 0;
	MIOS32_IRQ_Enable();

	// record event
	rec_event = 1;
      }
    } break;

    case CC: {
      rec_event = 1;
    } break;

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

  if( rec_event ) {
    // send Note Off events of current track
    // TODO: different in poly recording mode
    SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, 0);

    // start sequencer if not running and auto start enabled
    if( !SEQ_BPM_IsRunning() && seq_record_options.AUTO_START )
      SEQ_BPM_Start();

  }

  // temporary print edit screen
  SEQ_RECORD_PrintEditScreen();

  return 0; // no error
}
