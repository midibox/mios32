// $Id$
/*
 * MIDIbox MM V3
 * Motorfader Handler
 *
 * NOT USED ANYMORE AND DISABLED IN mios32_config.h
 * Fader Events are sent by MBHP_MF_NG module which is 
 * connected to MIDI IN/OUT 3 (Port J5B.A6 and A7) or 4 (Port J4B.SC and SD)
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
#include "mm_hwcfg.h"
#include "mm_mf.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// used to store the fader position
static u16 fader_position[8];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the fader positions
/////////////////////////////////////////////////////////////////////////////
s32 MM_MF_Init(u32 mode)
{
  int i;

  // reset fader values
  for(i=0; i<sizeof(fader_position); ++i)
    fader_position[i] = 0;

  // OBSOLETE!
  // we use the MBHP_MF_NG module instead

//#if ENABLE_MOTORDRIVER == 1
//  // configure motorfaders
//  // see http://www.ucapps.de/mbhp_mf.html for details about the parameters
//  int mf;
//  for(mf=1; mf<MIOS32_MF_NUM; ++mf) {
//    mios32_mf_config_t mf_config = MIOS32_MF_ConfigGet(mf);
//    mf_config.cfg.deadband = MIOS32_AIN_DEADBAND;
//    mf_config.cfg.pwm_period = 3;
//    mf_config.cfg.pwm_duty_cycle_up = 1;
//    mf_config.cfg.pwm_duty_cycle_down = 1;
//    MIOS32_MF_ConfigSet(mf, mf_config);
//  }
//#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the stored fader position
/////////////////////////////////////////////////////////////////////////////
u16 MM_MF_FaderPosGet(u8 fader)
{
  return fader_position[fader];
}

/////////////////////////////////////////////////////////////////////////////
// This function has to be called from AIN_NotifyChange when a fader
// has been moved manually
/////////////////////////////////////////////////////////////////////////////
s32 MM_MF_FaderEvent(u8 fader, u16 position_16bit)
{
  // store position
  fader_position[fader] = position_16bit;

  // convert 16-bit conversion result to 2*7bit values
  u8 position_l = (position_16bit >> (2+0)) & 0x7f;
  u8 position_h = (position_16bit >> (2+7)) & 0x7f;
  
  // ensure that fader number is in the range of 0..7
  fader &= 0x07;

  // send MIDI event
  u8 chn = (mm_hwcfg_midi_channel-1) & 0x0f;
  MIOS32_MIDI_SendCC(DEFAULT, chn, 0x00 | fader, position_h);
  MIOS32_MIDI_SendCC(DEFAULT, chn, 0x20 | fader, position_l);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function moves a fader to the given position
// The position is coded in 16bit format
/////////////////////////////////////////////////////////////////////////////
s32 MM_MF_FaderMove(u8 fader, u16 position_16bit)
{
  fader_position[fader] = position_16bit;

#if 0
  // OBSOLETE!
  return MIOS32_MF_FaderMove(fader, position_16bit >> 4); // convert to 12bit
#else
  // we use the MBHP_MF_NG module instead
  // (nothing to do, fader events forwarded by MIDI handler in app.c)
  return 0;
#endif
}
