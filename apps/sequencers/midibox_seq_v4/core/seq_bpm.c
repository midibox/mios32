// $Id$
/*
 * BPM generator
 *
 * Some notes to the way how MIDIboxSEQ generates the internal 384ppqn clock
 *
 * MASTER MODE
 * ~~~~~~~~~~~
 *
 * In order to reach 384ppqn resolution, a timer interrupt will be triggered
 * with an interval of:
 *   interval = (60 / (bpm * 24)) / 16
 *
 * It requests clock events to the sequencer on each invocation.
 *
 * The sequencer can check for clock events by calling 
 *    u32 bpm_tick_ptr; // will contain the BPM tick which has to be processed
 *    while( SEQ_BPM_ChkReqClk(&bpm_tick_ctr) ) {
 *      // got a new 384ppqn tick
 *    }
 *
 * Since the *_ChkReq* function is a destructive call, it should only be used
 * by the sequencer task.
 *
 * Similar request flags exist for MIDI Clock Start/Stop/Continue
 *
 *
 * SLAVE MODE
 * ~~~~~~~~~~
 *
 * A timer interrupt with an interval of 250 uS is used to measure 
 * the delay between two incoming MIDI clock events. The same interrupt
 * will call the sequencer clock handler with an interval of:
 *   interval = 250 uS * measured_delay / 16
 * (16x interpolated clock)
 *
 * There is a protection (-> sent_clk_ctr) which ensures, that never more
 * that 16 internal clock events will be triggered between two F8 to
 * improve robustness on BPM sweeps or jittering incoming MIDI clock
 *
 * Note that instead of the timer interrupt, we could also use a HW timer
 * to measure the delay. Problem: currently I don't know how to handle this
 * in the MacOS based emulation...
 *
 * 
 * The API of the BPM generator has been written in a way which allows
 * general usage by multiple applications
 * It could be provided under $MIOS32_PATH/modules/bpm once the robustness
 * has been proven
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

#include "seq_bpm.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_BPM_TimerInit(void);
static void SEQ_BPM_Timer_Slave(void);
static void SEQ_BPM_Timer_Master(void);
static s32 SEQ_BPM_DigitUpdate(void);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


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

static u8 slave_clk;

static u32 bpm_tick;

static u16 bpm;

static u32 incoming_clk_ctr;
static u32 incoming_clk_delay;
static u32 sent_clk_ctr;
static u32 sent_clk_delay;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Init(u32 mode)
{
  bpm_req_start = 0;
  bpm_req_stop = 0;
  bpm_req_cont = 0;
  bpm_req_clk_ctr = 0;

  bpm_tick = 0;

  slave_clk = 0;
  incoming_clk_ctr = 0;
  incoming_clk_delay = 0;
  sent_clk_ctr = 0;
  sent_clk_delay = 0;

  // start clock generator with 140 BPM in Auto mode
  bpm = 1400;
  SEQ_BPM_ModeSet(SEQ_BPM_MODE_Auto);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// sets/queries the current run mode
/////////////////////////////////////////////////////////////////////////////
seq_bpm_mode_t SEQ_BPM_ModeGet(void)
{
  return bpm_mode;
}

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
// set/query current BPM
// Note that BPM is multiplied by 10 to reach higher accuracy
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Get(void)
{
  return bpm;
}

s32 SEQ_BPM_Set(u16 _bpm)
{
  // set new BPM rate
  bpm = _bpm;

  // set new BPM rate if not in slave mode
  if( !slave_clk ) {
    // calculate timer period for the given BPM rate
    float period = 1E6 * (60 / (bpm*2.4)) / SEQ_BPM_RESOLUTION_FACTOR; // multiplied by 2.4 instead of 24 because of *10 BPM accuracy
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
// sets/queries the BPM tick (timestamp)
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_BPM_TickGet(void)
{
  return bpm_tick;
}

s32 SEQ_BPM_TickSet(u32 tick)
{
  MIOS32_IRQ_Disable();

  // especially in slave mode it can happen that additional clocks
  // have already been requested while the bpm_tick reference is
  // changed.
  // it's a race condition between BPM generator and sequencer which can
  // be easily solved by shifting bpm_tick to the current point of time, 
  // so that SEQ_BPM_ChkReqClk() will be polled multiple times
  // (see code there)
  bpm_tick = tick + bpm_req_clk_ctr;

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// BPM clocked as master or slave?
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_IsMaster(void)
{
  return slave_clk ? 0 : 1;
}


/////////////////////////////////////////////////////////////////////////////
// Timer interrupt
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_BPM_TimerInit(void)
{
  if( slave_clk ) {
    // in slave mode, the timer will be used to measure the delay between
    // two clocks to produce 16 internal clocks on every F8 event.
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
  // NOTE: also used by slave timer! See below
  ++bpm_tick;
  ++bpm_req_clk_ctr;
}


static void SEQ_BPM_Timer_Slave(void)
{
  // disable interrupts to avoid conflicts with NotifyRx handler (which could be called from 
  // a high-prio interrupt)
  MIOS32_IRQ_Disable();

  // increment clock counter, used to measure the delay between two F8 events
  ++incoming_clk_ctr;

  // decrement sent clock delay, send interpolated clock events (SEQ_BPM_RESOLUTION_FACTOR-1) times
  if( --sent_clk_delay == 0 ) {
    if( sent_clk_ctr < SEQ_BPM_RESOLUTION_FACTOR ) {
      sent_clk_delay = incoming_clk_delay/SEQ_BPM_RESOLUTION_FACTOR;
      ++bpm_tick;
      ++sent_clk_ctr;
      ++bpm_req_clk_ctr; // add at least one clock request
      // (additional requests could be added by F8 receiver on delay overruns)
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
// call from Direct Rx notification hook in app.c on incoming MIDI bytes
/////////////////////////////////////////////////////////////////////////////
extern s32 SEQ_BPM_NotifyMIDIRx(u8 midi_byte)
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
      bpm_req_clk_ctr += SEQ_BPM_RESOLUTION_FACTOR - sent_clk_ctr;

      // clear interpolation clock counter and get new SENT_CLK delay
      sent_clk_ctr = 0;
      sent_clk_delay = incoming_clk_delay / SEQ_BPM_RESOLUTION_FACTOR;

    } else if( midi_byte == 0xfa ) { // MIDI Start event
      // request sequencer start, disable stop request
      bpm_req_start = 1;
      bpm_req_stop = 0;

      // cancel all requested clocks
      bpm_req_clk_ctr = 0;
      sent_clk_ctr = SEQ_BPM_RESOLUTION_FACTOR - 1;
    } else if( midi_byte == 0xfb ) { // MIDI Continue event
      // request continue, disable stop request
      bpm_req_cont = 1;
      bpm_req_stop = 0;
    } else { // MIDI Stop event
      bpm_req_stop = 1;
    }

    // enable interrupts again
    MIOS32_IRQ_Enable();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a tempo is "tapped" with a button
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_TapTempo(void)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function starts the BPM generator and forwards this event to the
// sequencer. 
// If this function is called in Auto Mode, BPM will switch to Master Clock
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Start(void)
{
  MIOS32_IRQ_Disable();
  bpm_req_start = 1;

  // in Auto Slave Mode: check if we are already in master mode - if not, change to it now!
  if( slave_clk && bpm_mode == SEQ_BPM_MODE_Auto ) {
    slave_clk = 0;
    SEQ_BPM_TimerInit();
  }
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function stops the BPM generator and forwards this event to the
// sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_Stop(void)
{
  bpm_req_stop = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Has to be called by the sequencer to chec for requests
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BPM_ChkReqStop(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_stop;
  bpm_req_stop = 0;
  MIOS32_IRQ_Enable();
  return req;
}

s32 SEQ_BPM_ChkReqStart(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_start;
  bpm_req_start = 0;
  MIOS32_IRQ_Enable();
  return req;
}

s32 SEQ_BPM_ChkReqCont(void)
{
  MIOS32_IRQ_Disable();
  u8 req = bpm_req_cont;
  bpm_req_cont = 0;
  MIOS32_IRQ_Enable();
  return req;
}

// returns the bpm_tick which is related to the clock request
s32 SEQ_BPM_ChkReqClk(u32 *bpm_tick_ptr)
{
  MIOS32_IRQ_Disable();
  u8 req;
  if( (req=bpm_req_clk_ctr) ) {
    if( bpm_tick >= bpm_req_clk_ctr )
      *bpm_tick_ptr = bpm_tick - bpm_req_clk_ctr;
    --bpm_req_clk_ctr;
  }
  MIOS32_IRQ_Enable();
  return req;
}


/////////////////////////////////////////////////////////////////////////////
// This help function returns the number of BPM ticks for a given time
// E.g.: SEQ_BPM_TicksFor_mS(50) returns 38 ticks @ 120 BPM 384 ppqn
// Regardless if BPM generator is clocked in master or slave mode
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_BPM_TicksFor_mS(u16 time_ms)
{
  if( slave_clk ) {
    float time_per_tick = 0.25 * incoming_clk_delay / SEQ_BPM_RESOLUTION_FACTOR;
    return (u32)((float)time_ms / time_per_tick);
  }

  float period_m = 1E3 * (60 / (bpm*2.4)) / SEQ_BPM_RESOLUTION_FACTOR; // multiplied by 2.4 instead of 24 because of *10 BPM accuracy
  return (u32)((float)time_ms / period_m);
}

