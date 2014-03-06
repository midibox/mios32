// $Id$
//! \defgroup MBNG_SEQ
//! Sequencer functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <seq_bpm.h>
#include <midi_router.h>
#include <midi_port.h>

#include "tasks.h"
#include "app.h"
#include "mbng_seq.h"

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MBNG_SEQ_SongPos(u16 new_song_pos);
static s32 MBNG_SEQ_Tick(u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

// pause mode
static u8 seq_pause;


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // init BPM generator
  SEQ_BPM_Init(0);
  SEQ_BPM_Set(120.0);

  // reset sequencer
  MBNG_SEQ_Reset();

  // disable pause after power-on
  MBNG_SEQ_SetPauseMode(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! this sequencer handler is called periodically to check for new requests
//! from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_Handler(void)
{
  // handle BPM requests
  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    // note: don't remove any request check - clocks won't be propagated
    // as long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release pause mode
      MBNG_SEQ_SetPauseMode(0);

      MIDI_ROUTER_SendMIDIClockEvent(0xfb, 0);
    }

    if( SEQ_BPM_ChkReqStart() ) {
      MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);
      MBNG_SEQ_Reset();
      MBNG_SEQ_SongPos(0);
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      MBNG_SEQ_SongPos(new_song_pos);
    }

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {

      again = 1; // check all requests again after execution of this part
      MBNG_SEQ_Tick(bpm_tick);
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_SEQ_Tick(u32 bpm_tick)
{
  // send MIDI clock depending on ppqn
  if( (bpm_tick % (SEQ_BPM_PPQN_Get()/24)) == 0 ) {
    MIDI_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_Reset(void)
{
  // release pause and FFWD mode
  MBNG_SEQ_SetPauseMode(0);

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sets new song position (new_song_pos resolution: 16th notes)
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_SEQ_SongPos(u16 new_song_pos)
{
  u32 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);

  // set new tick value
  SEQ_BPM_TickSet(new_tick);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! enables/disables pause
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_SetPauseMode(u8 enable)
{
  seq_pause = enable;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \return 1 if pause enabled
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_PauseEnabled(void)
{
  return seq_pause;
}


/////////////////////////////////////////////////////////////////////////////
//! To control the play button function
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_PlayButton(void)
{
  // if in auto mode and BPM generator is not clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

  if( MBNG_SEQ_PauseEnabled() ) {
    // continue sequencer
    MBNG_SEQ_SetPauseMode(0);
    SEQ_BPM_Cont();
  } else {
    // reset sequencer
    MBNG_SEQ_Reset();

    // and start
    SEQ_BPM_Start();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! To control the stop button function
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_StopButton(void)
{
  if( SEQ_BPM_IsRunning() ) {
    SEQ_BPM_Stop();          // stop sequencer
  } else {
    // reset sequencer
    MBNG_SEQ_Reset();

    // disable pause mode
    MBNG_SEQ_SetPauseMode(0);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! To control the stop button function
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_PauseButton(void)
{
  // if in auto mode and BPM generator is not clocked in slave mode:
  // change to master mode
  SEQ_BPM_CheckAutoMaster();

  // toggle pause
  seq_pause ^= 1;

  // execute stop/continue depending on new mode
  MIOS32_IRQ_Disable();
  if( seq_pause ) {
    SEQ_BPM_Stop();
  } else {
    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Cont();
  }
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! To control the play/stop button function
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SEQ_PlayStopButton(void)
{
  if( SEQ_BPM_IsRunning() ) {
    MBNG_SEQ_StopButton();
  } else {
    MBNG_SEQ_PlayButton();
  }

  return 0; // no error
}


//! \}
