// $Id$
/*
 * Simple 8x8 Button/LED driver
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

#include <FreeRTOS.h>
#include <portmacro.h>

#include "blm8x8.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access (bypasses BLM8X8_DOUT_SR* functions)
u8 blm8x8_led_row[BLM8X8_NUM_ROWS];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 blm8x8_selected_column;

static u8 blm8x8_button_row[BLM8X8_NUM_ROWS];
static u8 blm8x8_button_row_changed[BLM8X8_NUM_ROWS];

static u8 blm8x8_button_debounce_delay;

#if BLM8X8_DEBOUNCE_MODE == 1
u8 blm8x8_button_debounce_ctr; // cheapo
#elif BLM8X8_DEBOUNCE_MODE == 2
u8 blm8x8_button_debounce_ctr[8*BLM8X8_NUM_ROWS]; // expensive
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes the 8x8 Button/LED matrix
// IN: <mode>: currently only mode 0 supported
//             later we could provide different pin mappings and operation 
//             modes (e.g. output only)
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_Init(u32 mode)
{
  u8 i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // set button value to initial value (1) and clear "changed" status of all 64 buttons
  // clear all LEDs
  for(i=0; i<BLM8X8_NUM_ROWS; ++i) {
    blm8x8_button_row[i] = 0xff;
    blm8x8_button_row_changed[i] = 0x00;
    blm8x8_led_row[i] = 0x00;
  }

  // clear debounce counter(s)
#if BLM8X8_DEBOUNCE_MODE == 1
  blm8x8_button_debounce_ctr = 0;
#elif BLM8X8_DEBOUNCE_MODE == 2
  for(i=0; i<8*BLM8X8_NUM_ROWS; ++i)
    blm8x8_button_debounce_ctr[i];
#endif

  // select first column
#if BLM8X8_DOUT_CATHODES
  MIOS32_DOUT_SRSet(BLM8X8_DOUT_CATHODES - 1, ~(1 << 0));
#endif

  // remember that this column has been selected
  blm8x8_selected_column = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a column
// It should be called from the APP_SRIO_ServicePrepare
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_PrepareCol(void)
{
  u8 dout_value;

  // increment column, wrap at 8
  if( ++blm8x8_selected_column >= 8 )
    blm8x8_selected_column = 0;

  // select next DOUT/DIN column (selected cathode line = 0, all others 1)
  dout_value = ~(1 << blm8x8_selected_column);

  // apply inversion mask (required when sink drivers are connected to the cathode lines)
  dout_value ^= BLM8X8_CATHODES_INV_MASK;

  // output on CATHODES register
#if BLM8X8_DOUT_CATHODES
  MIOS32_DOUT_SRSet(BLM8X8_DOUT_CATHODES - 1, dout_value);
#endif

  // output value of LED rows depending on current column
#if BLM8X8_DOUT
  MIOS32_DOUT_SRSet(BLM8X8_DOUT - 1, blm8x8_led_row[blm8x8_selected_column]);
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected column
// It should be called from the APP_SRIO_ServiceFinish hook
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_GetRow(void)
{
  u8 sr, sr_value;

#if BLM8X8_DEBOUNCE_MODE == 1
  // decrememt debounce counter if > 0
  if( blm8x8_button_debounce_ctr )
    --blm8x8_button_debounce_ctr;
#endif

  // since the row line of the buttons is identical to the row line of the LEDs,
  // we can derive the button row offset from blm8x8_selected_column
  sr = (blm8x8_selected_column-1) & 0x7;

  sr_value = 0xff; // if no DIN register selected
#if BLM8X8_DIN
  MIOS32_DIN_SRChangedGetAndClear(BLM8X8_DIN-1, 0xff); // ensure that change won't be propagated to normal DIN handler
  sr_value = MIOS32_DIN_SRGet(BLM8X8_DIN-1);

#if BLM8X8_DEBOUNCE_MODE == 2
  // expensive debounce handling: each pin has to be handled individually
  for(sr_pin=0; sr_pin<8; ++sr_pin) {
    u8 mask = 1 << sr_pin;
    u32 pin = (sr<<3) | sr_pin;

    // decrememt debounce counter if > 0
    if( blm8x8_button_debounce_ctr[pin]) {
      --blm8x8_button_debounce_ctr[pin];
    } else {
      // if debouncing counter == 0
      // check if pin has been toggled
      if( (sr_value ^ blm8x8_button_row[sr]) & mask ) {

	// set/clear pin flag
	if( sr_value & mask )
	  blm8x8_button_row[sr] |= mask;
	else
	  blm8x8_button_row[sr] &= ~mask;

	// notify change for lower priority task
	blm8x8_button_row_changed[sr] |= mask;

	// reload debounce counter
	blm8x8_button_debounce_ctr[pin] = blm8x8_button_debounce_delay;
      }
    }
  }
#else
  // cheap (or none) debounce handling

  // ignore so long debounce counter != 0
#if BLM8X8_DEBOUNCE_MODE == 1
  if( !blm8x8_button_debounce_ctr ) {
#else
  if( 1 ) {
#endif
    // determine pin changes
    u8 changed = sr_value ^ blm8x8_button_row[sr];

    // add them to existing notifications
    blm8x8_button_row_changed[sr] |= changed;

    // store new value
    blm8x8_button_row[sr] = sr_value;

#if BLM8X8_DEBOUNCE_MODE == 1
    // reload debounce counter if any pin has changed
    if( changed )
      blm8x8_button_debounce_ctr = blm8x8_button_debounce_delay;
#endif
  }

#endif
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from a task to check for button changes
// periodically. Events (change from 0->1 or from 1->0) will be notified 
// via the given callback function <notify_hook> with following parameters:
//   <notifcation-hook>(s32 pin, s32 value)
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_ButtonHandler(void *_notify_hook)
{
  s32 sr;
  s32 sr_pin;
  u8 changed;
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  // check all shift registers for DIN pin changes
  for(sr=0; sr<BLM8X8_NUM_ROWS; ++sr) {
    
    // check if there are pin changes - must be atomic!
    MIOS32_IRQ_Disable();
    changed = blm8x8_button_row_changed[sr];
    blm8x8_button_row_changed[sr] = 0;
    MIOS32_IRQ_Enable();

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    for(sr_pin=0; sr_pin<8; ++sr_pin)
      if( changed & (1 << sr_pin) )
        notify_hook(8*sr+sr_pin, (blm8x8_button_row[sr] & (1 << sr_pin)) ? 1 : 0);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns value of BLM DIN pin
// IN: pin number
// OUT: pin value (0 or 1), returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_DIN_PinGet(u32 pin)
{
  // check if pin available
  if( pin >= 8*BLM8X8_NUM_ROWS )
    return -1;

  return (blm8x8_button_row[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns value of BLM DIN "virtual" shift register
// IN: SR number in <sr>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM8X8_DIN_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= BLM8X8_NUM_ROWS )
    return -1;

  return blm8x8_button_row[sr];
}


/////////////////////////////////////////////////////////////////////////////
// sets red/green/blue LED to 0 or Vss
// IN: pin number in <pin>, pin value in <value>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_DOUT_PinSet(u32 pin, u32 value)
{
  // check if pin available
  if( pin >= 8*BLM8X8_NUM_ROWS )
    return -1;

  MIOS32_IRQ_Disable(); // should be atomic
  if( value )
    blm8x8_led_row[pin >> 3] |= (u8)(1 << (pin&7));
  else
    blm8x8_led_row[pin >> 3] &= ~(u8)(1 << (pin&7));
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns red/green/blue LED status
// IN: pin number in <pin>
// OUT: returns pin value or < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_DOUT_PinGet(u32 pin)
{
  // check if pin available
  if( pin >= 8*BLM8X8_NUM_ROWS )
    return -1;

  return (blm8x8_led_row[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// sets red or green "virtual" shift register
// IN: SR number in <sr>, SR value in <value>
// OUT: returns < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_DOUT_SRSet(u32 sr, u8 value)
{
  // check if SR available
  if( sr >= BLM8X8_NUM_ROWS )
    return -1;

  blm8x8_led_row[sr] = value;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns content of red or green "virtual" shift register
// IN: SR number in <sr>
// OUT: returns SR value or < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM8X8_DOUT_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= BLM8X8_NUM_ROWS )
    return -1;

  return blm8x8_led_row[sr];
}


/////////////////////////////////////////////////////////////////////////////
// sets the debounce delay
// IN: debounce delay (8bit value)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM8X8_DebounceDelaySet(u8 delay)
{
  blm8x8_button_debounce_delay = delay;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns the debounce delay
// IN: -
// OUT: debounce delay (8bit value)
/////////////////////////////////////////////////////////////////////////////
u8 BLM8X8_DebounceDelayGet(void)
{
  return blm8x8_button_debounce_delay;
}

