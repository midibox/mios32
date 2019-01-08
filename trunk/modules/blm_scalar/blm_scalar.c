// $Id$
//! \defgroup BLM_SCALAR
//!
//! BLM_SCALAR Driver
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "blm_scalar.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access (bypasses BLM_SCALAR_DOUT_SR* functions)
u8 blm_scalar_led[BLM_SCALAR_NUM_MODULES][8][BLM_SCALAR_NUM_COLOURS];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static blm_scalar_config_t blm_scalar_config;

static u8 blm_scalar_selected_row;

static u8 blm_scalar_button_debounce_ctr;

static u8 blm_scalar_button_row_values[BLM_SCALAR_NUM_MODULES][8];
static u8 blm_scalar_button_row_changed[BLM_SCALAR_NUM_MODULES][8];



/////////////////////////////////////////////////////////////////////////////
//! Initializes the BLM_SCALAR driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // define default configuration (can be changed during runtime)
  int mod;
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int mod_sr_offset = (BLM_SCALAR_NUM_COLOURS+1)*mod;
    blm_scalar_config.dout_cathodes[mod] = mod_sr_offset + 1;

    int colour;
    for(colour=0; colour<BLM_SCALAR_NUM_COLOURS; ++colour)
      blm_scalar_config.dout[mod][colour] = mod_sr_offset + colour + 2; // offset 2..3(..4)

    blm_scalar_config.din[mod] = mod + 1;
  }

  blm_scalar_config.cathodes_inv_mask = 0xff; // sink drivers connected
  blm_scalar_config.debounce_delay = 10;


  // set button value to initial value (1) and clear "changed" status of all 64 buttons
  // clear all LEDs
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int row;

    for(row=0; row<8; ++row) {
      blm_scalar_button_row_values[mod][row] = 0xff;
      blm_scalar_button_row_changed[mod][row] = 0x00;

      int colour;
      for(colour=0; colour<BLM_SCALAR_NUM_COLOURS; ++colour)
	blm_scalar_led[mod][row][colour] = 0x00;
    }
  }

  // clear debounce counter
  blm_scalar_button_debounce_ctr = 0;

  // select first row
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    if( blm_scalar_config.dout_cathodes[mod] )
      MIOS32_DOUT_SRSet(blm_scalar_config.dout_cathodes[mod]-1, (1 << 0) ^ blm_scalar_config.cathodes_inv_mask);
  }

  // remember that this row has been selected
  blm_scalar_selected_row = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Configures the BLM_SCALAR driver.
//!
//! See blm_scalar.h for more informations
//!
//! \param[in] config click on blm_scalar_config_t declaration to display the members
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_ConfigSet(blm_scalar_config_t config)
{
  // ensure that update is atomic
  MIOS32_IRQ_Disable();
  blm_scalar_config = config;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns current BLM configuration
//! \return config structure see BLM_SCALAR_ConfigSet()
/////////////////////////////////////////////////////////////////////////////
blm_scalar_config_t BLM_SCALAR_ConfigGet(void)
{
  return blm_scalar_config;
}


/////////////////////////////////////////////////////////////////////////////
//! This function prepares the DOUT register to drive a row.<BR>
//! It should be called from the APP_SRIO_ServicePrepare()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_PrepareCol(void)
{
  u8 row_offset;

  // increment row, wrap at 8
  if( ++blm_scalar_selected_row >= 8 )
    blm_scalar_selected_row = 0;

  // select next DOUT/DIN row (selected cathode line = 0, all others 1)
  u8 dout_value = ~(1 << blm_scalar_selected_row);

  // apply inversion mask
  dout_value ^= blm_scalar_config.cathodes_inv_mask;

  // output on CATHODES* registers
  int mod;
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int sr = blm_scalar_config.dout_cathodes[mod];
    if( sr )
      MIOS32_DOUT_SRSet(sr-1, dout_value);
  }

  // output colours
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int colour;
    for(colour=0; colour<BLM_SCALAR_NUM_COLOURS; ++colour) {
      int sr = blm_scalar_config.dout[mod][colour];
      if( sr )
	MIOS32_DOUT_SRSet(sr-1, blm_scalar_led[mod][blm_scalar_selected_row][colour]);
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function gets the DIN values of the selected row.<BR>
//! It should be called from the APP_SRIO_ServiceFinish() hook
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_GetRow(void)
{
  u8 sr, sr_value;
  u8 side;

  // decrememt debounce counter if > 0
  if( blm_scalar_button_debounce_ctr )
    --blm_scalar_button_debounce_ctr;

  // since the row line of the buttons is identical to the row line of the LEDs,
  // we can derive the button row offset from blm_scalar_selected_row
  int selected_row = (blm_scalar_selected_row-1) & 0x7;

  // check DINs
  int mod;
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int sr = blm_scalar_config.din[mod];

    if( sr ) {
      MIOS32_DIN_SRChangedGetAndClear(sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value = MIOS32_DIN_SRGet(sr-1);

      // ignore as long as debounce counter != 0
      if( !blm_scalar_button_debounce_ctr ) {
	// determine pin changes
	u8 changed = sr_value ^ blm_scalar_button_row_values[mod][selected_row];

	if( changed ) {
	  // add them to existing notifications
	  blm_scalar_button_row_changed[mod][selected_row] |= changed;

	  // store new value
	  blm_scalar_button_row_values[mod][selected_row] = sr_value;

	  // reload debounce counter if any pin has changed
	  blm_scalar_button_debounce_ctr = blm_scalar_config.debounce_delay;
	}
      }
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from a task to check for button changes
//! periodically. Events (change from 0->1 or from 1->0) will be notified 
//! via the given callback function <notify_hook> with following parameters:
//!   <notifcation-hook>(s32 pin, s32 value)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_ButtonHandler(void *_notify_hook)
{
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  // check all shift registers for DIN pin changes
  int mod;
  for(mod=0; mod<BLM_SCALAR_NUM_MODULES; ++mod) {
    int row;
    for(row=0; row<8; ++row) {
      // check if there are pin changes - must be atomic!
      MIOS32_IRQ_Disable();
      u8 changed = blm_scalar_button_row_changed[mod][row];
      blm_scalar_button_row_changed[mod][row] = 0;
      MIOS32_IRQ_Enable();

      // any pin change at this SR?
      if( !changed )
	continue;

      // check all 8 pins of the SR
      int sr_pin;
      for(sr_pin=0; sr_pin<8; ++sr_pin)
	if( changed & (1 << sr_pin) )
	  notify_hook(mod*64 + 8*row + sr_pin, (blm_scalar_button_row_values[mod][row] & (1 << sr_pin)) ? 1 : 0);
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns value of BLM DIN pin
//! \param[in] pin number of pin
//! \return pin value (0 or 1)
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_DIN_PinGet(u32 pin)
{
  // check if pin available
  if( pin >= 8*8*BLM_SCALAR_NUM_MODULES )
    return -1;

  int mod = pin / 64;
  int row = (pin / 8) % 8;
  int dinpin = pin % 8;
  return (blm_scalar_button_row_values[mod][row] & (1 << dinpin)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns value of BLM DIN "virtual" shift register
//! \param[in] sr number of shift register
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_SCALAR_DIN_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= 8*BLM_SCALAR_NUM_MODULES )
    return -1;

  int mod = sr / 8;
  int row = sr % 8;
  return blm_scalar_button_row_values[mod][row];
}


/////////////////////////////////////////////////////////////////////////////
//! Sets red/green/blue LED to 0 or Vss
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] pin the pin number
//! \param[in] value the pin value
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_DOUT_PinSet(u32 colour, u32 pin, u32 value)
{
  // check if pin available
  if( pin >= 8*8*BLM_SCALAR_NUM_MODULES )
    return -1;

  // check if colour available
  if( colour >= BLM_SCALAR_NUM_COLOURS )
    return -2;

  int mod = pin / 64;
  int row = (pin / 8) % 8;
  int doutpin = pin % 8;

  MIOS32_IRQ_Disable(); // should be atomic
  if( value )
    blm_scalar_led[mod][row][colour] |= (u8)(1 << doutpin);
  else
    blm_scalar_led[mod][row][colour] &= ~(u8)(1 << doutpin);
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns red/green/blue LED status
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] pin the pin number
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_DOUT_PinGet(u32 colour, u32 pin)
{
  // check if pin available
  if( pin >= 8*8*BLM_SCALAR_NUM_MODULES )
    return -1;

  // check if colour available
  if( colour >= BLM_SCALAR_NUM_COLOURS )
    return -2;

  int mod = pin / 64;
  int row = (pin / 8) % 8;
  int doutpin = pin % 8;

  return (blm_scalar_led[mod][row][colour] & (1 << doutpin)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets red or green "virtual" shift register
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] sr the shift register number
//! \param[in] value the shift register value
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_DOUT_SRSet(u32 colour, u32 sr, u8 value)
{
  // check if SR available
  if( sr >= 8*BLM_SCALAR_NUM_MODULES )
    return -1;

  // check if colour available
  if( colour >= BLM_SCALAR_NUM_COLOURS )
    return -2;

  int mod = sr / 8;
  int row = sr % 8;

  blm_scalar_led[mod][row][colour] = value;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns content of red or green "virtual" shift register
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] sr the shift register number
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_SCALAR_DOUT_SRGet(u32 colour, u32 sr)
{
  // check if SR available
  if( sr >= 8*BLM_SCALAR_NUM_MODULES )
    return -1;

  // check if colour available
  if( colour >= BLM_SCALAR_NUM_COLOURS )
    return -2;

  int mod = sr / 8;
  int row = sr % 8;

  return blm_scalar_led[mod][row][colour];
}

//! \}
