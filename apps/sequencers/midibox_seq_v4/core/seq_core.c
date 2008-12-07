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

    t->state.ALL = 0;
    t->step = 0; // TODO: depends on direction
    t->div_ctr = 0;
    t->next_delay_ctr = 0;
    t->step_replay_ctr = 0;
    t->step_saved = 0;
    t->step_fwd_ctr = 0;
    t->arp_pos = 0;
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
    if( seq_core_state.FIRST_CLK || ++t->div_ctr >= (SEQ_BPM_RESOLUTION_PPQN/4) ) {
      t->div_ctr = 0;

      if( seq_core_state.FIRST_CLK )
	t->step = 0;
      else {
	if( ++t->step >= 16 ) // TODO: variable track length, configurable direction, progression parameters, ...
	  t->step = 0;
      }

      u8 played_step = seq_core_state.ref_step;
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


