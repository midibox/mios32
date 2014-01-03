// $Id$
/*
 * MIDIbox CV V2 Button functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include <app.h>
#include <scs.h>
#include <MbCvEnvironment.h>

#include "mbcv_button.h"
#include "tasks.h"
#include "mbcv_hwcfg.h"
#include "mbcv_file_hw.h"
#include "mbcv_lre.h"
#include "scs_config.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Button functions
/////////////////////////////////////////////////////////////////////////////
static s32 MBCV_BUTTON_Cv(u8 depressed, u8 cv)
{
  if( !depressed ) {
    SCS_CONFIG_CvSet(cv);
  }

  return 0; // no error
}


static s32 MBCV_BUTTON_EncBank(u8 depressed, u8 bank)
{
  if( !depressed ) {
    MBCV_LRE_BankSet(bank);
  }

  return 0; // no error
}


static s32 MBCV_BUTTON_CvAndEncBank(u8 depressed, u8 cv)
{
  s32 status = 0;

  status |= MBCV_BUTTON_Cv(depressed, cv);
  status |= MBCV_BUTTON_EncBank(depressed, cv);

  return status;
}


static s32 MBCV_BUTTON_Lfo(u8 depressed, u8 lfo)
{
  if( !depressed ) {
    SCS_CONFIG_LfoSet(lfo);
  }

  return 0; // no error
}


static s32 MBCV_BUTTON_Env(u8 depressed, u8 env)
{
  if( !depressed ) {
    SCS_CONFIG_EnvSet(env);
  }

  return 0; // no error
}


static s32 MBCV_BUTTON_Scope(u8 depressed, u8 scope)
{
  if( !depressed ) {
    SCS_CONFIG_ScopeSet(scope);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_DIN_NotifyToggle
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_Handler(u32 pin, u8 depressed)
{
  // ignore as long as hardware config hasn't been read
  if( !MBCV_FILE_HW_ConfigLocked() )
    return -1;

  // request display update
  SCS_DisplayUpdateRequest();

  // stop current message if (new) button has been pressed
  if( !depressed )
    SCS_MsgStop();

  for(int i=0; i<CV_SE_NUM; ++i)
    if( pin == mbcv_hwcfg_button.cv[i] )
      return MBCV_BUTTON_Cv(depressed, i);

  for(int i=0; i<MBCV_LRE_NUM_BANKS; ++i)
    if( pin == mbcv_hwcfg_button.enc_bank[i] )
      return MBCV_BUTTON_EncBank(depressed, i);

  for(int i=0; i<CV_SE_NUM; ++i)
    if( pin == mbcv_hwcfg_button.cv_and_enc_bank[i] )
      return MBCV_BUTTON_CvAndEncBank(depressed, i);

  if( pin == mbcv_hwcfg_button.lfo1 )
    return MBCV_BUTTON_Lfo(depressed, 0);
  if( pin == mbcv_hwcfg_button.lfo2 )
    return MBCV_BUTTON_Lfo(depressed, 1);
  if( pin == mbcv_hwcfg_button.env1 )
    return MBCV_BUTTON_Env(depressed, 0);
  if( pin == mbcv_hwcfg_button.env2 )
    return MBCV_BUTTON_Env(depressed, 1);

  for(int i=0; i<CV_SCOPE_NUM; ++i)
    if( pin == mbcv_hwcfg_button.scope[i] )
      return MBCV_BUTTON_Scope(depressed, i);

  // always print debugging message
#if 1
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("[MBCV_BUTTON_Handler] Button SR:%d, Pin:%d not mapped, it has been %s.\n", 
            (pin >> 3) + 1,
            pin & 7,
            depressed ? "depressed" : "pressed");
  MUTEX_MIDIOUT_GIVE;
#endif

  return -1; // button not mapped
}
