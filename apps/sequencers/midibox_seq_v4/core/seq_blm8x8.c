// $Id$
/*
 * Button/LED Matrix Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2017 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "seq_blm8x8.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

u8 seq_blm8x8_led_row[SEQ_BLM8X8_NUM][8];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 seq_blm8x8_button_row[SEQ_BLM8X8_NUM][8];
static u8 seq_blm8x8_button_row_changed[SEQ_BLM8X8_NUM][8];

static u8 seq_blm8x8_current_row;
static u8 seq_blm8x8_debounce_ctr;

static seq_blm8x8_config_t seq_blm8x8_config[SEQ_BLM8X8_NUM];

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_Init(u32 mode)
{
  int blm;

  seq_blm8x8_config_t *config = (seq_blm8x8_config_t *)&seq_blm8x8_config[0];
  for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm, ++config) {
    int i;

    for(i=0; i<8; ++i) {
      seq_blm8x8_led_row[blm][i] = 0x00;
      seq_blm8x8_button_row[blm][i] = 0xff;
      seq_blm8x8_button_row_changed[blm][i] = 0x00;
    }

    config->rowsel_dout_sr = 0;
    config->led_dout_sr = 0;
    config->button_din_sr = 0;
    config->rowsel_inv_mask = 0x00;
    config->col_inv_mask = 0x00;
    config->debounce_delay = 0;
  }

  seq_blm8x8_current_row = 0;
  seq_blm8x8_debounce_ctr = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a row
// It should be called from the APP_SRIO_ServicePrepare
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_PrepareRow(void)
{
  // increment current row, wrap at 8
  if( ++seq_blm8x8_current_row >= 8 )
    seq_blm8x8_current_row = 0;

  u8 rowsel_value = ~(1 << seq_blm8x8_current_row);

  int blm;
  seq_blm8x8_config_t *config = (seq_blm8x8_config_t *)&seq_blm8x8_config[0];
  for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm, ++config) {
    // cathode
    if( config->rowsel_dout_sr ) {
      MIOS32_DOUT_SRSet(config->rowsel_dout_sr - 1, rowsel_value ^ config->rowsel_inv_mask);
    }

    // anode
    if( config->led_dout_sr ) {
      MIOS32_DOUT_SRSet(config->led_dout_sr - 1, seq_blm8x8_led_row[blm][seq_blm8x8_current_row] ^ config->col_inv_mask);
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected row
// It should be called from the APP_SRIO_ServiceFinish hook
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_GetRow(void)
{
  u8 scanned_row = seq_blm8x8_current_row ? (seq_blm8x8_current_row - 1) : (8 - 1);

  int blm;
  seq_blm8x8_config_t *config = (seq_blm8x8_config_t *)&seq_blm8x8_config[0];

  for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm, ++config) {

    if( config->button_din_sr ) {
      u8 sr = config->button_din_sr - 1;

      // ensure that change won't be propagated to normal DIN handler
      MIOS32_DIN_SRChangedGetAndClear(sr, 0xff);

      // cheap debounce handling. ignore any changes if debounce_ctr > 0
      if( !seq_blm8x8_debounce_ctr ) {
	u8 sr_value = MIOS32_DIN_SRGet(sr);

	// *** set change notification and new value. should not be interrupted ***
	MIOS32_IRQ_Disable();

	// if a second change happens before the last change was notified (clear
	// changed flags), the change flag will be unset (two changes -> original value)
	if( seq_blm8x8_button_row_changed[blm][scanned_row] ^= (sr_value ^ seq_blm8x8_button_row[blm][scanned_row]) ) {
	  seq_blm8x8_debounce_ctr = config->debounce_delay; // restart debounce delay
	}

	// copy new values to seq_led_matrix_button_rows
	seq_blm8x8_button_row[blm][scanned_row] = sr_value;
	MIOS32_IRQ_Enable();
	// *** end atomic block ***
      } else {
	--seq_blm8x8_debounce_ctr; // decrement debounce control
      }
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from a task to check for button changes
// periodically. Events (change from 0->1 or from 1->0) will be notified
// via the given callback function <notify_hook> with following parameters:
//   <notifcation-hook>(u32 btn, u32 value)
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_ButtonHandler(void *_notify_hook)
{
  void (*notify_hook)(u8 blm, u32 pin, u32 value) = _notify_hook;

  // no hook?
  if( _notify_hook == NULL )
    return -2;

  int blm;
  for(blm=0; blm<SEQ_BLM8X8_NUM; ++blm) {
    int row;

    u8 *button_row = (u8 *)&seq_blm8x8_button_row[blm][0];
    u8 *button_row_changed = (u8 *)&seq_blm8x8_button_row_changed[blm][0];

    for(row=0; row<8; ++row, ++button_row, ++button_row_changed) {
      // *** fetch changed / values, reset changed. should not be interrupted ***
      MIOS32_IRQ_Disable();
      u8 changed = *button_row_changed;
      u8 values = *button_row;
      *button_row_changed = 0;
      MIOS32_IRQ_Enable();
      // *** end atomic block ***

      // check if any changes in this row
      if( !changed )
	continue;

      // ..walk pins and notify changes
      int pin;
      u8 pin_mask = 0x01;
      for(pin=0; pin < 8; ++pin, pin_mask <<= 1) {
        if( changed & pin_mask )
          notify_hook(blm, row*8 + pin, (values & pin_mask) ? 1 : 0);
      }
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns a button's state
// IN: button number
// OUT: button value (0 or 1), returns < 0 if button not available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_ButtonGet(u8 blm, u32 button)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( button >= 64 ) 
    return -2;

  return (seq_blm8x8_button_row[blm][button/8] & (1 << (button % 8))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns the buttons serial-register value for a row / SR
// IN: button row in <row>, row-SR in <sr>
// OUT: serial register value (0 if register not available)
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_BLM8X8_ButtonSRGet(u8 blm, u8 row)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( row >= 8 ) 
    return -2;

  return seq_blm8x8_button_row[blm][row];
}


/////////////////////////////////////////////////////////////////////////////
// sets a LED's value
// IN: LED number in <led>, LED value in <value>
// OUT: returns < 0 if LED not available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_LEDSet(u8 blm, u8 led, u8 value)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( led >= 64 ) 
    return -2;

  if( value ) {
    seq_blm8x8_led_row[blm][led/8] |= (1 << (led % 8));
  } else {
    seq_blm8x8_led_row[blm][led/8] &= ~(1 << (led % 8));
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// returns the status of a LED
// IN: LED number in <led>
// OUT: returns LED state value or < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_LEDGet(u8 blm, u8 led)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( led >= 64 ) 
    return -2;

  return (seq_blm8x8_led_row[blm][led/8] & (1 << (led % 8))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns the LED serial-register value for a row / SR
// IN: LED row in <row>, row-SR in <sr>
// OUT: serial register value (0 if register not available)
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_BLM8X8_LEDSRGet(u8 blm, u8 row)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( row >= 8 ) 
    return -2;

  return seq_blm8x8_led_row[blm][row];
}


/////////////////////////////////////////////////////////////////////////////
// sets the LED serial-register value for a row / sr
// IN: LED row in <row>, row-SR in <sr>
// OUT: < 0 on error (SR not available), 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_LEDSRSet(u8 blm, u8 row, u8 sr_value)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  if( row >= 8 ) 
    return -2;

  seq_blm8x8_led_row[blm][row] = sr_value;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// sets the seq_blm8x8 soft configurations
// IN: config, struct with members:
//    .rowsel_dout_sr
//    .led_dout_sr
//    .button_din_sr
//    .rowsel_inv_mask
//    .col_inv_mask
//    .debounce_delay
// OUT: returns < 0 on error, 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_BLM8X8_ConfigSet(u8 blm, seq_blm8x8_config_t config)
{
  if( blm >= SEQ_BLM8X8_NUM )
    return -1;

  seq_blm8x8_config[blm] = config;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// gets the seq_blm8x8 soft configurations
// IN: -
// OUT: struct with members:
//    .rowsel_dout_sr
//    .led_dout_sr
//    .button_din_sr
//    .rowsel_inv_mask
//    .col_inv_mask
//    .debounce_delay
/////////////////////////////////////////////////////////////////////////////
seq_blm8x8_config_t SEQ_BLM8X8_ConfigGet(u8 blm)
{
  if( blm >= SEQ_BLM8X8_NUM )
    blm = 0;

  return seq_blm8x8_config[blm];
}
