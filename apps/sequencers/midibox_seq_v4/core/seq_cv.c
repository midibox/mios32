// $Id$
/*
 * CV functions of MIDIbox SEQ
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
#include <aout.h>
#include <notestack.h>


#include "seq_cv.h"
#include "seq_hwcfg.h"
#include "seq_bpm.h"
#include "seq_core.h"



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 gate_inversion_mask;

static u8 sync_clk_pulsewidth;
static u16 sync_clk_divider;

// each channel has an own notestack
static notestack_t cv_notestack[SEQ_CV_NUM];
static notestack_item_t cv_notestack_items[SEQ_CV_NUM][SEQ_CV_NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_Init(u32 mode)
{
  int i;

  gate_inversion_mask = 0x00;

  // initialize J5 pins
  // they will be enabled after MBSEQ_HW.V4 has been read
  // as long as this hasn't been done, activate pull-downs
#ifndef MBSEQV4L
  // MBSEQV4L: allocated by BLM_CHEAPO driver
  for(i=0; i<6; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);
#endif

  // pin J5.A6 and J5.A7 used for UART2 (-> MIDI OUT3)

#if defined(MIOS32_FAMILY_STM32F10x)
  for(i=8; i<12; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);
#elif defined(MIOS32_FAMILY_LPC17xx)
  for(i=0; i<4; ++i)
    MIOS32_BOARD_J28_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif

  // initialize AOUT driver
  AOUT_Init(0);

  // initial pulsewidth and divider for DIN clock
  sync_clk_pulsewidth = 1;
  sync_clk_divider = 16; // 24 ppqn

  // reset all channels
  SEQ_CV_ResetAllChannels();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name AOUT interface
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_IfSet(aout_if_t if_type)
{
  if( if_type >= AOUT_NUM_IF )
    return -1; // invalid interface

  aout_config_t config;
  config = AOUT_ConfigGet();
  config.if_type = if_type;
  config.if_option = (config.if_type == AOUT_IF_74HC595) ? 0xffffffff : 0x00000000; // AOUT_LC: select 8/8 bit configuration
  config.num_channels = 8;
  //config.chn_inverted = 0; // configurable
  AOUT_ConfigSet(config);
  return AOUT_IF_Init(0);
}

aout_if_t SEQ_CV_IfGet(void)
{
  aout_config_t config;
  config = AOUT_ConfigGet();
  return config.if_type;
}


// will return 8 characters
const char* SEQ_CV_IfNameGet(aout_if_t if_type)
{
  return AOUT_IfNameGet(if_type);
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name CV Curve
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_CurveSet(u8 cv, u8 curve)
{
  if( cv >= SEQ_CV_NUM )
    return -1; // invalid cv channel selected

  if( curve >= SEQ_CV_NUM_CURVES )
    return -2; // invalid curve selected

  u32 mask = 1 << cv;
  aout_config_t config;
  config = AOUT_ConfigGet();

  if( curve & 1 )
    config.chn_hz_v |= mask;
  else
    config.chn_hz_v &= ~mask;

  if( curve & 2 )
    config.chn_inverted |= mask;
  else
    config.chn_inverted &= ~mask;

  return AOUT_ConfigSet(config);
}

u8 SEQ_CV_CurveGet(u8 cv)
{
  if( cv >= SEQ_CV_NUM )
    return 0; // invalid cv channel selected, return default curve

  u32 mask = 1 << cv;
  aout_config_t config;
  config = AOUT_ConfigGet();

  u8 curve = 0;
  if( config.chn_hz_v & mask )
    curve |= 1;
  if( config.chn_inverted & mask )
    curve |= 2;

  return curve;
}

// will return 6 characters
// located outside the function to avoid "core/seq_cv.c:168:3: warning: function returns address of local variable"
static const char curve_desc[3][7] = {
  "V/Oct ",
  "Hz/V  ",
  "Inv.  ",
};
const char* SEQ_CV_CurveNameGet(u8 cv)
{

  if( cv >= SEQ_CV_NUM )
    return "------";

  return curve_desc[SEQ_CV_CurveGet(cv)];
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name Calibration Mode
/////////////////////////////////////////////////////////////////////////////

s32 SEQ_CV_CaliModeSet(u8 cv, aout_cali_mode_t mode)
{
  if( cv >= SEQ_CV_NUM )
    return -1; // invalid cv channel selected

  if( mode >= SEQ_CV_NUM_CALI_MODES )
    return -2; // invalid mode selected

  return AOUT_CaliModeSet(cv, mode);

}

aout_cali_mode_t SEQ_CV_CaliModeGet(void)
{
  return AOUT_CaliModeGet();
}

const char* SEQ_CV_CaliNameGet(void)
{
  return AOUT_CaliNameGet(SEQ_CV_CaliModeGet());
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set Slewrate
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_SlewRateSet(u8 cv, u8 value)
{
  return AOUT_PinSlewRateSet(cv, value);
}

s32 SEQ_CV_SlewRateGet(u8 cv)
{
  return AOUT_PinSlewRateGet(cv);
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set Gate Inversion
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_GateInversionSet(u8 gate, u8 inverted)
{
  if( gate >= SEQ_CV_NUM )
    return -1; // invalid gate selected

  if( inverted )
    gate_inversion_mask |= (1 << gate);
  else
    gate_inversion_mask &= ~(1 << gate);

  return 0; // no error
}

u8 SEQ_CV_GateInversionGet(u8 gate)
{
  if( gate >= SEQ_CV_NUM )
    return 0; // invalid gate selected

  return (gate_inversion_mask & (1 << gate)) ? 1 : 0;
}


s32 SEQ_CV_GateInversionAllSet(u8 mask)
{
  gate_inversion_mask = mask;
  return 0; // no error
}

u8 SEQ_CV_GateInversionAllGet(void)
{
  return gate_inversion_mask;
}



/////////////////////////////////////////////////////////////////////////////
// Get/Set CV Pitch Range
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_PitchRangeSet(u8 cv, u8 range)
{
  if( cv >= SEQ_CV_NUM )
    return -1; // invalid cv channel selected

  AOUT_PinPitchRangeSet(cv, range);
  return 0; // no error
}

u8 SEQ_CV_PitchRangeGet(u8 cv)
{
  return AOUT_PinPitchRangeGet(cv);
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set DIN Clock Pulsewidth
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_ClkPulseWidthSet(u8 width)
{
  sync_clk_pulsewidth = width;
  return 0; // no error
}

u8 SEQ_CV_ClkPulseWidthGet(void)
{
  return sync_clk_pulsewidth;
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set DIN Clock Divider
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_ClkDividerSet(u16 div)
{
  sync_clk_divider = div;
  return 0; // no error
}

u16 SEQ_CV_ClkDividerGet(void)
{
  return sync_clk_divider;
}


/////////////////////////////////////////////////////////////////////////////
// Updates all CV channels and gates
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_Update(void)
{
  static u8 last_gates = 0xff; // to force an update
  static u8 last_start_stop = 0xff; // to force an update

  // Start/Stop at J5C.A9
  u8 start_stop = SEQ_BPM_IsRunning();
  if( start_stop != last_start_stop ) {
    last_start_stop = start_stop;
#if defined(MIOS32_FAMILY_STM32F10x)
    MIOS32_BOARD_J5_PinSet(9, start_stop);
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J28_PinSet(1, start_stop);
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
  }

  // DIN Sync Pulse at J5C.A8
  if( seq_core_din_sync_pulse_ctr > 1 ) {
#if defined(MIOS32_FAMILY_STM32F10x)
    MIOS32_BOARD_J5_PinSet(8, 1);
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J28_PinSet(0, 1);
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
    --seq_core_din_sync_pulse_ctr;
  } else if( seq_core_din_sync_pulse_ctr == 1 ) {
#if defined(MIOS32_FAMILY_STM32F10x)
    MIOS32_BOARD_J5_PinSet(8, 0);
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J28_PinSet(0, 0);
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif

    seq_core_din_sync_pulse_ctr = 0;
  }

  // update AOUTs
  AOUT_Update();

  // update J5 Outputs (forwarding AOUT digital pins for modules which don't support gates)
  // The MIOS32_BOARD_* function won't forward pin states if J5_ENABLED was set to 0
  u8 gates = AOUT_DigitalPinsGet() ^ gate_inversion_mask;
  if( gates != last_gates ) {
    last_gates = gates;

#ifndef MBSEQV4L
    // MBSEQV4L: allocated by BLM_CHEAPO driver
    int i;
    for(i=0; i<6; ++i) {
      MIOS32_BOARD_J5_PinSet(i, gates & 1);
      gates >>= 1;
    }
#endif

#if defined(MIOS32_FAMILY_STM32F10x)
#ifndef MBSEQV4L
    // J5B.A6 and J5B.A7 allocated by MIDI OUT3
    // therefore Gate 7 and 8 are routed to J5C.A10 and J5C.A11
    MIOS32_BOARD_J5_PinSet(10, (last_gates & 0x40) ? 1 : 0);
    MIOS32_BOARD_J5_PinSet(11, (last_gates & 0x80) ? 1 : 0);
#else
    // MBSEQV4L: Gate 1 and 2 are routed to J5C.A10 and J5C.A11
    MIOS32_BOARD_J5_PinSet(10, (last_gates & 0x01) ? 1 : 0);
    MIOS32_BOARD_J5_PinSet(11, (last_gates & 0x02) ? 1 : 0);
#endif
#elif defined(MIOS32_FAMILY_LPC17xx)
#ifndef MBSEQV4L
    // J5B.A6 and J5B.A7 allocated by MIDI OUT3
    // therefore Gate 7 and 8 are routed to J28.WS and J28.MCLK
    MIOS32_BOARD_J28_PinSet(2, (last_gates & 0x40) ? 1 : 0);
    MIOS32_BOARD_J28_PinSet(3, (last_gates & 0x80) ? 1 : 0);
#else
    // MBSEQV4L: Gate 1 and 2 are routed to J28.WS and J28.MCLK
    MIOS32_BOARD_J28_PinSet(2, (last_gates & 0x01) ? 1 : 0);
    MIOS32_BOARD_J28_PinSet(3, (last_gates & 0x02) ? 1 : 0);
#endif
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Processes a MIDI event for the given CV port
// (currently we only support a single CV port -> the AOUT port)
// returns 1 if event has been taken
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_SendPackage(u8 cv_port, mios32_midi_package_t package)
{
  if( cv_port > 0 )
    return 0; // event not taken

  // Note Off -> Note On with velocity 0
  if( package.event == NoteOff ) {
    package.event = NoteOn;
    package.velocity = 0;
  }

  if( package.event == NoteOn ) {
    // if channel 1..8: set only note value on CV OUT #1..8, triggers Gate #1..8
    // if channel 9..12: set note/velocity on each pin pair (CV OUT #1/2, #3/4, #5/6, #7/8), triggers Gate #1+2, #3+4, #5+6, #7+8
    // if channel 13..15: set velocity/note on each pin pair (CV OUT #1/2, #3/4, #5/6), triggers Gate #1+2, #3+4, #5+6
    // if channel 16: trigger the extension pins (DOUT)

    if( package.chn == Chn16 ) {
      int gate_pin = package.note - 0x24; // C-1 is the base note
      if( gate_pin >= 0 ) {
#ifndef MBSEQV4L
	u8 dout_sr = gate_pin / 8;
	u8 dout_pin = gate_pin % 8;

	if( dout_sr < SEQ_HWCFG_NUM_SR_DOUT_GATES && seq_hwcfg_dout_gate_sr[dout_sr] )
	  MIOS32_DOUT_PinSet((seq_hwcfg_dout_gate_sr[dout_sr]-1)*8 + dout_pin, package.velocity ? 1 : 0);
#endif
      }
    } else {
      int aout_chn_note, aout_chn_vel, gate_pin;

      if( package.chn <= Chn8 ) {
	aout_chn_note = package.chn;
	aout_chn_vel = -1;
	gate_pin = package.chn;
      } else if( package.chn <= Chn12 ) {
	aout_chn_note = ((package.chn & 3) << 1);
	aout_chn_vel = aout_chn_note + 1;
	gate_pin = package.chn & 3;
      } else { // Chn <= 15
	aout_chn_vel = ((package.chn & 3) << 1);
	aout_chn_note = aout_chn_vel + 1;
	gate_pin = package.chn & 3;
      }

      // branch depending on Note On/Off event
      if( package.event == NoteOn && package.velocity > 0 ) {
	// push note into note stack
	NOTESTACK_Push(&cv_notestack[aout_chn_note], package.note, package.velocity);
      } else {
	// remove note from note stack
	NOTESTACK_Pop(&cv_notestack[aout_chn_note], package.note);
      }


      // still a note in stack?
      if( cv_notestack[aout_chn_note].len ) {
	// take first note of stack
	// we have to convert the 7bit value to a 16bit value
	u16 note_cv = cv_notestack_items[aout_chn_note][0].note << 9;
	u16 velocity_cv = cv_notestack_items[aout_chn_note][0].tag << 9;

	// change voltages
	AOUT_PinSet(aout_chn_note, note_cv);
	if( aout_chn_vel >= 0 )
	  AOUT_PinSet(aout_chn_vel, velocity_cv);

	// set gate pin
	if( gate_pin >= 0 )
	  AOUT_DigitalPinSet(gate_pin, 1);
      } else {
	// clear gate pin
	if( gate_pin >= 0 )
	  AOUT_DigitalPinSet(gate_pin, 0);
      }
    }
  } else if( package.event == CC ) {
    // if channel 1..8: sets CV Out #1..8 depending on CC number (16 = #1, 17 = #2, ...) - channel has no effect
    // if channel 9..16: sets CV Out #1..8 depending on channel, always sets Gate #1..8

    int aout_chn, gate_pin;
    if( package.chn <= Chn8 ) {
      aout_chn = package.cc_number - 16;
      gate_pin = aout_chn;
    } else {
      aout_chn = package.chn & 0x7;
      gate_pin = aout_chn;
    }

    // aout_chn could be >= 8, but this doesn't matter... AOUT_PinSet() checks for number of available channels
    // this could be useful for future extensions (e.g. higher number of AOUT Channels)
    if( aout_chn >= 0 )
      AOUT_PinSet(aout_chn, package.value << 9);

    // Gate is always set (useful for controlling pitch of an analog synth where gate is connected as well)
    AOUT_DigitalPinSet(gate_pin, 1);
#if 0
    // not here - all pins are updated at once after AOUT_Update() (see SEQ_TASK_MIDI() in app.c)
    MIOS32_BOARD_J5_PinSet(gate_pin, 1);
#endif

  } else if( package.event == PitchBend ) {
    int aout_chn_note;

    if( package.chn <= Chn8 ) {
      aout_chn_note = package.chn;
    } else if( package.chn <= Chn12 ) {
      aout_chn_note = ((package.chn & 3) << 1);
    } else { // Chn <= 15
      aout_chn_note = ((package.chn & 3) << 1) + 1;
    }

    int pitch = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
    if( pitch > -100 && pitch < 100 )
      pitch = 0;
    AOUT_PinPitchSet(aout_chn_note, pitch);
  }

  return 1; // event taken
}


/////////////////////////////////////////////////////////////////////////////
// Called to reset all channels/notes (e.g. after session change)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_ResetAllChannels(void)
{
  int cv;

  for(cv=0; cv<SEQ_CV_NUM; ++cv) {
    NOTESTACK_Init(&cv_notestack[cv],
		   NOTESTACK_MODE_PUSH_TOP,
		   &cv_notestack_items[cv][0],
		   SEQ_CV_NOTESTACK_SIZE);

    AOUT_PinSet(cv, 0x0000);
    AOUT_PinPitchSet(cv, 0x0000);
  }

  AOUT_DigitalPinsSet(0x00);

#ifndef MBSEQV4L
  int sr;
  for(sr=0; sr<SEQ_HWCFG_NUM_SR_DOUT_GATES; ++sr)
    MIOS32_DOUT_SRSet(sr, seq_hwcfg_dout_gate_sr[sr]);
#endif

  return 0; // no error
}
