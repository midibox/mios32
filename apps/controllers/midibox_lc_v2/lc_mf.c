// $Id$
/*
 * MIDIbox LC V2
 * Motorfader Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "lc_hwcfg.h"
#include "lc_mf.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// used to store the fader position
static u16 fader_position[8];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the fader positions
/////////////////////////////////////////////////////////////////////////////
s32 LC_MF_Init(u32 mode)
{
  int i;

  // reset fader values
  for(i=0; i<sizeof(fader_position); ++i)
    fader_position[i] = 0;

#if ENABLE_MOTORDRIVER == 1
  // configure motorfaders
  // see http://www.ucapps.de/mbhp_mf.html for details about the parameters
  int mf;
  for(mf=1; mf<MIOS32_MF_NUM; ++mf) {
    mios32_mf_config_t mf_config = MIOS32_MF_ConfigGet(mf);
    mf_config.cfg.deadband = MIOS32_AIN_DEADBAND;
    mf_config.cfg.pwm_period = 3;
    mf_config.cfg.pwm_duty_cycle_up = 1;
    mf_config.cfg.pwm_duty_cycle_down = 1;
    MIOS32_MF_ConfigSet(mf, mf_config);
  }
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the stored fader position
/////////////////////////////////////////////////////////////////////////////
u16 LC_MF_FaderPosGet(u8 fader)
{
  return fader_position[fader];
}

/////////////////////////////////////////////////////////////////////////////
// This function has to be called from AIN_NotifyChange when a fader
// has been moved manually
/////////////////////////////////////////////////////////////////////////////
s32 LC_MF_FaderEvent(u8 fader, u16 position_16bit)
{
#if TOUCH_SENSOR_MODE >= 2
  // in this mode, no value will be sent if touch sensor not active
  if( !MIOS32_MF_SuspendGet(fader) )
    return;
#endif

  // store position
  fader_position[fader] = position_16bit;

  // convert 16-bit conversion result to 14bit value
  u16 value_14bit = position_16bit >> 2;

  // send Pitch Bender event and exit
  return MIOS32_MIDI_SendPitchBend(DEFAULT, fader, value_14bit);
}


/////////////////////////////////////////////////////////////////////////////
// This function moves a fader to the given position
// The position is coded in 16bit format
/////////////////////////////////////////////////////////////////////////////
s32 LC_MF_FaderMove(u8 fader, u16 position_16bit)
{
  fader_position[fader] = position_16bit;
  return MIOS32_MF_FaderMove(fader, position_16bit >> 4); // convert to 12bit
}
