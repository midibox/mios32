// $Id$
//! \defgroup SEQ_BPM
//!
//! Some comments to the way how the bpm_tick is generated:
//!
//! <B>MASTER MODE</B>
//!
//! A timer interrupt will be triggered with an interval of:
//!   interval = (60 / (bpm * 24)) / (ppqn/24)
//!
//! It increments bpm_tick, and request the clock event to the sequencer.
//!
//! The sequencer can check for clock events by calling 
//! \code
//!    u32 bpm_tick_ptr; // will contain the BPM tick which has to be processed
//!    while( SEQ_BPM_ChkReqClk(&bpm_tick_ctr) ) {
//!      // got a new tick
//!    }
//! \endcode
//!
//! This polling scheme ensures, that no bpm_tick gets lost if the sequencer
//! task is stalled for any reason
//!
//! Since the *_ChkReq* function is a destructive call, it should only be used
//! by the sequencer task, and not called from any other function.
//!
//! Similar request flags exist for MIDI Clock Start/Stop/Continue and Song Position
//! \note these flags <B>have to be</B> polled as well, otherwise clock requests
//! will be held back
//!
//!
//! <B>SLAVE MODE</B>
//!
//! In order to reach a higher ppqn resolution (e.g. 384 ppqn) than provided 
//! by MIDI (24 ppqn), the incoming clock has to be interpolated in ppqn/24 steps
//!
//! A timer interrupt with an interval of 250 uS is used to measure 
//! the delay between two incoming MIDI clock events. The same interrupt
//! will call the sequencer clock handler with an interval of:
//!   interval = 250 uS * measured_delay / (ppqn/24)
//! (e.g. 384 ppqn: 16x interpolated clock)
//!
//! There is a protection (-> sent_clk_ctr) which ensures, that never more
//! that 16 internal ticks will be triggered between two F8 events to 
//! improve the robustness on BPM sweeps or jittering incoming MIDI clock
//!
//! Note that instead of the timer interrupt, we could also use a HW timer
//! to measure the delay. Problem: currently I don't know how to handle this
//! in the MacOS based emulation...
//!
//! \{
/* ==========================================================================
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

#include "seq_bpm.h"


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_BPM_TimerInit(void);
static void SEQ_BPM_Timer_Slave(void);
static void SEQ_BPM_Timer_Master(void);
static s32 SEQ_BPM_DigitUpdate(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// we are using bytes instead of unions for some flags to avoid the 
// requirement for read-modify-store (atomic) operations

static seq_bpm_mode_t bpm_mode;

static u8 bpm_req_start;
static u8 bpm_req_stop;
static u8 bpm_req_cont;
static u8 bpm_req_clk_ctr;
static u8 bpm_req_song_pos;

static u8 slave_clk;

static u32 bpm_tick;
static u8  running; // 0: not running, 1: received start event, 2: received first clock after start

static float bpm;
static u16 ppqn;

static u32 incoming_clk_ctr;
static u32 incoming_clk_delay;
static u32 sent_clk_ctr;
static u32 sent_clk_delay;

static u16 new_song_pos;
static u8  receive_song_pos_state;


/////////////////////////////////////////////////////////////////////////////
//! Initialisation of BPM generator
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Init(u32 mode)
{
  bpm_req_start = 0;
  bpm_req_stop = 0;
  bpm_req_cont = 0;
  bpm_req_clk_ctr = 0;
  bpm_req_song_pos = 0;

  bpm_tick = 0;
  running = 0;
  new_song_pos = 0;

  receive_song_pos_state = 0;

  slave_clk = 0;
  incoming_clk_ctr = 0;
  incoming_clk_delay = 0;
  sent_clk_ctr = 0;
  sent_clk_delay = 0;

  // start clock generator with 140 BPM/384 ppqn in Auto mode
  ppqn = 384;
  bpm = 140.0;
  SEQ_BPM_ModeSet(SEQ_BPM_MODE_Auto);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Queries the current run mode
//! \return the run mode
/////////////////////////////////////////////////////////////////////////////
seq_bpm_mode_t SEQ_BPM_ModeGet(void)
{
  return bpm_mode;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets a new run mode
//! \param[in] mode the run mode
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ModeSet(seq_bpm_mode_t mode)
{
  bpm_mode = mode;

  // disable clock slave mode if SEQ_BPM_MODE_Auto or SEQ_BPM_MODE_Master
  slave_clk = (mode == SEQ_BPM_MODE_Slave) ? 1 : 0;

  // initialize timer
  SEQ_BPM_TimerInit();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Queries the current BPM rate
//! \return bpm rate as float
/////////////////////////////////////////////////////////////////////////////
float SEQ_BPM_Get(void)
{
  return bpm;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets a new BPM rate
//! \param[in] bpm rate as float
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Set(float _bpm)
{
  // set new BPM rate
  bpm = _bpm;

  // set new BPM rate if not in slave mode
  if( !slave_clk ) {
    // calculate timer period for the given BPM rate
    float period = 1E6 * (60 / (bpm*24)) / (float)(ppqn/24);
    u16 period_u = (u16)period;
    // safety measure: ensure that period is nether less than 250 uS
    if( period_u < 250 )
      period_u = 250;
    MIOS32_TIMER_ReInit(SEQ_BPM_MIOS32_TIMER_NUM, period_u); // re-init timer interval
  }

  // update LED digits
  SEQ_BPM_DigitUpdate();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Queries the current PPQN value
//! \return ppqn value
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_PPQN_Get(void)
{
  return ppqn;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets a new PPQN value
//! \note after changing PPQN, SEQ_BPM_Set() has to be called again to take
//! over the new value.
//! In other words: set PPQN before BPM
//!
//! Supported ppqn for slave mode: 24, 48, 96, 192 and 384
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_PPQN_Set(u16 _ppqn)
{
  // set new ppqn value
  ppqn = _ppqn;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Queries the current tick value
//! \return BPM tick value
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_BPM_TickGet(void)
{
  return bpm_tick;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets a new BPM tick value
//! \param[in] tick new BPM tick value
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_TickSet(u32 tick)
{
  bpm_tick = tick;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! BPM generator running?
//! \return 0 if generator not running
//! \return 1 if start event has been received
//! \return 2 if first clock after start has been received and bpm_tick will be incremented
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_IsRunning(void)
{
  return running;
}


/////////////////////////////////////////////////////////////////////////////
//! BPM clocked as master or slave?
//! \return 0: BPM clocked as slave, 1: BPM clocked as master
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_IsMaster(void)
{
  return slave_clk ? 0 : 1;
}


/////////////////////////////////////////////////////////////////////////////
//! BPM should be clocked as master or slave?
//! If this function is called in Auto Mode, BPM will switch to Master Clock
//! \return 1 if we changed from auto slave to master mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_CheckAutoMaster(void)
{
  // in Auto Slave Mode: check if we are already in master mode - if not, change to it now!
  if( slave_clk && bpm_mode == SEQ_BPM_MODE_Auto ) {
    MIOS32_IRQ_Disable();
    slave_clk = 0;
    SEQ_BPM_TimerInit();
    MIOS32_IRQ_Enable();

    return 1;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Timer interrupt
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BPM_TimerInit(void)
{
  if( slave_clk ) {
    // in slave mode, the timer will be used to measure the delay between
    // two clocks to generate 16 internal clocks (@384ppqn) on every F8 event.
    // using 250 uS as reference
    // using highest priority for best accuracy (routine is very short, so that this doesn't hurt)
    MIOS32_TIMER_Init(SEQ_BPM_MIOS32_TIMER_NUM, 250, SEQ_BPM_Timer_Slave, MIOS32_IRQ_PRIO_HIGHEST);
  } else {
    // initial timer configuration for master mode -- calls the core clk routine directly
    MIOS32_TIMER_Init(SEQ_BPM_MIOS32_TIMER_NUM, 1000, SEQ_BPM_Timer_Master, MIOS32_IRQ_PRIO_HIGHEST);
    // set the correct BPM rate
    SEQ_BPM_Set(bpm);
  }

  return 0; // no error
}


static void SEQ_BPM_Timer_Master(void)
{
  if( running >= 2 ) {
    ++bpm_tick;
    ++bpm_req_clk_ctr;
  }
}


static void SEQ_BPM_Timer_Slave(void)
{
  // disable interrupts to avoid conflicts with NotifyRx handler (which could be called from 
  // a high-prio interrupt)
  MIOS32_IRQ_Disable();

  // increment clock counter, used to measure the delay between two F8 events
  ++incoming_clk_ctr;

  // decrement sent clock delay, send interpolated clock events ((ppqn/24)-1) times
  if( sent_clk_delay && --sent_clk_delay == 0 ) {
    if( sent_clk_ctr < (ppqn/24) ) {
      sent_clk_delay = incoming_clk_delay/(ppqn/24);
      ++sent_clk_ctr;

      if( running >= 2 ) {
	++bpm_tick;
	++bpm_req_clk_ctr;
      }
    }
  }
  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Update the LED digits
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BPM_DigitUpdate(void)
{
  // branch depending on current (selected) mode
  if( bpm_mode == SEQ_BPM_MODE_Slave ) {
    // TODO
  } else { // SEQ_BPM_MODE_Master or SEQ_BPM_MODE_Auto
    // TODO
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! should be called from Direct Rx notification hook in app.c on incoming MIDI bytes
//! see also example code
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_NotifyMIDIRx(u8 midi_byte)
{
  // any MIDI clock/start/cont/stop event received?
  if( midi_byte == 0xf8 || (midi_byte >= 0xfa && midi_byte <= 0xfc) ) {
    // if in Auto mode: switch from master to slave mode if required
    if( !slave_clk && bpm_mode == SEQ_BPM_MODE_Auto ) {
      // switch to slave mode and re-init timer IRQ
      slave_clk = 1;
      SEQ_BPM_TimerInit();
    }

    // exit if not in slave mode
    if( !slave_clk )
      return 0; // no error

    // following operations should be atomic
    MIOS32_IRQ_Disable();

    if( midi_byte == 0xf8 ) { // MIDI clock
      // we've measured a new delay between two F8 events
      incoming_clk_delay = incoming_clk_ctr;
      incoming_clk_ctr = 0;

      // how many clocks (still) need to be triggered?
      if( running >= 2 )
	bpm_req_clk_ctr += (ppqn/24) - sent_clk_ctr;

      // clear interpolation clock counter and get new SENT_CLK delay
      sent_clk_ctr = 0;
      sent_clk_delay = incoming_clk_delay / (ppqn/24);

      // if first clock after start or continue event: set run state to 2
      // (now we start to send BPM ticks)
      if( running == 1 )
	running = 2;

    } else if( midi_byte == 0xfa ) { // MIDI Start event
      // request sequencer start, disable stop request
      bpm_req_start = 1;
      bpm_req_stop = 0;

      // cancel all requested clocks
      bpm_req_clk_ctr = 0;
      sent_clk_ctr = (ppqn/24);

      // reset BPM tick value
      bpm_tick = 0;

      // enter run state 1 (bpm_tick will be incremented once the first F8 event has been received)
      running = 1;
    } else if( midi_byte == 0xfb ) { // MIDI Continue event
      // request continue, disable stop request
      bpm_req_cont = 1;
      bpm_req_stop = 0;

      // enter run state 1 (bpm_tick will be incremented once the first F8 event has been received)
      running = 1;
    } else { // MIDI Stop event
      bpm_req_stop = 1;

      // enter stop state (bpm_tick won't be incremented)
      running = 0;
    }

    // enable interrupts again
    MIOS32_IRQ_Enable();

  } else if( receive_song_pos_state || midi_byte == 0xf2 ) {

    // new song position is received
    switch( receive_song_pos_state ) {
      case 0:
	// expecting LSB and MSB
	receive_song_pos_state = 1;
	break;

      case 1:
	// ignore any realtime event (e.g. 0xfe)
	if( midi_byte < 0xf8 ) {
	  // an unexpected status byte has been received
	  if( midi_byte >= 0x80 )
	    receive_song_pos_state = 0;
	  else {
	    // get LSB, expect MSB
	    receive_song_pos_state = 2;
	    new_song_pos = midi_byte;
	  }
	}
	break;

      case 2:
	// ignore any realtime event (e.g. 0xfe)
	if( midi_byte < 0xf8 ) {
	  // an unexpected status byte has been received
	  if( midi_byte >= 0x80 )
	    receive_song_pos_state = 0;
	  else {
	    // get MSB and send request to update sequencer
	    receive_song_pos_state = 0;
	    new_song_pos |= (midi_byte << 7);
	    bpm_req_song_pos = 1;

	    // take over new bpm_tick value immediately (16th notes resolution)
	    bpm_tick = new_song_pos * (ppqn/4);
	  }
	}
	break;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called when a tempo is "tapped" with a button
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_TapTempo(void)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function starts the BPM generator and forwards this event to the
//! sequencer. 
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Start(void)
{
  MIOS32_IRQ_Disable();
  bpm_req_start = 1;
  if( !slave_clk || running == 1 ) { // master mode: start to send events, slave mode: change from 1->2 state
    running = 2;
  }
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function continues the BPM generator and forwards this event to the
//! sequencer. 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Cont(void)
{
  MIOS32_IRQ_Disable();
  bpm_req_cont = 1;
  if( !slave_clk || running == 1 ) { // master mode: start to send events, slave mode: change from 1->2 state
    running = 2;
  }
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function stops the BPM generator and forwards this event to the
//! sequencer
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Stop(void)
{
  MIOS32_IRQ_Disable();
  bpm_req_stop = 1;
  running = 0;
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Has to be called by the sequencer to check for a stop request
//! \return 0 if no stop request
//! \return 1 if stop has been requested
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqStop(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_stop;
  bpm_req_stop = 0;
  MIOS32_IRQ_Enable();
  return req;
}

/////////////////////////////////////////////////////////////////////////////
//! Has to be called by the sequencer to check for a start request
//! \return 0 if no start request
//! \return 1 if start has been requested
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqStart(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_start;
  bpm_req_start = 0;
  MIOS32_IRQ_Enable();
  return req;
}

/////////////////////////////////////////////////////////////////////////////
//! Has to be called by the sequencer to check for a continue request
//! \return 0 if no continue request
//! \return 1 if continue has been requested
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqCont(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_cont;
  bpm_req_cont = 0;
  MIOS32_IRQ_Enable();
  return req;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the bpm_tick which is related to the clock request
//! \attention ChkReqClk will return 0 so long Stop/Cont/Start/SongPos haven't been
//! checked to ensure that clocks won't be propagated so long a position or
//! run state change hasn't been flagged to the sequencer. Accordingly, the
//! sequencer application has to poll all requests as shown in the examples,
//! otherwise it will never receive a clock
//! \param[out] bpm_tick_ptr a pointer to a u32 value which will get the 
//!             requested bpm_tick
//! \return 0 if no clock request (content of bpm_tick invalid)
//! \return 1 if clock has been requested (content of bpm_tick valid)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqClk(u32 *bpm_tick_ptr)
{
  u8 req;

  // don't trigger clock if there is an ongoing request which could change the song position
  if( bpm_req_stop || bpm_req_start || bpm_req_cont || bpm_req_song_pos ) {
    req = 0;
  } else {
    MIOS32_IRQ_Disable();
    if( bpm_req_clk_ctr > bpm_tick ) {
      // ensure that bpm_tick never gets negative (e.g. if clock wasn't polled for long time)
      bpm_req_clk_ctr = bpm_tick;
    }
    if( (req=bpm_req_clk_ctr) ) {
      *bpm_tick_ptr = bpm_tick - bpm_req_clk_ctr;
      --bpm_req_clk_ctr;
    }
    MIOS32_IRQ_Enable();
  }
  return req;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns a new song position if requested from external
//! \param[out] bpm_tick_ptr a pointer to a u32 value which will get the 
//!             requested bpm_tick
//! \return 0 if no song position request (content of song_pos invalid)
//! \return 1 if new song position has been requested (content of song_pos valid)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqSongPos(u16 *song_pos)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_song_pos;
  bpm_req_song_pos = 0;
  *song_pos = new_song_pos;
  MIOS32_IRQ_Enable();
  return req;
}


/////////////////////////////////////////////////////////////////////////////
//! This help function returns the number of BPM ticks for a given time
//! E.g.: SEQ_BPM_TicksFor_mS(50) returns 38 ticks @ 120 BPM 384 ppqn
//! Regardless if BPM generator is clocked in master or slave mode
//! \param[in] time_ms the time in milliseconds (e.g. 50 corresponds to 50 mS)
//! \return the number of BPM ticks
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_BPM_TicksFor_mS(u16 time_ms)
{
  if( slave_clk ) {
    float time_per_tick = 0.25 * incoming_clk_delay / (ppqn/24);
    return (u32)((float)time_ms / time_per_tick);
  }

  float period_m = 1E3 * (60 / (bpm*24)) / (ppqn/24);
  return (u32)((float)time_ms / period_m);
}

//! \}
