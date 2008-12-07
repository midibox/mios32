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
#include "seq_midi.h"
#include "seq_bpm.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_CORE_ResetTrkPos(seq_core_trk_t *t, seq_cc_trk_t *tcc);
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, s32 reverse);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_core_state_t seq_core_state;
seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Init(u32 mode)
{
  seq_core_state.ALL = 0;

  // reset track parameters
  SEQ_CC_Init(0);

  // reset sequencer
  SEQ_CORE_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

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

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      if( seq_core_state.RUN && !seq_core_state.PAUSE )
	SEQ_CORE_Tick(bpm_tick);
    }
  } while( again && num_loops < 10 );
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

  // init bpm tick if in master mode (in slave mode it has already been done while receiving MIDI clock start)
  if( SEQ_BPM_IsMaster() )
    SEQ_BPM_TickSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Starts the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Start(u32 no_echo)
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
s32 SEQ_CORE_Stop(u32 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE = 0;
  seq_core_state.RUN = 0;
  SEQ_MIDI_FlushQueue();
  return no_echo ? 0 : SEQ_BPM_Stop();
}


/////////////////////////////////////////////////////////////////////////////
// Continues the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Cont(u32 no_echo)
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
s32 SEQ_CORE_Pause(u32 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state.PAUSE ^= 1;
  if( seq_core_state.PAUSE )
    SEQ_MIDI_FlushQueue();
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Tick(u32 bpm_tick)
{
  // increment reference step on each 16th note
  if( (bpm_tick % (SEQ_BPM_RESOLUTION_PPQN/4)) == 0 ) {
    if( seq_core_state.FIRST_CLK )
      seq_core_state.ref_step = 0;
    else {
      if( ++seq_core_state.ref_step >= 16 ) // TODO: adjustable steps per measure
	seq_core_state.ref_step = 0;
    }
  }

  int track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    seq_cc_trk_t *tcc = &seq_cc_trk[track];

    // TODO: use configurable clock dividers
    if( t->state.FIRST_CLK || ++t->div_ctr >= (SEQ_BPM_RESOLUTION_PPQN/4) ) {
      t->div_ctr = 0;

      // determine next step depending on direction mode
      if( !t->state.FIRST_CLK )
	SEQ_CORE_NextStep(t, tcc, 0); // 0=not reverse

      // clear "first clock" flag (on following clock ticks we can continue as usual)
      t->state.FIRST_CLK = 0;

      // generate event (temp. solution - no event mode supported yet!)
      u8 step_mask = 1 << (t->step % 8);
      u8 step_ix = t->step / 8;
      if( trg_layer_value[track][0][step_ix] & step_mask ) {
	mios32_midi_package_t midi_package;

	midi_package.type = 0x9;
	midi_package.evnt0 = 0x90 | tcc->midi_chn;
	midi_package.evnt1 = par_layer_value[track][0][t->step];
	midi_package.evnt2 = par_layer_value[track][1][t->step];

	SEQ_MIDI_Send(tcc->midi_port, midi_package, SEQ_MIDI_OnEvent, bpm_tick);
	midi_package.evnt2 = 0x00;
	u32 delay = 4*par_layer_value[track][2][t->step]; // TODO: later we will support higher note length resolution
	SEQ_MIDI_Send(tcc->midi_port, midi_package, SEQ_MIDI_OffEvent, bpm_tick+delay);
      }
    }
  }

  // clear "first clock" flag if it was set before
  seq_core_state.FIRST_CLK = 0;
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

