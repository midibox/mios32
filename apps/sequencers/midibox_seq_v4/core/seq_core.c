// $Id$
/*
 * Sequencer Core Routines
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

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_midi.h"
#include "seq_midi_in.h"
#include "seq_bpm.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"
#include "seq_random.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// set this to 1 if performance of clock handler should be measured with a scope
// (LED toggling in APP_Background() has to be disabled!)
// set this to 2 to visualize forward delay during pattern changes
#define LED_PERFORMANCE_MEASURING 2


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_CORE_Tick(u32 bpm_tick, u8 no_echo);

static s32 SEQ_CORE_ResetTrkPos(seq_core_trk_t *t, seq_cc_trk_t *tcc);
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, s32 reverse);
static s32 SEQ_CORE_Transpose(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *midi_package);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_core_state_t seq_core_state;
seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];

u8 steps_per_measure;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 bpm_tick_forward_delay_ctr;
static u32 bpm_tick_forward_delay_ctr_req;
static u32 bpm_tick_forwarded;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Init(u32 mode)
{
  seq_core_state.ALL = 0;
  bpm_tick_forward_delay_ctr_req = 0;
  bpm_tick_forward_delay_ctr = 0;
  bpm_tick_forwarded = 0;

  steps_per_measure = 16; // will be configurable later

  // reset track parameters
  SEQ_CC_Init(0);

  // reset layers
  SEQ_LAYER_Init(0);

  // reset patterns
  SEQ_PATTERN_Init(0);

  // clear registers which are not reset by SEQ_CORE_Reset()
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    t->sustain_note.ALL = 0;
  }

  // reset sequencer
  SEQ_CORE_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

  SEQ_BPM_PPQN_Set(384);
  SEQ_BPM_Set(1400); // for 140.0 BPM

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Handler(void)
{
  // handle requests

  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    if( SEQ_BPM_ChkReqStop() )
      SEQ_CORE_Stop(1);

    if( SEQ_BPM_ChkReqCont() )
      SEQ_CORE_Cont(1);

    if( SEQ_BPM_ChkReqStart() )
      SEQ_CORE_Start(1);

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) )
      SEQ_CORE_SongPos(new_song_pos, 1);

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      if( seq_core_state.RUN && !seq_core_state.PAUSE ) {

	// TODO: forwarding mechanism can be simplified
	// it's better to do it the same way like in the MIDI file player
	if( bpm_tick_forward_delay_ctr ) {
	  --bpm_tick_forward_delay_ctr;
#if LED_PERFORMANCE_MEASURING == 2
	  MIOS32_BOARD_LED_Set(0xffffffff, bpm_tick_forward_delay_ctr ? 1 : 0);
#endif
	} else if( bpm_tick_forward_delay_ctr_req ) {
#if LED_PERFORMANCE_MEASURING == 2
	  MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif
	  // forwarding sequencer has been requested!
	  bpm_tick_forwarded = bpm_tick;
	  while( bpm_tick_forward_delay_ctr_req ) {
	    ++bpm_tick_forward_delay_ctr;
	    --bpm_tick_forward_delay_ctr_req;
	    SEQ_CORE_Tick(bpm_tick_forwarded++, 1);
	  }
	} else {
#if LED_PERFORMANCE_MEASURING == 1
	  MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif
	  SEQ_CORE_Tick(bpm_tick, 1);
#if LED_PERFORMANCE_MEASURING == 1
	  MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif
	}
      }
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_FlushQueue();

  // play sustained notes
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    if( t->sustain_note.ALL ) {
      MIOS32_MIDI_SendPackage(t->sustain_port, t->sustain_note);
      t->sustain_note.ALL = 0;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Reset(void)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE = 0;
  seq_core_state.FIRST_CLK = 1;

  int track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    seq_cc_trk_t *tcc = &seq_cc_trk[track];

    t->state.ALL = 0;
    SEQ_CORE_ResetTrkPos(t, tcc);
  }

  // init bpm tick
  SEQ_BPM_TickSet(0);

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_CORE_PlayOffEvents();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Starts the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Start(u8 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  SEQ_CORE_Reset();
  seq_core_state.RUN = 1;
  return no_echo ? 0 : SEQ_BPM_Start();
}


/////////////////////////////////////////////////////////////////////////////
// Stops the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Stop(u8 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE = 0;
  seq_core_state.RUN = 0;
  SEQ_CORE_PlayOffEvents();
  return no_echo ? 0 : SEQ_BPM_Stop();
}


/////////////////////////////////////////////////////////////////////////////
// Continues the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Cont(u8 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE = 0;
  seq_core_state.RUN = 1;
  return no_echo ? 0 : SEQ_BPM_Start();
}



/////////////////////////////////////////////////////////////////////////////
// Pauses sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Pause(u8 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE ^= 1;
  if( seq_core_state.PAUSE )
    SEQ_CORE_PlayOffEvents();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets new song position (new_song_pos resolution: 16th notes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_SongPos(u16 new_song_pos, u8 no_echo)
{
  u16 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 6);

  // play off events
  SEQ_CORE_PlayOffEvents();

  // and set new tick value
  SEQ_BPM_TickSet(new_tick);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_Tick(u32 bpm_tick, u8 no_echo)
{
  //  printf("BPM %d\n\r", bpm_tick);

  // increment reference step on each 16th note
  // set request flag on overrun (tracks can synch to measure)
  u8 synch_to_measure_req = 0;
  if( (bpm_tick % (384/4)) == 0 ) {
    if( seq_core_state.FIRST_CLK )
      seq_core_state.ref_step = 0;
    else {
      if( ++seq_core_state.ref_step >= steps_per_measure ) {
	seq_core_state.ref_step = 0;
	synch_to_measure_req = 1;
      }
    }
  }

  int track;
  seq_core_trk_t *t = &seq_core_trk[0];
  seq_cc_trk_t *tcc = &seq_cc_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++t, ++tcc, ++track) {

    // if "synch to measure" flag set: reset track if master has reached the selected number of steps
    // MEMO: we could also provide the option to synch to another track
    if( synch_to_measure_req && tcc->clkdiv.SYNCH_TO_MEASURE )
      SEQ_CORE_ResetTrkPos(t, tcc);

    // MEMO: due to compatibilty reasons, the clock-predivider is currently multiplied by 4 (previously we used 96ppqn)
    u16 prediv = bpm_tick % (4 * (tcc->clkdiv.TRIPLETS ? 4 : 6));
    if( t->state.FIRST_CLK || (!prediv && ++t->div_ctr > tcc->clkdiv.value) ) {
      t->div_ctr = 0;

      // increment step if not in arpeggiator mode or arp position == 0
      if( tcc->mode.playmode != SEQ_CORE_TRKMODE_Arpeggiator || !t->arp_pos ) {
	u8 skip_ctr = 0;
	do {
	  // determine next step depending on direction mode
	  if( !t->state.FIRST_CLK )
	    SEQ_CORE_NextStep(t, tcc, 0); // 0=not reverse

	  // clear "first clock" flag (on following clock ticks we can continue as usual)
	  t->state.FIRST_CLK = 0;

	  // if skip flag set for this flag: try again
	  if( SEQ_TRG_SkipGet(track, t->step) )
	    ++skip_ctr;
	  else
	    break;

	} while( skip_ctr < 32 ); // try 32 times maximum
      }


      // solo function: don't play MIDI event if track not selected
      if( seq_ui_button_state.SOLO && !SEQ_UI_IsSelectedTrack(track) )
	continue;

      // mute function
      if( t->state.MUTED )
	continue;

      // if random gate trigger set: play step with 1:1 probability
      if( SEQ_TRG_RandomGateGet(track, t->step) && (SEQ_RANDOM_Gen(0) & 1) )
	continue;

      // fetch MIDI events which should be played
      if( tcc->evnt_mode < SEQ_LAYER_EVNTMODE_NUM ) {
	s32 (*getevnt_func)(u8 track, u8 step, seq_layer_evnt_t layer_events[4]) = seq_layer_getevnt_func[tcc->evnt_mode];
	seq_layer_evnt_t layer_events[4];
	s32 number_of_events = getevnt_func(track, t->step, layer_events);
	if( number_of_events > 0 ) {
	  int i;
	  seq_layer_evnt_t *e = &layer_events[0];
	  for(i=0; i<number_of_events; ++e, ++i) {
	    mios32_midi_package_t *p = &e->midi_package;

	    // transpose notes/CCs
	    SEQ_CORE_Transpose(t, tcc, p);

	    // skip if velocity has been cleared by transpose function
	    // (e.g. no key pressed in transpose mode)
	    if( p->type == NoteOn && !p->velocity )
	      continue;

	    // sustained note: play off event
	    if( tcc->mode.SUSTAIN && t->sustain_note.ALL ) {
	      SEQ_MIDI_Send(t->sustain_port, t->sustain_note, SEQ_MIDI_OffEvent, bpm_tick);
	      t->sustain_note.ALL = 0;
	    }

	    // roll/glide flag
	    if( e->len >= 0 ) {
	      if( SEQ_TRG_RollGet(track, t->step) )
		e->len = 0x2c; // 2x12
	      if( SEQ_TRG_GlideGet(track, t->step) )
		e->len = 0x1f; // Glide
	    }

	    // send On event
	    if( p->type == CC ) {
	      SEQ_MIDI_Send(tcc->midi_port, *p, SEQ_MIDI_CCEvent, bpm_tick);
	      t->vu_meter = 0x7f; // for visualisation in mute menu
	    } else {
	      if( p->note && p->velocity ) {
		// force velocity to 0x7f if accent flag set
		if( SEQ_TRG_AccentGet(track, t->step) )
		  p->velocity = 0x7f;

		SEQ_MIDI_Send(tcc->midi_port, *p, SEQ_MIDI_OnEvent, bpm_tick);
		t->vu_meter = p->velocity; // for visualisation in mute menu
	      } else
		p->velocity = 0; // force velocity to 0 for next check
	    }

	    // send Off event
	    if( p->velocity && (e->len >= 0) ) { // if off event should be played (note: on CC events, velocity matches with CC value)
	      if( e->len < 32 ) {
		p->velocity = 0x00; // clear velocity
		if( p->type == NoteOn && tcc->mode.SUSTAIN ) {
		  t->sustain_port = tcc->midi_port;
		  t->sustain_note = *p;
		} else {
		  u32 delay = 4*(e->len+1); // TODO: later we will support higher note length resolution
		  SEQ_MIDI_Send(tcc->midi_port, *p, SEQ_MIDI_OffEvent, bpm_tick + delay);
		}
	      } else {
		// multi trigger - thanks to MIDI queueing mechanism, realisation is much much easier than on MBSEQ V3!!! :-)
		int i;
		int triggers = (e->len>>5);
		u32 delay = 4 * (e->len & 0x1f);
		// TODO: here we could add a special FX, e.g. lowering velocity on each trigger
		for(i=0; i<triggers; ++i)
		  SEQ_MIDI_Send(tcc->midi_port, *p, SEQ_MIDI_OffEvent, bpm_tick + (i+1)*delay);

		p->velocity = 0x00; // clear velocity
		for(i=0; i<=triggers; ++i)
		  SEQ_MIDI_Send(tcc->midi_port, *p, SEQ_MIDI_OffEvent, bpm_tick + i*delay + (delay/2));
	      }
	    }
	  }
	}
      }
    }
  }

  // clear "first clock" flag if it was set before
  seq_core_state.FIRST_CLK = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets the step position variables of a track
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_ResetTrkPos(seq_core_trk_t *t, seq_cc_trk_t *tcc)
{
  // don't increment on first clock event
  t->state.FIRST_CLK = 1;

  // reset step REPLAY and FWD counters
  t->step_replay_ctr = 0;
  t->step_fwd_ctr = 0;

  // reset clock divider
  t->div_ctr = 0;

  // reset record state and other record specific variables
  if( 0 ) { // TODO: define record mode
    t->state.REC_EVNT_ACTIVE = 0;
    t->state.REC_MUTE_NEXTSTEP = 0;
    // record_step = 0;
    // record_current_event1 = 0;
    // record_length_ctr = 0;
  }

  // next part depends on forward/backward direction
  if( tcc->dir_mode == SEQ_CORE_TRKDIR_Backward ) {
    // only for Backward mode
    t->state.BACKWARD = 1;
    t->step = tcc->length;
  } else {
    // for Forward/PingPong/Pendulum/Random/...
    t->state.BACKWARD = 0;
    t->step = 0;
  }

  // save position (for repeat function)
  t->step_saved = t->step;

  t->arp_pos = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Determine next step depending on direction mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, s32 reverse)
{
  int i;
  u8 save_step = 0;
  u8 new_step = 1;

  // handle progression parameters if position shouldn't be reset
  if( !reverse && !t->state.POS_RESET ) {
    if( ++t->step_fwd_ctr > tcc->steps_forward ) {
      t->step_fwd_ctr = 0;
      if( tcc->steps_jump_back ) {
	for(i=0; i<tcc->steps_jump_back; ++i) {
	  SEQ_CORE_NextStep(t, tcc, 1); // 1=reverse
	}
      }
      if( ++t->step_replay_ctr > tcc->steps_replay ) {
	t->step_replay_ctr = 0;
	save_step = 1; // request to save the step in t->step_saved at the end of this routine
      } else {
	t->step = t->step_saved;
	new_step = 0; // don't calculate new step
      }
    }
  }

  if( new_step ) {
    // special cases:
    switch( tcc->dir_mode ) {
      case SEQ_CORE_TRKDIR_Forward:
	t->state.BACKWARD = 0; // force backward flag
	break;

      case SEQ_CORE_TRKDIR_Backward:
	t->state.BACKWARD = 1; // force backward flag
	break;

      case SEQ_CORE_TRKDIR_PingPong:
      case SEQ_CORE_TRKDIR_Pendulum:
	// nothing else to do...
	break;

      case SEQ_CORE_TRKDIR_Random_Dir:
	// set forward/backward direction with 1:1 probability
	t->state.BACKWARD = SEQ_RANDOM_Gen(0) & 1;
        break;

      case SEQ_CORE_TRKDIR_Random_Step:
	t->step = SEQ_RANDOM_Gen_Range(tcc->loop, tcc->length);
	new_step = 0; // no new step calculation required anymore
        break;

      case SEQ_CORE_TRKDIR_Random_D_S:
	{
	  // we continue with a probability of 50%
	  // we change the direction with a probability of 25%
	  // we jump to a new step with a probability of 25%
	  u32 rnd;
	  if( ((rnd=SEQ_RANDOM_Gen(0)) & 0xff) < 0x80 ) {
	    if( rnd < 0x40 ) {
	      // set forward/backward direction with 1:1 probability
	      t->state.BACKWARD = SEQ_RANDOM_Gen(0) & 1;
	    } else {
	      t->step = SEQ_RANDOM_Gen_Range(tcc->loop, tcc->length);
	      new_step = 0; // no new step calculation required anymore
	    }
	  }
	}
	break;
    }
  }

  if( new_step ) { // note: new_step will be cleared in SEQ_CORE_TRKDIR_Random_Step mode
    // branch depending on forward/backward mode, take reverse flag into account
    if( t->state.BACKWARD ^ reverse ) {
      // jump to last step if first loop step has been reached or a position reset has been requested
      // in pendulum mode: switch to forward direction
      if( t->state.POS_RESET || t->step <= tcc->loop ) {
	if( tcc->dir_mode == SEQ_CORE_TRKDIR_Pendulum ) {
	  t->state.BACKWARD = 0;
	} else {
	  t->step = tcc->length;
	}
	// reset arp position as well
	t->arp_pos = 0;
      } else {
	// no reset required; decrement step
	--t->step;

	// in pingpong mode: turn direction if loop step has been reached after this decrement
	if( t->step <= tcc->loop && tcc->dir_mode == SEQ_CORE_TRKDIR_PingPong )
	  t->state.BACKWARD = 0;
      }
    } else {
      // jump to first (loop) step if last step has been reached or a position reset has been requested
      // in pendulum mode: switch to backward direction
      if( t->state.POS_RESET || t->step >= tcc->length ) {
	if( tcc->dir_mode == SEQ_CORE_TRKDIR_Pendulum ) {
	  t->state.BACKWARD = 1;
	} else {
	  t->step = tcc->loop;
	}
	// reset arp position as well
	t->arp_pos = 0;
      } else {
	// no reset required; increment step
	++t->step;

	// in pingpong mode: turn direction if last step has been reached after this increment
	if( t->step >= tcc->length && tcc->dir_mode == SEQ_CORE_TRKDIR_PingPong )
	  t->state.BACKWARD = 1;
      }
    }
  }

  if( !reverse ) {
    // requested by progression handler
    if( save_step )
      t->step_saved = t->step;

    t->state.POS_RESET = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Transposes if midi_package contains a Note Event
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_Transpose(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *midi_package)
{
  int note = midi_package->note;

  int inc_oct = tcc->transpose_oct;
  if( inc_oct >= 8 )
    inc_oct -= 16;

  int inc_semi = tcc->transpose_semi;
  if( inc_semi >= 8 )
    inc_semi -= 16;

  // in transpose or arp playmode we allow to transpose notes and CCs
  if( tcc->mode.playmode == SEQ_CORE_TRKMODE_Transpose ) {
    int tr_note = SEQ_MIDI_IN_TransposerNoteGet(tcc->mode.HOLD);

    if( tr_note < 0 ) {
      midi_package->velocity = 0; // disable note and exit
      return -1; // note has been disabled
    }

    inc_semi += tr_note - 0x3c; // C-3 is the base note
  }
  else if( tcc->mode.playmode == SEQ_CORE_TRKMODE_Arpeggiator ) {
    int key_num = (note >> 2) & 0x3;
    int arp_oct = (note >> 4) & 0x7;

    if( arp_oct < 2 ) { // Multi Arp Event
      inc_oct += ((note >> 2) & 7) - 4;
      key_num = t->arp_pos;
    } else {
      inc_oct += arp_oct - 4;
    }

    int arp_note = SEQ_MIDI_IN_ArpNoteGet(tcc->mode.HOLD, !tcc->mode.UNSORTED, key_num);

    if( arp_note & 0x80 ) {
      t->arp_pos = 0;
    } else {
      if( arp_oct < 2 ) { // Multi Arp Event
	// play next key, step will be incremented once t->arp_pos reaches 0 again
	if( ++t->arp_pos >= 4 )
	  t->arp_pos = 0;
      }
    }

    note = arp_note & 0x7f;

    if( !note ) { // disable note and exit
      midi_package->velocity = 0;
      return -1; // note has been disabled
    }
  }

#if 0
  // apply transpose octave/semitones parameter
  if( midi_package->type != NoteOn && midi_package->type != NoteOff )
    return -1; // no note event
  // TODO: what about tracks which play transposed/arp events + CC?
#endif

  if( inc_oct ) {
    note += 12 * inc_oct;
    if( inc_oct < 0 ) {
      while( note < 0 )
	note += 12;
    } else {
      while( note >= 128 )
	note -= 12;
    }
  }

  if( inc_semi ) {
    note += inc_semi;
    if( inc_semi < 0 ) {
      while( note < 0 )
	note += 12;
    } else {
      while( note >= 128 )
	note -= 12;
    }
  }

  midi_package->note = note;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function requests to pre-generate sequencer ticks for a given time
// It returns -1 if the previously requested delay hasn't passed yet
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_AddForwardDelay(u16 delay_ms)
{
  if( bpm_tick_forward_delay_ctr )
    return -1; // ongoing request

  // calculate how many BPM ticks are required to bridge the delay
  u32 add_bpm_ticks = SEQ_BPM_TicksFor_mS(delay_ms);

#if 0
  // for checking the result on the console
  printf("[SEQ_CORE_AddForwardDelay] %d ticks\n\r", add_bpm_ticks);
#endif

  // requested ticks will be taken from SEQ_CORE_Handler()
  bpm_tick_forward_delay_ctr_req = add_bpm_ticks;

  return 0; // no error
}
