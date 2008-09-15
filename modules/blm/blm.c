// $Id$
/*
 * Button/Single/Duo/Triple (RGB)-LED driver
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

#include "blm.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u8 blm_selected_column;

u8 blm_button_row[BLM_NUM_ROWS];
u8 blm_button_row_changed[BLM_NUM_ROWS];

u8 blm_led_row[BLM_NUM_COLOURS][BLM_NUM_ROWS];

u8 blm_button_debounce_delay;

#if BLM_DEBOUNCE_MODE == 1
u8 blm_button_debounce_ctr; // cheapo
#elif BLM_DEBOUNCE_MODE == 2
u8 blm_button_debounce_ctr[8*BLM_NUM_ROWS]; // expensive
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes SPI pins and peripheral
// IN: <mode>: currently only mode 0 supported
//             later we could provide different pin mappings and operation 
//             modes (e.g. output only)
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_Init(u32 mode)
{
  u8 i, j;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

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
    blm_button_debounce_ctr[i];
#endif

  // select first column
#if BLM_DOUT_CATHODES1
  MIOS32_DOUT_SRSet(BLM_DOUT_CATHODES1 - 1, (1 << 4) | (1 << 0));
#endif

#if BLM_DOUT_CATHODES2
  MIOS32_DOUT_SRSet(BLM_DOUT_CATHODES2 - 1, (1 << 4) | (1 << 0));
#endif

  // remember that this column has been selected
  blm_selected_column = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a column
// It should be called before MIOS32_SRIO_ScanStart() is executed to capture
// the read DIN values
// IN: -
// OUT: returns -1 on errors
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
#if BLM_DOUT_CATHODES1
  MIOS32_DOUT_SRSet(BLM_DOUT_CATHODES1 - 1, dout_value);
#endif
#if BLM_DOUT_CATHODES2
  MIOS32_DOUT_SRSet(BLM_DOUT_CATHODES2 - 1, dout_value);
#endif

  // determine row offset (0..7) depending on selected column
  row_offset = blm_selected_column << 1;

  // output value of LED rows depending on current column
#if BLM_NUM_COLOURS >= 1
#if BLM_DOUT_L1
  MIOS32_DOUT_SRSet(BLM_DOUT_L1 - 1, blm_led_row[0][row_offset+0]);
#endif
#if BLM_DOUT_R1
  MIOS32_DOUT_SRSet(BLM_DOUT_R1 - 1, blm_led_row[0][row_offset+1]);
#endif
#endif

#if BLM_NUM_COLOURS >= 2
#if BLM_DOUT_L2
  MIOS32_DOUT_SRSet(BLM_DOUT_L2 - 1, blm_led_row[1][row_offset+0]);
#endif
#if BLM_DOUT_R2
  MIOS32_DOUT_SRSet(BLM_DOUT_R2 - 1, blm_led_row[1][row_offset+1]);
#endif
#endif

#if BLM_NUM_COLOURS >= 3
#if BLM_DOUT_L3
  MIOS32_DOUT_SRSet(BLM_DOUT_L3 - 1, blm_led_row[2][row_offset+0]);
#endif
#if BLM_DOUT_R3
  MIOS32_DOUT_SRSet(BLM_DOUT_R3 - 1, blm_led_row[2][row_offset+1]);
#endif
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected column
// It should be called from the SRIO scan finish notification hook
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_GetRow(void)
{
  u8 sr, sr_pin, sr_value;
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
#if BLM_DIN_L
      MIOS32_DIN_SRChangedGetAndClear(BLM_DIN_L-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value = MIOS32_DIN_SRGet(BLM_DIN_L-1);
#endif
    } else { // side == 1
#if BLM_DIN_R
      MIOS32_DIN_SRChangedGetAndClear(BLM_DIN_R-1, 0xff); // ensure that change won't be propagated to normal DIN handler
      sr_value = MIOS32_DIN_SRGet(BLM_DIN_R-1);
#endif
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
	  blm_button_debounce_ctr[pin] = blm_button_debounce_delay;
	}
      }
    }
#else
    // cheap (or none) debounce handling

    // ignore so long debounce counter != 0
    if( !blm_button_debounce_ctr ) {
      // determine pin changes
      u8 changed = sr_value ^ blm_button_row[sr];

      // add them to existing notifications
      blm_button_row_changed[sr] |= changed;

      // store new value
      blm_button_row[sr] = sr_value;

#if BLM_DEBOUNCE_MODE == 1
      // reload debounce counter if any pin has changed
      if( changed )
	blm_button_debounce_ctr = blm_button_debounce_delay;
#endif
    }

#endif
  }

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
s32 BLM_ButtonHandler(void *_notify_hook)
{
  s32 sr;
  s32 num_sr;
  s32 sr_pin;
  u8 changed;
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  // check all shift registers for DIN pin changes
  for(sr=0; sr<BLM_NUM_ROWS; ++sr) {
    
    // check if there are pin changes - must be atomic!
    portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
    changed = blm_button_row_changed[sr];
    blm_button_row_changed[sr] = 0;
    portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

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
// returns value of BLM DIN pin
// IN: pin number
// OUT: pin value (0 or 1), returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DIN_PinGet(u32 pin)
{
  // check if pin available
  if( pin >= 8*BLM_NUM_ROWS )
    return -1;

  return (blm_button_row[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns value of BLM DIN "virtual" shift register
// IN: colour selection (0/1) in <colour>, pin number in <pin>, pin value in <value>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_DIN_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= BLM_NUM_ROWS )
    return -1;

  return blm_button_row[sr];
}


/////////////////////////////////////////////////////////////////////////////
// sets red/green/blue LED to 0 or Vss
// IN: colour selection (0/1/2) in <colour>, pin number in <pin>, pin value in <value>
// OUT: returns < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DOUT_PinSet(u32 colour, u32 pin, u32 value)
{
  // check if pin available
  if( pin >= 8*BLM_NUM_ROWS )
    return -1;

  // check if colour available
  if( colour >= BLM_NUM_COLOURS )
    return -2;

  if( value )
    blm_led_row[colour][pin >> 3] |= (u8)(1 << (pin&7));
  else
    blm_led_row[colour][pin >> 3] &= ~(u8)(1 << (pin&7));

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns red/green/blue LED status
// IN: colour selection (0/1/2) in <colour>, pin number in <pin>
// OUT: returns pin value or < 0 if pin not available
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
// sets red or green "virtual" shift register
// IN: colour selection (0/1/2) in <colour>, SR number in <sr>, SR value in <value>
// OUT: returns < 0 if SR not available
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
// returns content of red or green "virtual" shift register
// IN: colour selection (0/1/2) in <colour>, SR number in <sr>
// OUT: returns SR value or < 0 if SR not available
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


/////////////////////////////////////////////////////////////////////////////
// sets the debounce delay
// IN: debounce delay (8bit value)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_DebounceDelaySet(u8 delay)
{
  blm_button_debounce_delay = delay;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns the debounce delay
// IN: -
// OUT: debounce delay (8bit value)
/////////////////////////////////////////////////////////////////////////////
u8 BLM_DebounceDelayGet(void)
{
  return blm_button_debounce_delay;
}

