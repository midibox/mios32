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

static u8 mbcv_button_matrix_column;

static u8 mbcv_button_matrix_row_values[MBCV_BUTTON_MATRIX_ROWS];
static u8 mbcv_button_matrix_row_changed[MBCV_BUTTON_MATRIX_ROWS];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_Init(u32 mode)
{
  int i;

  for(i=0; i<MBCV_BUTTON_MATRIX_ROWS; ++i) {
    mbcv_button_matrix_row_values[i] = 0xff;
    mbcv_button_matrix_row_changed[i] = 0;
  }

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

static s32 MBCV_BUTTON_KeyboardKey(u8 depressed, u8 key)
{
  MbCvEnvironment* env = APP_GetEnv();
  u8 selectedCv = SCS_CONFIG_CvGet();

  u8 note = 0x30 + key;
  u8 velocity = 100; // make it configurable?


  if( depressed ) {
    env->mbCv[selectedCv].noteOff(note, false);
  } else {
    env->mbCv[selectedCv].noteOn(note, velocity, false);
  }

  DEBUG_MSG("Key %d %d\n", key, depressed);

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

  if( pin == mbcv_hwcfg_button.scs_exit ) {
    return SCS_DIN_NotifyToggle(SCS_PIN_EXIT, depressed);
  }

  if( pin == mbcv_hwcfg_button.scs_shift ) {
    return SCS_DIN_NotifyToggle(SCS_PIN_SOFT1 + SCS_NumMenuItemsGet(), depressed);
  }

  for(int i=0; i<SCS_NUM_MENU_ITEMS; ++i)
    if( pin == mbcv_hwcfg_button.scs_soft[i] ) {
      return SCS_DIN_NotifyToggle(SCS_PIN_SOFT1 + i, depressed);
    }

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

  for(int i=0; i<MBCV_BUTTON_KEYBOARD_KEYS_NUM; ++i)
    if( pin == mbcv_hwcfg_button.keyboard[i] )
      return MBCV_BUTTON_KeyboardKey(depressed, i);
      
  // always print debugging message
#if 1
  MUTEX_MIDIOUT_TAKE;
  u8 sr = (pin >> 3) + 1;

  if( sr >= 17 ) {
    DEBUG_MSG("[MBCV_BUTTON_Handler] Button SR:M%d, Pin:%d not mapped, it has been %s.\n", 
	      sr-16, pin & 7, depressed ? "depressed" : "pressed");
  } else {
    DEBUG_MSG("[MBCV_BUTTON_Handler] Button SR:%d, Pin:%d not mapped, it has been %s.\n", 
	      sr, pin & 7, depressed ? "depressed" : "pressed");
  }
  MUTEX_MIDIOUT_GIVE;
#endif

  return -1; // button not mapped
}


/////////////////////////////////////////////////////////////////////////////
//! This function prepares the DOUT register to drive a row.<BR>
//! It should be called from the APP_SRIO_ServicePrepare()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_MATRIX_PrepareCol(void)
{
  // increment row, wrap at 8
  if( ++mbcv_button_matrix_column >= MBCV_BUTTON_MATRIX_COLUMNS )
    mbcv_button_matrix_column = 0;

  // select next DIN row (selected cathode line = 0, all others 1)
  u8 dout_value = ~(1 << mbcv_button_matrix_column);

  // output on row selection SR
  if( mbcv_hwcfg_bm.dout_select_sr ) {
    MIOS32_DOUT_SRSet(mbcv_hwcfg_bm.dout_select_sr-1, dout_value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected row
// It should be called from the APP_SRIO_ServiceFinish hook
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_MATRIX_GetRow(void)
{
  // previously scanned column is:
  int column = (mbcv_button_matrix_column-1) & (MBCV_BUTTON_MATRIX_COLUMNS-1);

  // check DINs
  int i;
  for(i=0; i<(MBCV_BUTTON_MATRIX_ROWS/8); ++i) {
    u8 sr = mbcv_hwcfg_bm.din_scan_sr[i];

    if( sr ) {
      MIOS32_DIN_SRChangedGetAndClear(sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      u8 sr_value = MIOS32_DIN_SRGet(sr-1);

      int selected_row = i*8 + column;

      // determine pin changes
      u8 changed = sr_value ^ mbcv_button_matrix_row_values[selected_row];

      if( changed ) {
	// add them to existing notifications
	mbcv_button_matrix_row_changed[selected_row] |= changed;

	// store new value
	mbcv_button_matrix_row_values[selected_row] = sr_value;
      }
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function should be called from a task to check for button changes
//! periodically.
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_BUTTON_MATRIX_Handler(void)
{
  // check all shift registers for DIN pin changes
  int row;
  for(row=0; row<MBCV_BUTTON_MATRIX_ROWS; ++row) {
    // check if there are pin changes - must be atomic!
    MIOS32_IRQ_Disable();
    u8 changed = mbcv_button_matrix_row_changed[row];
    mbcv_button_matrix_row_changed[row] = 0;
    MIOS32_IRQ_Enable();

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    int sr_pin;
    for(sr_pin=0; sr_pin<8; ++sr_pin) {
      if( changed & (1 << sr_pin) ) {
	u8 value = (mbcv_button_matrix_row_values[row] & (1 << sr_pin)) ? 1 : 0;
	//DEBUG_MSG("DIN Matrix row=%d pin=%d value=%d\n", row, sr_pin, value);
	MBCV_BUTTON_Handler(16*8 + row*MBCV_BUTTON_MATRIX_COLUMNS + sr_pin, value);
      }
    }
  }

  return 0; // no error
}
