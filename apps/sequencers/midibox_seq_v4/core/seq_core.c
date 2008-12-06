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

// we are using bytes instead of unions for these flags to avoid the 
// requirement for read-modify-store (atomic) operations
u8 seq_core_state_run;
u8 seq_core_state_pause;

u8 played_step;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Init(u32 mode)
{
  seq_core_state_run = 0;
  seq_core_state_pause = 0;

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

      if( seq_core_state_run && !seq_core_state_pause )
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

  seq_core_state_pause = 0;
  played_step = 0;
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
  seq_core_state_run = 1;
  return no_echo ? 0 : SEQ_BPM_Start();
}


/////////////////////////////////////////////////////////////////////////////
// Stops the sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Stop(u32 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state_pause = 0;
  seq_core_state_run = 0;
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

  seq_core_state_pause = 0;
  seq_core_state_run = 1;
  return no_echo ? 0 : SEQ_BPM_Start();
}



/////////////////////////////////////////////////////////////////////////////
// Pauses sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Pause(u32 no_echo)
{
  // request display update
  seq_ui_display_update_req = 1;

  seq_core_state_pause ^= 1;
  if( seq_core_state_pause )
    SEQ_MIDI_FlushQueue();
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Tick(u32 bpm_tick)
{
  if( (bpm_tick % (SEQ_BPM_RESOLUTION_PPQN/4)) == 0 ) {
    // TODO: "played_step" still only a temporary solution...
    int track;
    u8 step_mask = 1 << (played_step&7);
    u8 step_ix = played_step >> 3;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
      if( trg_layer_value[track][0][step_ix] & step_mask ) {
	mios32_midi_package_t midi_package;

	midi_package.type = 0x9;
	midi_package.evnt0 = 0x90 | track;
	midi_package.evnt1 = par_layer_value[track][0][played_step];
	midi_package.evnt2 = par_layer_value[track][1][played_step];

	SEQ_MIDI_Send(DEFAULT, midi_package, SEQ_MIDI_OnEvent, bpm_tick);
	midi_package.evnt2 = 0x00;
	u32 delay = 4*par_layer_value[track][2][played_step]; // TODO: later we will support higher note length resolution
	SEQ_MIDI_Send(DEFAULT, midi_package, SEQ_MIDI_OffEvent, bpm_tick+delay);
      }
    }

    if( ++played_step >= 16 )
      played_step = 0;
  }
}


