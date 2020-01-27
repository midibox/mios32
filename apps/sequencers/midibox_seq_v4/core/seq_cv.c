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

#if !defined(MIOS32_DONT_USE_AOUT)

#include <string.h>
#include <aout.h>
#include <notestack.h>


#include "seq_cv.h"
#include "seq_hwcfg.h"
#include "seq_bpm.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// for smokestacksproductions:
// mirror J5.A0 at P0.4 (J18, CAN port) since the original IO doesn't work on his LPCXPRESSO anymore
#define MIRROR_J5_A0_AT_J18 1

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u16 seq_cv_clkout_divider[SEQ_CV_NUM_CLKOUT];
u8  seq_cv_clkout_pulsewidth[SEQ_CV_NUM_CLKOUT];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 gates;
static u32 gate_inversion_mask;
static u32 sus_key_mask;

static u8 dout_trigger_width_ms;
static u8 dout_trigger_pipeline[SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX][SEQ_HWCFG_NUM_SR_DOUT_GATES];

static u8 seq_cv_clkout_pulse_ctr[SEQ_CV_NUM_CLKOUT];


// each channel has an own notestack
static notestack_t cv_notestack[SEQ_CV_NUM];
static notestack_item_t cv_notestack_items[SEQ_CV_NUM][SEQ_CV_NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_Init(u32 mode)
{
  int i;

  gates = 0;
  gate_inversion_mask = 0x00000000;
  sus_key_mask = 0xffffffff;

  dout_trigger_width_ms = 0;

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
#elif defined(MIOS32_FAMILY_STM32F4xx)
  for(i=6; i<7; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);
  // potential conflict with LCD driver, keep these pins in current state
  //for(i=8; i<16; ++i)
  //  MIOS32_BOARD_J10_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);
#elif defined(MIOS32_FAMILY_LPC17xx)
  for(i=0; i<4; ++i)
    MIOS32_BOARD_J28_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);

# if MIRROR_J5_A0_AT_J18
  MIOS32_SYS_LPC_PINSEL(0, 4, 0); // GPIO
  MIOS32_SYS_LPC_PINMODE(0, 4, 0); // doesn't matter
  MIOS32_SYS_LPC_PINDIR(0, 4, 1); // output mode
  MIOS32_SYS_LPC_PINMODE_OD(0, 4, 0); // push-pull
# endif

#else
# warning "please adapt for this MIOS32_FAMILY"
#endif

  // initialize AOUT driver
  AOUT_Init(0);

  // legacy: start with 8 channels
  {
    aout_config_t config;
    config = AOUT_ConfigGet();
    config.num_channels = 8;
    AOUT_ConfigSet(config);
  }

  // initial pulsewidth and divider for clock outputs
  {
      int clkout;

      for(clkout=0; clkout<SEQ_CV_NUM_CLKOUT; ++clkout) {
	seq_cv_clkout_pulsewidth[clkout] = 1;
	seq_cv_clkout_divider[clkout] = 16; // 24 ppqn
	seq_cv_clkout_pulse_ctr[clkout] = 0;
      }
  }

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
  config.num_channels = AOUT_IF_MaxChannelsGet(if_type);
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
// Get number of AOUT channels (depends on device)
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_CV_NumChnGet(void)
{
  aout_config_t config;
  config = AOUT_ConfigGet();
  return config.num_channels;
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

  if( mode < AOUT_NUM_CALI_MODES ) {
    return AOUT_CaliModeSet(cv, mode);
  }

  // set calibration value
  AOUT_CaliModeSet(cv, mode);

  s32 x = mode - AOUT_NUM_CALI_MODES;
  u32 ref_value = x*AOUT_NUM_CALI_POINTS_Y_INTERVAL;
  if( ref_value > 0xffff )
    ref_value = 0xffff;

  return AOUT_CaliCfgValueSet(ref_value);
}

aout_cali_mode_t SEQ_CV_CaliModeGet(void)
{
  return AOUT_CaliModeGet();
}

// str must be reserved for up to 10+1 characters!
s32 SEQ_CV_CaliNameGet(char *str, u8 cv, u8 display_bipolar)
{
  u32 mode = (u32)SEQ_CV_CaliModeGet();

  if( mode < AOUT_NUM_CALI_MODES ) {
    if( display_bipolar ) {
      switch( mode ) {
      case AOUT_CALI_MODE_1V:
	sprintf(str, "  -4.00V  ");
	break;
      case AOUT_CALI_MODE_2V:
	sprintf(str, "  -3.00V  ");
	break;
      case AOUT_CALI_MODE_4V:
	sprintf(str, "  -1.00V  ");
	break;
      case AOUT_CALI_MODE_8V:
	sprintf(str, "   3.00V  ");
	break;
      default:
	sprintf(str, "  %s  ", AOUT_CaliNameGet(mode));
      }
    } else {
      sprintf(str, "  %s  ", AOUT_CaliNameGet(mode));
    }
  } else {
    // calibration values
    u16 *cali_points = SEQ_CV_CaliPointsPtrGet(cv);
    if( cali_points == NULL ) {
      sprintf(str, "!!ERROR!! ");
    } else {
      s32 x = mode - AOUT_NUM_CALI_MODES;
      if( x >= AOUT_NUM_CALI_POINTS_X )
	x = AOUT_NUM_CALI_POINTS_X-1;

      s32 cali_point = cali_points[x] >> 4; // 16bit -> 12bit
      s32 cali_ref = AOUT_CaliCfgValueGet() >> 4; // 16bit -> 12bit

      s32 cali_diff = cali_point - cali_ref;
      if( x >= (AOUT_NUM_CALI_POINTS_X-1) ) {
	sprintf(str, " Max:%5d", cali_diff);
      } else {
	sprintf(str, " %2dV:%5d", display_bipolar ? (x - 5) : x, cali_diff);
      }
    }
  }

  return 0; // no error
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


s32 SEQ_CV_GateInversionAllSet(u32 mask)
{
  gate_inversion_mask = mask;
  return 0; // no error
}

u32 SEQ_CV_GateInversionAllGet(void)
{
  return gate_inversion_mask;
}



/////////////////////////////////////////////////////////////////////////////
// SusKey
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_SusKeySet(u8 gate, u8 sus_key)
{
  if( gate >= SEQ_CV_NUM )
    return -1; // invalid gate selected

  if( sus_key )
    sus_key_mask |= (1 << gate);
  else
    sus_key_mask &= ~(1 << gate);

  return 0; // no error
}

u8 SEQ_CV_SusKeyGet(u8 gate)
{
  if( gate >= SEQ_CV_NUM )
    return 0; // invalid gate selected

  return (sus_key_mask & (1 << gate)) ? 1 : 0;
}


s32 SEQ_CV_SusKeyAllSet(u32 mask)
{
  sus_key_mask = mask;
  return 0; // no error
}

u32 SEQ_CV_SusKeyAllGet(void)
{
  return sus_key_mask;
}


/////////////////////////////////////////////////////////////////////////////
// DOUT Trigger Width
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_DOUT_TriggerWidthSet(u8 width_ms)
{
  if( width_ms > SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX )
    width_ms = SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX; // no error, just saturate

  dout_trigger_width_ms = width_ms;

  // clear trigger pipeline to avoid artifacts
  memset(dout_trigger_pipeline, 0, SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX * SEQ_HWCFG_NUM_SR_DOUT_GATES);

  return 0; // no error
}

u8 SEQ_CV_DOUT_TriggerWidthGet(void)
{
  return dout_trigger_width_ms;
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
// Pointer to calibration values
/////////////////////////////////////////////////////////////////////////////
u16* SEQ_CV_CaliPointsPtrGet(u8 cv)
{
  return AOUT_CaliPointsPtrGet(cv);
}

/////////////////////////////////////////////////////////////////////////////
// Get/Set DIN Clock Pulsewidth
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_ClkPulseWidthSet(u8 clkout, u8 width)
{
  if( clkout >= SEQ_CV_NUM_CLKOUT )
    return -1; // invalid clkout

  seq_cv_clkout_pulsewidth[clkout] = width;
  return 0; // no error
}

u8 SEQ_CV_ClkPulseWidthGet(u8 clkout)
{
  return seq_cv_clkout_pulsewidth[clkout];
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set DIN Clock Divider
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_ClkDividerSet(u8 clkout, u16 div)
{
  if( clkout >= SEQ_CV_NUM_CLKOUT )
    return -1; // invalid clkout

  seq_cv_clkout_divider[clkout] = div;
  return 0; // no error
}

u16 SEQ_CV_ClkDividerGet(u8 clkout)
{
  return seq_cv_clkout_divider[clkout];
}


/////////////////////////////////////////////////////////////////////////////
// Trigger a clock line
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_Clk_Trigger(u8 clkout)
{
  if( clkout >= SEQ_CV_NUM_CLKOUT )
    return -1; // invalid clkout

  seq_cv_clkout_pulse_ctr[clkout] = seq_cv_clkout_pulsewidth[clkout] + 1;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Trigger (or immediately clear) a DOUT
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_DOUT_GateSet(u8 dout, u8 value)
{
  if( dout >= 8*SEQ_HWCFG_NUM_SR_DOUT_GATES )
    return -1; // invalid dout

  u8 dout_sr = dout / 8;
  u8 dout_pin = dout % 8;

  if( dout_sr < SEQ_HWCFG_NUM_SR_DOUT_GATES ) {
    MIOS32_IRQ_Disable(); // should be atomic
    if( value ) {
      int i;

      // this has to be done for all pipeline stages to support triggers
      for(i=0; i<SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX; ++i) {
	dout_trigger_pipeline[i][dout_sr] |= (1 << dout_pin);
      }
    } else {
      // only relevant if pin used as gate instead of trigger
      if( dout_trigger_width_ms == 0 ) {
	dout_trigger_pipeline[0][dout_sr] &= ~(1 << dout_pin);
      }
    }
    MIOS32_IRQ_Enable();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Updates all CV channels and gates
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_Update(void)
{
  static u32 last_gates = 0xffffffff; // to force an update
  static u8 last_start_stop = 0xff; // to force an update

  u8 start_stop = SEQ_BPM_IsRunning();

  // Clock outputs
  // Note: a clock output acts as start/stop if clock divider set to 0
  u8 clk_sr_value = 0;
  {
    int clkout;
    u16 *clk_divider = (u16 *)&seq_cv_clkout_divider[0];
    u8 *pulse_ctr = (u8 *)&seq_cv_clkout_pulse_ctr[0];
    for(clkout=0; clkout<SEQ_CV_NUM_CLKOUT; ++clkout, ++clk_divider, ++pulse_ctr) {
      if( !*clk_divider && start_stop ) {
	clk_sr_value |= (1 << clkout);
      }

      if( *pulse_ctr ) {
	*pulse_ctr -= 1;
	clk_sr_value |= (1 << clkout);
      }
    }
  }

  // Update Clock SR
  if( seq_hwcfg_clk_sr )
    MIOS32_DOUT_SRSet(seq_hwcfg_clk_sr-1, clk_sr_value);

  // Additional IO: Start/Stop at J5C.A9
  if( start_stop != last_start_stop ) {
    last_start_stop = start_stop;

    if( seq_hwcfg_j5_enabled ) {
#if defined(MIOS32_FAMILY_STM32F10x)
      MIOS32_BOARD_J5_PinSet(9, start_stop);
#elif defined(MIOS32_FAMILY_STM32F4xx)
      MIOS32_BOARD_J10_PinSet(9, start_stop);
#elif defined(MIOS32_FAMILY_LPC17xx)
      MIOS32_BOARD_J28_PinSet(1, start_stop);
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
    }
  }

  if( seq_hwcfg_j5_enabled ) {
  // DIN Sync Pulse at J5C.A8
#if defined(MIOS32_FAMILY_STM32F10x)
    MIOS32_BOARD_J5_PinSet(8, (clk_sr_value & 1));
#elif defined(MIOS32_FAMILY_STM32F4xx)
    MIOS32_BOARD_J10_PinSet(8, (clk_sr_value & 1));
#elif defined(MIOS32_FAMILY_LPC17xx)
    MIOS32_BOARD_J28_PinSet(0, (clk_sr_value & 1));
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
  }

  // update J5 Outputs (forwarding AOUT digital pins for modules which don't support gates)
  // The MIOS32_BOARD_* function won't forward pin states if J5_ENABLED was set to 0
  u32 new_gates = gates ^ gate_inversion_mask;
  if( new_gates != last_gates ) {
    last_gates = new_gates;

    AOUT_DigitalPinsSet(new_gates);

    {
      int sr;
      for(sr=0; sr<SEQ_HWCFG_NUM_SR_CV_GATES; ++sr) {
	if( seq_hwcfg_cv_gate_sr[sr] )
	  MIOS32_DOUT_SRSet(seq_hwcfg_cv_gate_sr[sr]-1, (new_gates >> sr*8) & 0xff);
      }
    }

    if( seq_hwcfg_j5_enabled ) {
#ifndef MBSEQV4L
      // MBSEQV4L: allocated by BLM_CHEAPO driver
      int i;
      u8 tmp = new_gates;
      for(i=0; i<6; ++i) {
	MIOS32_BOARD_J5_PinSet(i, tmp & 1);
	tmp >>= 1;
      }
#endif

#if MIRROR_J5_A0_AT_J18 && defined(MIOS32_FAMILY_LPC17xx)
      MIOS32_SYS_LPC_PINSET(0, 4, (new_gates & 1));
#endif

#if defined(MIOS32_FAMILY_STM32F10x)
#ifndef MBSEQV4L
      // J5B.A6 and J5B.A7 allocated by MIDI OUT3
      // therefore Gate 7 and 8 are routed to J5C.A10 and J5C.A11
      // MBSEQV4L: use shift register gates!
      MIOS32_BOARD_J5_PinSet(10, (new_gates & 0x40) ? 1 : 0);
      MIOS32_BOARD_J5_PinSet(11, (new_gates & 0x80) ? 1 : 0);
#endif
#elif defined(MIOS32_FAMILY_STM32F4xx)
#ifndef MBSEQV4L
      // no special routing required for STM32F4
      // MBSEQV4L: use shift register gates!
      MIOS32_BOARD_J5_PinSet(6, (new_gates & 0x40) ? 1 : 0);
      MIOS32_BOARD_J5_PinSet(7, (new_gates & 0x80) ? 1 : 0);
#endif
#elif defined(MIOS32_FAMILY_LPC17xx)
#ifndef MBSEQV4L
      // J5B.A6 and J5B.A7 allocated by MIDI OUT3
      // therefore Gate 7 and 8 are routed to J28.WS and J28.MCLK
      MIOS32_BOARD_J28_PinSet(2, (new_gates & 0x40) ? 1 : 0);
      MIOS32_BOARD_J28_PinSet(3, (new_gates & 0x80) ? 1 : 0);
#else
      // MBSEQV4L: Gate 1 and 2 are routed to J28.WS and J28.MCLK
      MIOS32_BOARD_J28_PinSet(2, (new_gates & 0x01) ? 1 : 0);
      MIOS32_BOARD_J28_PinSet(3, (new_gates & 0x02) ? 1 : 0);
#endif
#else
# warning "please adapt for this MIOS32_FAMILY"
#endif
    }
  }

  // update AOUTs
  AOUT_Update();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from APP_SRIO_ServicePrepare() to update the DOUT triggers
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_SRIO_Prepare(void)
{
#ifndef MBSEQV4L
  int i;
  u8 sr;

  for(i=0; i<SEQ_HWCFG_NUM_SR_DOUT_GATES; ++i) {
    if( (sr=seq_hwcfg_dout_gate_sr[i]) > 0 ) {
      MIOS32_DOUT_SRSet(sr-1, dout_trigger_pipeline[0][i]);
    }
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called from APP_SRIO_ServiceFinish() to shift the pipeline
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CV_SRIO_Finish(void)
{
  // shift pipeline

  if( dout_trigger_width_ms > 0 ) {
    u8 width = (dout_trigger_width_ms < SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX) ? dout_trigger_width_ms : SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX;

    memcpy(&dout_trigger_pipeline[0][0], &dout_trigger_pipeline[1][0], SEQ_HWCFG_NUM_SR_DOUT_GATES*(width-1));
    memset(&dout_trigger_pipeline[width-1][0], 0, SEQ_HWCFG_NUM_SR_DOUT_GATES);
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
  if( cv_port > (SEQ_CV_NUM/8) )
    return 0; // event not taken

  // Note Off -> Note On with velocity 0
  if( package.event == NoteOff ) {
    package.event = NoteOn;
    package.velocity = 0;
  }

  if( package.event == NoteOn ) {
    // if channel 1..8: set only note value on CV OUT #1..8, triggers Gate #1..8
    // if channel 9..12: set note/velocity on each pin pair (CV OUT #1/2, #3/4, #5/6, #7/8), triggers Gate #1/3/4/6 and Gate 2/4/5/8 if velocity > 100
    // if channel 13..15: set velocity/note on each pin pair (CV OUT #1/2, #3/4, #5/6), triggers Gate #1/2/3/4 and Gate 5/6/7/8 if velocity > 100
    // if channel 16: trigger the extension pins (DOUT)

    if( package.chn == Chn16 ) {
      int dout_pin = package.note - 0x24; // C-1 is the base note
      if( dout_pin >= 0 ) {
	SEQ_CV_DOUT_GateSet(dout_pin, (package.velocity > 0) ? 1 : 0);
      }
    } else {
      int aout_chn_note, aout_chn_vel, gate_pin_normal, gate_pin_velocity_gt100;

      if( package.chn <= Chn8 ) {
	aout_chn_note = package.chn + 8*cv_port;
	aout_chn_vel = -1;
	gate_pin_normal = package.chn + 8*cv_port;
	gate_pin_velocity_gt100 = -1; // not relevant
      } else if( package.chn <= Chn12 ) {
	aout_chn_note = ((package.chn & 3) << 1) + 8*cv_port;
	aout_chn_vel = aout_chn_note + 1;
	gate_pin_normal = ((package.chn & 3) << 1) + 8*cv_port;
	gate_pin_velocity_gt100 = gate_pin_normal + 1;
      } else { // Chn <= 15
	aout_chn_vel = ((package.chn & 3) << 1) + 8*cv_port;
	aout_chn_note = aout_chn_vel + 1;
	gate_pin_normal = ((package.chn & 3) << 1) + 8*cv_port;
	gate_pin_velocity_gt100 = gate_pin_normal + 1;
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
	AOUT_PinSlewRateEnableSet(aout_chn_note, ((cv_notestack[aout_chn_note].len >= 2) && (sus_key_mask & (1 << aout_chn_note))) ? 1 : 0);
	AOUT_PinSet(aout_chn_note, note_cv);
	if( aout_chn_vel >= 0 )
	  AOUT_PinSet(aout_chn_vel, velocity_cv);

	// set gate pins
	if( gate_pin_normal >= 0 )
	  gates |= (1 << gate_pin_normal);

	if( gate_pin_velocity_gt100 >= 0 ) {
	  if( package.velocity > 100 )
	    gates |= (1 << gate_pin_velocity_gt100);
	  else
	    gates |= (1 << gate_pin_velocity_gt100);
	}
      } else {
	// clear gate pins
	if( gate_pin_normal >= 0 )
	  gates &= ~(1 << gate_pin_normal);

	if( gate_pin_velocity_gt100 >= 0 )
	  gates &= ~(1 << gate_pin_velocity_gt100);
      }
    }
  } else if( package.event == CC ) {
    // if channel 1..8: sets CV Out #1..8 depending on CC number (16 = #1, 17 = #2, ...) - channel has no effect
    // if channel 9..16: sets CV Out #1..8 depending on channel, always sets Gate #1..8

    int aout_chn, gate_pin;
    if( package.chn <= Chn8 ) {
      aout_chn = (package.cc_number - 16) + 8*cv_port;
      gate_pin = aout_chn;
    } else {
      aout_chn = (package.chn & 0x7) + 8*cv_port;
      gate_pin = aout_chn;
    }

    // aout_chn could be >= 8, but this doesn't matter... AOUT_PinSet() checks for number of available channels
    // this could be useful for future extensions (e.g. higher number of AOUT Channels)
    if( aout_chn >= 0 ) {
      AOUT_PinSlewRateEnableSet(aout_chn, 1);
      AOUT_PinSet(aout_chn, package.value << 9);
    }

    // Gate is always set (useful for controlling pitch of an analog synth where gate is connected as well)
    gates |= (1 << gate_pin);
#if 0
    // not here - all pins are updated at once after AOUT_Update() (see SEQ_TASK_MIDI() in app.c)
    MIOS32_BOARD_J5_PinSet(gate_pin, 1);
#endif

  } else if( package.event == PitchBend ) {
    int aout_chn_note;

    if( package.chn <= Chn8 ) {
      aout_chn_note = package.chn + 8*cv_port;
    } else if( package.chn <= Chn12 ) {
      aout_chn_note = ((package.chn & 3) << 1) + 8*cv_port;
    } else { // Chn <= 15
      aout_chn_note = ((package.chn & 3) << 1) + 1 + 8*cv_port;
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

    AOUT_PinSlewRateEnableSet(cv, 1);
    AOUT_PinSet(cv, 0x0000);
    AOUT_PinPitchSet(cv, 0x0000);
  }

  gates = 0x00;

  memset(dout_trigger_pipeline, 0, SEQ_CV_DOUT_TRIGGER_WIDTH_MS_MAX * SEQ_HWCFG_NUM_SR_DOUT_GATES);

  return 0; // no error
}
#endif
