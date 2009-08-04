// $Id$
//! \defgroup BLM
//!
//! Button/Single/Duo/Triple (RGB)-LED driver
//!
//! \{
/* ==========================================================================
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

#include "blm.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access (bypasses BLM_DOUT_SR* functions)
u8 blm_led_row[BLM_NUM_COLOURS][BLM_NUM_ROWS];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static blm_config_t blm_config;

static u8 blm_selected_column;

static u8 blm_button_row[BLM_NUM_ROWS];
static u8 blm_button_row_changed[BLM_NUM_ROWS];

#if BLM_DEBOUNCE_MODE == 1
static u8 blm_button_debounce_ctr; // cheapo
#elif BLM_DEBOUNCE_MODE == 2
static u8 blm_button_debounce_ctr[8*BLM_NUM_ROWS]; // expensive
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes the button LED matrix
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_Init(u32 mode)
{
  u8 i, j;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // take over configuration from default settings
  blm_config.dout_l1_sr = BLM_DOUT_L1_SR;
  blm_config.dout_r1_sr = BLM_DOUT_R1_SR;
  blm_config.dout_cathodes_sr1 = BLM_DOUT_CATHODES_SR1;
  blm_config.dout_cathodes_sr2 = BLM_DOUT_CATHODES_SR2;
  blm_config.cathodes_inv_mask = BLM_CATHODES_INV_MASK;
  blm_config.dout_l2_sr = BLM_DOUT_L2_SR;
  blm_config.dout_r2_sr = BLM_DOUT_R2_SR;
  blm_config.dout_l3_sr = BLM_DOUT_L3_SR;
  blm_config.dout_r3_sr = BLM_DOUT_R3_SR;
  blm_config.din_l_sr = BLM_DIN_L_SR;
  blm_config.din_r_sr = BLM_DIN_R_SR;
  blm_config.debounce_delay = 0;


  // set button value to initial value (1) and clear "changed" status of all 64 buttons
  // clear all LEDs
  for(i=0; i<BLM_NUM_ROWS; ++i) {
    blm_button_row[i] = 0xff;
    blm_button_row_changed[i] = 0x00;

    for(j=0; j<BLM_NUM_COLOURS; ++j)
      blm_led_row[j][i] = 0x00;
  }

  // clear debounce counter(s)
#if BLM_DEBOUNCE_MODE == 1
  blm_button_debounce_ctr = 0;
#elif BLM_DEBOUNCE_MODE == 2
  for(i=0; i<8*BLM_NUM_ROWS; ++i)
    blm_button_debounce_ctr[i] = 0;
#endif

  // select first column
  if( blm_config.dout_cathodes_sr1 )
    MIOS32_DOUT_SRSet(blm_config.dout_cathodes_sr1 - 1, (1 << 4) | (1 << 0));

  if( blm_config.dout_cathodes_sr2 )
    MIOS32_DOUT_SRSet(blm_config.dout_cathodes_sr2 - 1, (1 << 4) | (1 << 0));

  // remember that this column has been selected
  blm_selected_column = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Configures the BLM driver.
//!
//! See blm.h for more informations
//!
//! \param[in] config a structure with following members:
//! <UL>
//!   <LI>config.dout_l1_sr (LED anodes, left side)
//!   <LI>config.dout_r1_sr (LED anodes, right side)
//!   <LI>config.dout_cathodes_sr1 (LED cathodes and button selection lines (2 * 4 bit))
//!   <LI>config.dout_cathodes_sr2 (optional second SR which outputs the same values)
//!   <LI>config.cathodes_inv_mask (inversion mask for cathodes)
//!   <LI>config.dout_l2_sr (for 2-colour LED option)
//!   <LI>config.dout_r2_sr (for 2-colour LED option)
//!   <LI>config.dout_l3_sr (for 3-colour LED option)
//!   <LI>config.dout_r3_sr (for 3-colour LED option)
//!   <LI>config.debounce_delay (used if debounce mode >= 1)
//! </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_ConfigSet(blm_config_t config)
{
  // ensure that update is atomic
  MIOS32_IRQ_Disable();
  blm_config = config;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns current BLM configuration
//! \return config structure see BLM_ConfigSet()
/////////////////////////////////////////////////////////////////////////////
blm_config_t BLM_ConfigGet(void)
{
  return blm_config;
}


/////////////////////////////////////////////////////////////////////////////
//! This function prepares the DOUT register to drive a column.<BR>
//! It should be called from the APP_SRIO_ServicePrepare()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_PrepareCol(void)
{
  u8 dout_value;
  u8 row_offset;

  // increment column, wrap at 4
  if( ++blm_selected_column >= 4 )
    blm_selected_column = 0;

  // select next DOUT/DIN column (selected cathode line = 0, all others 1)
  dout_value = (1 << blm_selected_column) ^ 0x0f;

  // duplicate the 4 selection lines for button matrix
  dout_value |= (dout_value << 4);

  // finally apply inversion mask (required when sink drivers are connected to the cathode lines)
  dout_value ^= BLM_CATHODES_INV_MASK;

  // output on CATHODES* registers
  if( blm_config.dout_cathodes_sr1 )
    MIOS32_DOUT_SRSet(blm_config.dout_cathodes_sr1 - 1, dout_value);
  if( blm_config.dout_cathodes_sr2 )
    MIOS32_DOUT_SRSet(blm_config.dout_cathodes_sr2 - 1, dout_value);

  // determine row offset (0..7) depending on selected column
  row_offset = blm_selected_column << 1;

  // output value of LED rows depending on current column
#if BLM_NUM_COLOURS >= 1
  if( blm_config.dout_l1_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_l1_sr - 1, blm_led_row[0][row_offset+0]);
  if( blm_config.dout_r1_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_r1_sr - 1, blm_led_row[0][row_offset+1]);
#endif

#if BLM_NUM_COLOURS >= 2
  if( blm_config.dout_l2_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_l2_sr - 1, blm_led_row[1][row_offset+0]);
  if( blm_config.dout_r2_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_r2_sr - 1, blm_led_row[1][row_offset+1]);
#endif

#if BLM_NUM_COLOURS >= 3
  if( blm_config.dout_l3_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_l3_sr - 1, blm_led_row[2][row_offset+0]);
  if( blm_config.dout_r3_sr )
    MIOS32_DOUT_SRSet(blm_config.dout_r3_sr - 1, blm_led_row[2][row_offset+1]);
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function gets the DIN values of the selected column.<BR>
//! It should be called from the APP_SRIO_ServiceFinish() hook
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_GetRow(void)
{
  u8 sr, sr_value;
  u8 side;

#if BLM_DEBOUNCE_MODE == 1
  // decrememt debounce counter if > 0
  if( blm_button_debounce_ctr )
    --blm_button_debounce_ctr;
#endif

  // check left/right half
  for(side=0; side<2; ++side) {
    // since the row line of the buttons is identical to the row line of the LEDs,
    // we can derive the button row offset from blm_selected_column
    sr = (((blm_selected_column-1)&0x3) << 1) + side;

    sr_value = 0xff; // if no DIN register selected
    if( side == 0 ) {
      if( blm_config.din_l_sr ) {
	MIOS32_DIN_SRChangedGetAndClear(blm_config.din_l_sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
	sr_value = MIOS32_DIN_SRGet(blm_config.din_l_sr-1);
      }
    } else { // side == 1
      if( blm_config.din_r_sr ) {
	MIOS32_DIN_SRChangedGetAndClear(blm_config.din_r_sr-1, 0xff); // ensure that change won't be propagated to normal DIN handler
	sr_value = MIOS32_DIN_SRGet(blm_config.din_r_sr-1);
      }
    }

#if BLM_DEBOUNCE_MODE == 2
    // expensive debounce handling: each pin has to be handled individually
    for(sr_pin=0; sr_pin<8; ++sr_pin) {
      u8 mask = 1 << sr_pin;
      u32 pin = (sr<<3) | sr_pin;

      // decrememt debounce counter if > 0
      if( blm_button_debounce_ctr[pin]) {
	--blm_button_debounce_ctr[pin];
      } else {
	// if debouncing counter == 0
	// check if pin has been toggled
	if( (sr_value ^ blm_button_row[sr]) & mask ) {

	  // set/clear pin flag
	  if( sr_value & mask )
	    blm_button_row[sr] |= mask;
	  else
	    blm_button_row[sr] &= ~mask;

	  // notify change for lower priority task
	  blm_button_row_changed[sr] |= mask;

	  // reload debounce counter
	  blm_button_debounce_ctr[pin] = blm_config.debounce_delay;
	}
      }
    }
#else
    // cheap (or none) debounce handling

    // ignore so long debounce counter != 0
#if BLM_DEBOUNCE_MODE == 1
    if( !blm_button_debounce_ctr ) {
#else
    if( 1 ) {
#endif
      // determine pin changes
      u8 changed = sr_value ^ blm_button_row[sr];

      // add them to existing notifications
      blm_button_row_changed[sr] |= changed;

      // store new value
      blm_button_row[sr] = sr_value;

#if BLM_DEBOUNCE_MODE == 1
      // reload debounce counter if any pin has changed
      if( changed )
	blm_button_debounce_ctr = blm_config.debounce_delay;
#endif
    }

#endif
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
s32 BLM_ButtonHandler(void *_notify_hook)
{
  s32 sr;
  s32 sr_pin;
  u8 changed;
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  // check all shift registers for DIN pin changes
  for(sr=0; sr<BLM_NUM_ROWS; ++sr) {
    
    // check if there are pin changes - must be atomic!
    MIOS32_IRQ_Disable();
    changed = blm_button_row_changed[sr];
    blm_button_row_changed[sr] = 0;
    MIOS32_IRQ_Enable();

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    for(sr_pin=0; sr_pin<8; ++sr_pin)
      if( changed & (1 << sr_pin) )
        notify_hook(8*sr+sr_pin, (blm_button_row[sr] & (1 << sr_pin)) ? 1 : 0);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns value of BLM DIN pin
//! \param[in] pin number of pin
//! \return pin value (0 or 1)
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DIN_PinGet(u32 pin)
{
  // check if pin available
  if( pin >= 8*BLM_NUM_ROWS )
    return -1;

  return (blm_button_row[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns value of BLM DIN "virtual" shift register
//! \param[in] sr number of shift register
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_DIN_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= BLM_NUM_ROWS )
    return -1;

  return blm_button_row[sr];
}


/////////////////////////////////////////////////////////////////////////////
//! Sets red/green/blue LED to 0 or Vss
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] pin the pin number
//! \param[in] value the pin value
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DOUT_PinSet(u32 colour, u32 pin, u32 value)
{
  // check if pin available
  if( pin >= 8*BLM_NUM_ROWS )
    return -1;

  // check if colour available
  if( colour >= BLM_NUM_COLOURS )
    return -2;

  MIOS32_IRQ_Disable(); // should be atomic
  if( value )
    blm_led_row[colour][pin >> 3] |= (u8)(1 << (pin&7));
  else
    blm_led_row[colour][pin >> 3] &= ~(u8)(1 << (pin&7));
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns red/green/blue LED status
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] pin the pin number
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DOUT_PinGet(u32 colour, u32 pin)
{
  // check if pin available
  if( pin >= 8*BLM_NUM_ROWS )
    return -1;

  // check if colour available
  if( colour >= BLM_NUM_COLOURS )
    return -2;

  return (blm_led_row[colour][pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets red or green "virtual" shift register
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] sr the shift register number
//! \param[in] value the shift register value
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DOUT_SRSet(u32 colour, u32 sr, u8 value)
{
  // check if SR available
  if( sr >= BLM_NUM_ROWS )
    return -1;

  // check if colour available
  if( colour >= BLM_NUM_COLOURS )
    return -2;

  blm_led_row[colour][sr] = value;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns content of red or green "virtual" shift register
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] sr the shift register number
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_DOUT_SRGet(u32 colour, u32 sr)
{
  // check if SR available
  if( sr >= BLM_NUM_ROWS )
    return -1;

  // check if colour available
  if( colour >= BLM_NUM_COLOURS )
    return -2;

  return blm_led_row[colour][sr];
}

//! \}
