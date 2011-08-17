// $Id$
/*
 * MIDIbox CV V2 mapping functions
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
#include <seq_bpm.h>


#include "mbcv_map.h"
#include "mbcv_midi.h"
#include "mbcv_patch.h"



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// TODO
static u8 seq_core_din_sync_pulse_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_Init(u32 mode)
{
  int i;

  // initialize J5 pins
  // they will be enabled after MBSEQ_HW.V4 has been read
  // as long as this hasn't been done, activate pull-downs
  for(i=0; i<12; ++i)
    MIOS32_BOARD_J5_PinInit(i, MIOS32_BOARD_PIN_MODE_INPUT_PD);

  // initialize AOUT driver
  AOUT_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name AOUT interface
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_IfSet(aout_if_t if_type)
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

aout_if_t MBCV_MAP_IfGet(void)
{
  aout_config_t config;
  config = AOUT_ConfigGet();
  return config.if_type;
}


// will return 8 characters
const char* MBCV_MAP_IfNameGet(void)
{
  return AOUT_IfNameGet(MBCV_MAP_IfGet());
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name CV Curve
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_CurveSet(u8 cv, u8 curve)
{
  if( cv >= MBCV_PATCH_NUM_CV )
    return -1; // invalid cv channel selected

  if( curve >= MBCV_MAP_NUM_CURVES )
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

u8 MBCV_MAP_CurveGet(u8 cv)
{
  if( cv >= MBCV_PATCH_NUM_CV )
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
const char* MBCV_MAP_CurveNameGet(u8 cv)
{

  if( cv >= MBCV_PATCH_NUM_CV )
    return "------";

  return curve_desc[MBCV_MAP_CurveGet(cv)];
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name Calibration Mode
/////////////////////////////////////////////////////////////////////////////

s32 MBCV_MAP_CaliModeSet(u8 cv, aout_cali_mode_t mode)
{
  if( cv >= MBCV_PATCH_NUM_CV )
    return -1; // invalid cv channel selected

  if( mode >= MBCV_MAP_NUM_CALI_MODES )
    return -2; // invalid mode selected

  return AOUT_CaliModeSet(cv, mode);

}

aout_cali_mode_t MBCV_MAP_CaliModeGet(void)
{
  return AOUT_CaliModeGet();
}

const char* MBCV_MAP_CaliNameGet(void)
{
  return AOUT_CaliNameGet(MBCV_MAP_CaliModeGet());
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set Slewrate
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_SlewRateSet(u8 cv, u8 value)
{
  return AOUT_PinSlewRateSet(cv, value);
}

s32 MBCV_MAP_SlewRateGet(u8 cv)
{
  return AOUT_PinSlewRateGet(cv);
}


/////////////////////////////////////////////////////////////////////////////
// Updates all CV channels and gates
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_Update(void)
{
  static u8 last_gates = 0xff; // to force an update
  static u8 last_start_stop = 0xff; // to force an update

  // Start/Stop at J5C.A9
  u8 start_stop = SEQ_BPM_IsRunning();
  if( start_stop != last_start_stop ) {
    last_start_stop = start_stop;
    MIOS32_BOARD_J5_PinSet(9, start_stop);
  }

  // DIN Sync Pulse at J5C.A8
  if( seq_core_din_sync_pulse_ctr > 1 ) {
    MIOS32_BOARD_J5_PinSet(8, 1);
    --seq_core_din_sync_pulse_ctr;
  } else if( seq_core_din_sync_pulse_ctr == 1 ) {
    MIOS32_BOARD_J5_PinSet(8, 0);
    seq_core_din_sync_pulse_ctr = 0;
  }

  // update AOUTs
  AOUT_Update();

  // update J5 Outputs (forwarding AOUT digital pins for modules which don't support gates)
  // The MIOS32_BOARD_* function won't forward pin states if J5_ENABLED was set to 0
  u8 gates = AOUT_DigitalPinsGet() ^ mbcv_patch_gate_inverted[0];
  if( gates != last_gates ) {
    int i;

    last_gates = gates;
    for(i=0; i<6; ++i) {
      MIOS32_BOARD_J5_PinSet(i, gates & 1);
      gates >>= 1;
    }
    // J5B.A6 and J5B.A7 allocated by MIDI OUT3
    // therefore Gate 7 and 8 are routed to J5C.A10 and J5C.A11
    MIOS32_BOARD_J5_PinSet(10, gates & 1);
    gates >>= 1;
    MIOS32_BOARD_J5_PinSet(11, gates & 1);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called to reset all channels/notes (e.g. after session change)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_MAP_ResetAllChannels(void)
{
  int cv;

  // reset all notestacks
  MBCV_MIDI_Init(0);

  // reset AOUT voltages
  for(cv=0; cv<MBCV_PATCH_NUM_CV; ++cv) {
    AOUT_PinSet(cv, 0x0000);
    AOUT_PinPitchSet(cv, 0x0000);
  }

  // clear pins
  AOUT_DigitalPinsSet(0x00);

  int sr;
  for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
    MIOS32_DOUT_SRSet(sr, mbcv_midi_dout_gate_sr[sr]);

  return 0; // no error
}
