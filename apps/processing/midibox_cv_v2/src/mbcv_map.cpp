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
#include <app.h>
#include <MbCvEnvironment.h>


#include "mbcv_map.h"
#include "mbcv_patch.h"
#include "mbcv_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MBCV_MAP_Init(u32 mode)
{
  // initialize AOUT driver
  AOUT_Init(0);

  // change default config to AOUT by default
  MBCV_MAP_IfSet(AOUT_IF_MAX525);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name AOUT interface
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MBCV_MAP_IfSet(aout_if_t if_type)
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

extern "C" aout_if_t MBCV_MAP_IfGet(void)
{
  aout_config_t config;
  config = AOUT_ConfigGet();
  return config.if_type;
}


// will return 8 characters
extern "C" const char* MBCV_MAP_IfNameGet(aout_if_t if_type)
{
  return AOUT_IfNameGet(if_type);
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Name Calibration Mode
/////////////////////////////////////////////////////////////////////////////

extern "C" s32 MBCV_MAP_CaliModeSet(u8 cv, aout_cali_mode_t mode)
{
  if( cv >= CV_SE_NUM )
    return -1; // invalid cv channel selected

  if( mode >= MBCV_MAP_NUM_CALI_MODES )
    return -2; // invalid mode selected

  return AOUT_CaliModeSet(cv, mode);

}

extern "C" aout_cali_mode_t MBCV_MAP_CaliModeGet(void)
{
  return AOUT_CaliModeGet();
}

extern "C" const char* MBCV_MAP_CaliNameGet(void)
{
  return AOUT_CaliNameGet(MBCV_MAP_CaliModeGet());
}


/////////////////////////////////////////////////////////////////////////////
// Updates all CV channels and gates
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MBCV_MAP_Update(void)
{
  // retrieve the AOUT values of all channels
  MbCvEnvironment* env = APP_GetEnv();

  u16 *out = env->cvOut.first();
  for(int cv=0; cv<CV_SE_NUM; ++cv, ++out)
    AOUT_PinSet(cv, *out);

  // update AOUTs
  AOUT_Update();

  AOUT_DigitalPinsSet(env->cvGates);

  // gates and DIN sync signals are available at shift registers
  u8 gates = AOUT_DigitalPinsGet();
  u8 din_sync_value = 0;
  {
    if( env->mbCvClock.isRunning )
      din_sync_value |= (1 << 0); // START/STOP

    din_sync_value |= env->mbCvClock.externalClocks << 1;
  }

  // following DOUT transfers should be atomic to ensure, that all pins are updated at the same scan cycle
  MIOS32_IRQ_Disable();
  if( mbcv_hwcfg_dout.gate_sr )
    MIOS32_DOUT_SRSet(mbcv_hwcfg_dout.gate_sr-1, gates);
  if( mbcv_hwcfg_dout.clk_sr )
    MIOS32_DOUT_SRSet(mbcv_hwcfg_dout.clk_sr-1, din_sync_value);
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called to reset all channels/notes (e.g. after session change)
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MBCV_MAP_ResetAllChannels(void)
{
  MbCvEnvironment* env = APP_GetEnv();
  int cv;

  for(cv=0; cv<CV_SE_NUM; ++cv)
    env->mbCv[cv].noteAllOff(false);

  // reset AOUT voltages
  for(cv=0; cv<CV_SE_NUM; ++cv) {
    AOUT_PinPitchSet(cv, 0x0000);
    AOUT_PinSet(cv, 0x0000);
  }

  return 0; // no error
}
