// $Id$
//! \defgroup BLM_CHEAPO
//!
//! BLM_CHEAPO Driver
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "blm_cheapo.h"


/////////////////////////////////////////////////////////////////////////////
// local defines
/////////////////////////////////////////////////////////////////////////////

#define NUM_ROWS 8


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_row;

static u8 button_row[NUM_ROWS];
static u8 button_row_changed[NUM_ROWS];
static u8 button_debounce_ctr;
static u8 button_debounce_reload;

static u8 led_row[NUM_ROWS];


/////////////////////////////////////////////////////////////////////////////
//! Initializes the BLM_CHEAPO driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize all pins of J5A and J5B as outputs in Push-Pull Mode
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);

  int row;
  for(row=0; row<NUM_ROWS; ++row) {
    button_row[row] = 0xff;
    button_row_changed[row] = 0x00;
  }

  selected_row = 0;
  button_debounce_ctr = 0;
  button_debounce_reload = 10; // mS

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function services the row selection lines at J15 and the 
//! LED output lines at J5A/B
//! It should be called from the APP_SRIO_ServicePrepare()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_PrepareCol(void)
{
  if( ++selected_row >= 8 )
    selected_row = 0;

  // selection lines at J15
  u8 selection_mask = (1 << selected_row);
  MIOS32_BOARD_J15_DataSet(selection_mask);

  // LED columns at J5A/B
  MIOS32_BOARD_J5_Set(led_row[selected_row]);

  return 0; // no error
}


////////////////////////////////////////////////////////////////////////////////////////////////
//! This function gets the button values of the selected row.
//! It should be called from the APP_SRIO_ServiceFinish() hook
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_GetRow(void)
{
#if 0
  // determine the scanned row (selected_row driver has already been incremented, decrement it again)
  u8 row = (selected_row-1) & 0x7;
#else
  // no shift registers, value immediately available
  u8 row = selected_row;
#endif

  // decrememt debounce counter if > 0
  if( button_debounce_ctr )
    --button_debounce_ctr;

  u8 sr_value = MIOS32_BOARD_J10_Get();

  // determine pin changes
  u8 changed = sr_value ^ button_row[row];

  // ignore as long as debounce counter running
  if( !button_debounce_ctr ) {

    // add them to existing notifications
    button_row_changed[row] |= changed;

    // store new value
    button_row[row] = sr_value;

    // reload debounce counter if any pin has changed
    if( changed )
      button_debounce_ctr = button_debounce_reload;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from a task to check for button changes
//! periodically. Events (change from 0->1 or from 1->0) will be notified 
//! via the given callback function <notify_hook> with following parameters:
//!   <notifcation-hook>(s32 pin, s32 value)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_ButtonHandler(void *_notify_hook)
{
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  int row;
  for(row=0; row<NUM_ROWS; ++row) {
    
    // check if there are pin changes - must be atomic!
    MIOS32_IRQ_Disable();
    u8 changed = button_row_changed[row];
    button_row_changed[row] = 0;
    MIOS32_IRQ_Enable();

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    int pin;
    u8 mask = (1 << 0);
    for(pin=0; pin<8; ++pin, mask <<= 1)
      if( changed & mask ) {
	u8 pin_value = (button_row[row] & (1 << pin)) ? 1 : 0;
#if BLM_CHEAPO_RIGHT_HALF_REVERSED
	// reverse pins for right half (layout measure)
	u8 blm_pin = pin;
	if( row >= 4 ) {
	  blm_pin = 7 - pin;
	}
#endif

#if 0
	MIOS32_MIDI_SendDebugMessage("[BLM_CHEAPO] Row:%d  Pin:%d  Value:%d\n", row, blm_pin, pin_value);
#endif
	notify_hook(8*row + blm_pin, pin_value);
      }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sets LED to 0 or Vss
//! \param[in] pin the pin number
//! \param[in] value the pin value
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_DOUT_PinSet(u32 pin, u32 value)
{
  int row = pin / 8;
  int blm_pin = pin % 8;

  if( row >= NUM_ROWS )
    return -1;

#if BLM_CHEAPO_RIGHT_HALF_REVERSED
  // reverse pins for right half (layout measure)
  if( row >= 4 ) {
    blm_pin = 7 - blm_pin;
  }
#endif

  MIOS32_IRQ_Disable(); // should be atomic
  if( value )
    led_row[row] |= (u8)(1 << blm_pin);
  else
    led_row[row] &= ~(u8)(1 << blm_pin);
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns LED status
//! \param[in] pin the pin number
//! \return < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_DOUT_PinGet(u32 pin)
{
  int row = pin / 8;
  int blm_pin = pin % 8;

  if( row >= NUM_ROWS )
    return -1;

#if BLM_CHEAPO_RIGHT_HALF_REVERSED
  // reverse pins for right half (layout measure)
  if( row >= 4 ) {
    blm_pin = 7 - blm_pin;
  }
#endif

  return (led_row[row] & (1 << blm_pin)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets 8 LEDs of a row at once
//! \param[in] sr the row number
//! \param[in] value state of 8 LEDs
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_CHEAPO_DOUT_SRSet(u32 row, u8 value)
{
  if( row >= NUM_ROWS )
    return -1;

#if BLM_CHEAPO_RIGHT_HALF_REVERSED
  if( row > 4 ) {
    led_row[row] = mios32_dout_reverse_tab[value];
    return 0;
  }
#endif

  led_row[row] = value;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns content of red or green "virtual" shift register
//! \param[in] colour the colour selection (0/1/2)
//! \param[in] sr the shift register number
//! \return < 0 if SR not available
/////////////////////////////////////////////////////////////////////////////
u8 BLM_CHEAPO_DOUT_SRGet(u32 row)
{
  if( row >= NUM_ROWS )
    return -1;

#if BLM_CHEAPO_RIGHT_HALF_REVERSED
  if( row > 4 ) {
    return mios32_dout_reverse_tab[led_row[row]];
  }
#endif

  return led_row[row];
}

//! \}
