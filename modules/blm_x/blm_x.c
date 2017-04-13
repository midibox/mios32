/*
 * Configurable DIN / DOUT matrix driver. 1-8 rows can be configured (cathode
 * lines. The anode lines (cols) can be configured individually for DIN / DOUT.
 * It's possible to configure multiple colors for multi-color LED's or multiple
 * LED's per matrix-crosspoint.
 *
 * The driver is inspired by Thorsten Klose's BLM-drivers, and is originally
 * developped to drive 4x4 three-color button-pads by sparkfun.com.
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "blm_x.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

#if (BLM_X_LED_FIRST_DOUT_SR > 0)
u8 BLM_X_LED_rows[BLM_X_NUM_ROWS][BLM_X_NUM_LED_SR];
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 current_row;

#if (BLM_X_BTN_FIRST_DIN_SR > 0)

static volatile u8 btn_rows[BLM_X_NUM_ROWS][BLM_X_NUM_BTN_SR];
static volatile u8 btn_rows_changed[BLM_X_NUM_ROWS][BLM_X_NUM_BTN_SR];

#if BLM_X_DEBOUNCE_MODE == 1
static u16 debounce_ctr; // for cheap debouncing
#elif BLM_X_DEBOUNCE_MODE == 2
static u8 debounce_ctr[BLM_X_NUM_ROWS][BLM_X_NUM_BTN_SR*8]; // for expensive debouncing
#endif

#endif

static blm_x_config_t blm_x_config;

/////////////////////////////////////////////////////////////////////////////
// Initializes the Button/LED matrix
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_Init(void){
  u32 r,i;
  //walk all rows for initialization of LED's / buttons
  for(r=0; r<BLM_X_NUM_ROWS; ++r) {
#if (BLM_X_BTN_FIRST_DIN_SR > 0)
    // set button value to initial value (1) and clear "changed" status of all buttons
    for(i=0; i<BLM_X_NUM_BTN_SR; ++i){
      btn_rows[r][i] = 0xff;
      btn_rows_changed[r][i] = 0x00;
    }
#if BLM_X_DEBOUNCE_MODE == 2
    // clear debounce counter for debounce-mode 2
    for(i=0; i < BLM_X_NUM_BTN_SR*8 ; ++i)
      debounce_ctr[r][i] = 0;
#endif
#endif
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
    // initialize LED-rows
    for(i=0; i<BLM_X_NUM_LED_SR; ++i)
      BLM_X_LED_rows[r][i] = 0x00;
#endif
  }
  // clear debounce counter for debounce-mode 1
#if ( (BLM_X_DEBOUNCE_MODE == 1) && (BLM_X_BTN_FIRST_DIN_SR > 0) )
  debounce_ctr = 0;
#endif
   // init config
  blm_x_config.rowsel_dout_sr = BLM_X_ROWSEL_DOUT_SR;
  blm_x_config.led_first_dout_sr = BLM_X_LED_FIRST_DOUT_SR;
  blm_x_config.btn_first_din_sr = BLM_X_BTN_FIRST_DIN_SR;
  blm_x_config.rowsel_inv_mask = BLM_X_ROWSEL_INV_MASK;
  blm_x_config.color_mode = BLM_X_COLOR_MODE;
  blm_x_config.debounce_delay = BLM_X_DEBOUNCE_DELAY;
  // init current row
  current_row = 0;
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function prepares the DOUT register to drive a row
// It should be called from the APP_SRIO_ServicePrepare
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_PrepareRow(void){
  u8 dout_value;
  u32 i;
  // increment current row, wrap at BLM_X_NUM_ROWS
  if( ++current_row >= BLM_X_NUM_ROWS )
    current_row = 0;
  // select next DOUT/DIN row (selected cathode line = 0, all others 1).
  // if less than five rows, each cathode line has a twin (second nibble).
  dout_value = ~(1 << current_row);
#if (BLM_X_NUM_ROWS < 5)
  dout_value ^= (1 << (current_row + 4));
#endif
  // apply inversion mask (required when sink drivers are connected to the cathode lines)
  dout_value ^= blm_x_config.rowsel_inv_mask;
  // output on CATHODES register
  MIOS32_DOUT_SRSet(blm_x_config.rowsel_dout_sr - 1, dout_value);
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  // output value of LED rows depending on current row
  for(i = 0;i < BLM_X_NUM_LED_SR;i++)
    MIOS32_DOUT_SRSet(blm_x_config.led_first_dout_sr - 1 + i, BLM_X_LED_rows[current_row][i]);
#endif
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function gets the DIN values of the selected row
// It should be called from the APP_SRIO_ServiceFinish hook
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_GetRow(void){
#if (BLM_X_BTN_FIRST_DIN_SR > 0)
  u8 sr_value;
  u32 sr,scanned_row;
  //since the row set in BLM_PrepareRow will only take effect after the DIN's are already scanned,
  //the button-row read here is one step back to the current row
  scanned_row = current_row ? (current_row -1) : (BLM_X_NUM_ROWS - 1);
  // ensure that change won't be propagated to normal DIN handler
  for(sr = 0; sr < BLM_X_NUM_BTN_SR; sr++)
    MIOS32_DIN_SRChangedGetAndClear(blm_x_config.btn_first_din_sr - 1 + sr, 0xff);
#if (BLM_X_DEBOUNCE_MODE < 2)
#if (BLM_X_DEBOUNCE_MODE == 1)
  // cheap debounce handling. ignore any changes if debounce_ctr > 0
  if(!debounce_ctr){
#endif
    for(sr = 0; sr < BLM_X_NUM_BTN_SR; sr++){
      sr_value = MIOS32_DIN_SRGet(blm_x_config.btn_first_din_sr - 1 + sr);
      //*** set change notification and new value. should not be interrupted ***
      MIOS32_IRQ_Disable();
      // if a second change happens before the last change was notified (clear
      // changed flags), the change flag will be unset (two changes -> original value)
      if( btn_rows_changed[scanned_row][sr] ^= (sr_value ^ btn_rows[scanned_row][sr]) ){
#if (BLM_X_DEBOUNCE_MODE == 1)
        debounce_ctr = blm_x_config.debounce_delay;//restart debounce delay
#endif
      }
      //copy new values to btn_rows
      btn_rows[scanned_row][sr] = sr_value;
      MIOS32_IRQ_Enable();
      //*** end atomic block ***
    }
#if (BLM_X_DEBOUNCE_MODE == 1)
  }
  else
    --debounce_ctr;//decrement debounce control
#endif
#endif
#if (BLM_X_DEBOUNCE_MODE == 2)
  for(sr = 0; sr < BLM_X_NUM_BTN_SR; sr++){
    sr_value = MIOS32_DIN_SRGet(blm_x_config.btn_first_din_sr - 1 + sr);
    //walk the 8 DIN pins of the SR
    u8 pin;
    for(pin = 0;pin < 8; pin++){
      //debounce-handling for individual buttons
      if(debounce_ctr[scanned_row][sr*8 + pin])
        debounce_ctr[scanned_row][sr*8 + pin]--;
      else{
        u8 pin_mask = 1 << pin;
        //*** set change notification and new value. should not be interrupted ***
        MIOS32_IRQ_Disable();
        // set change-notification-bit. if a second change happens before the last change was notified (clear
        // changed flags), the change flag will be unset (two changes -> original value)
        if( ( btn_rows_changed[scanned_row][sr] ^= (sr_value & pin_mask) ^ (btn_rows[scanned_row][sr] & pin_mask) ) & pin_mask )
          debounce_ctr[scanned_row][sr*8 + pin] = blm_x_config.debounce_delay;//restart debounce delay
        //set the new value bit
        if(sr_value & pin_mask)
          btn_rows[scanned_row][sr] |= pin_mask;//set value bit
        else
          btn_rows[scanned_row][sr] &= ~pin_mask;//clear value bit
        MIOS32_IRQ_Enable();
        //*** end atomic block ***
      }
    }
  }
#endif
  return 0;
#else
  return -1;//buttons disabled
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from a task to check for button changes
// periodically. Events (change from 0->1 or from 1->0) will be notified
// via the given callback function <notify_hook> with following parameters:
//   <notifcation-hook>(u32 btn, u32 value)
// IN: -
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_BtnHandler(void *_notify_hook){
#if (BLM_X_BTN_FIRST_DIN_SR > 0)
  u8 changed, values, pin_mask;
  u32 r,sr,pin;
  void (*notify_hook)(u32 btn, u32 value) = _notify_hook;
  // no hook?
  if( _notify_hook == NULL )
    return -2;
  // walk all rows & serial registers to check for changed pins
  for(r = 0; r < BLM_X_NUM_ROWS; r++){
    for(sr = 0; sr < BLM_X_NUM_BTN_SR; sr++){
      //*** fetch changed / values, reset changed. should not be interrupted ***
      MIOS32_IRQ_Disable();
      changed = btn_rows_changed[r][sr];
      values = btn_rows[r][sr];
      btn_rows_changed[r][sr] = 0;
      MIOS32_IRQ_Enable();
      //*** end atomic block ***
      //check if any changes in this SR..
      if(!changed) continue;
      //..walk pins and notify changes
      for(pin = 0; pin < 8; pin++){
        pin_mask = 1 << pin;
        if(changed & pin_mask)
          notify_hook(r * BLM_X_BTN_NUM_COLS + sr * 8 + pin, (values & pin_mask) ? 1 : 0 );
      }
    }
  }
  return 0;
#else
  return -1;//buttons disabled
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns a button's state
// IN: button number
// OUT: button value (0 or 1), returns < 0 if button not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_BtnGet(u32 btn){
#if (BLM_X_BTN_FIRST_DIN_SR > 0)
  u32 row,sr,pin;
  // check if button available
  if( btn >= BLM_X_NUM_ROWS * BLM_X_BTN_NUM_COLS )
    return -1;
  // compute row,SR & pin
  row = btn / BLM_X_BTN_NUM_COLS;
  sr = (pin = btn % BLM_X_BTN_NUM_COLS) / 8;
  pin %= 8;
  // return value
  return ( btn_rows[row][sr] & (1 << pin) ) ? 1 : 0;
#else
  return -1;//buttons disabled
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns the buttons serial-register value for a row / SR
// IN: button row in <row>, row-SR in <sr>
// OUT: serial register value (0 if register not available)
/////////////////////////////////////////////////////////////////////////////
u8 BLM_X_BtnSRGet(u8 row, u8 sr){
#if (BLM_X_BTN_FIRST_DIN_SR > 0)
  if (row > BLM_X_NUM_ROWS -1 || sr > BLM_X_NUM_BTN_SR - 1)
    return 0;
  return btn_rows[row][sr];
#else
  return 0;//buttons disabled
#endif
}


/////////////////////////////////////////////////////////////////////////////
// sets a LED's value
// IN: LED number in <led>, color in <color>, LED value in <value>
// OUT: returns < 0 if LED/color not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_LEDSet(u32 led, u32 color, u32 value){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  u32 row,sr,pin;
  // check if LED/color available
  if( led >= BLM_X_NUM_ROWS * BLM_X_LED_NUM_COLS || color > BLM_X_LED_NUM_COLORS )
    return -1;
  // compute row,sr,pin
  row = led / BLM_X_LED_NUM_COLS;
  if(blm_x_config.color_mode == 0)
    sr = ( pin = led % BLM_X_LED_NUM_COLS + BLM_X_LED_NUM_COLS*color ) / 8;
  else
    sr = ( pin = (led % BLM_X_LED_NUM_COLS)*BLM_X_LED_NUM_COLORS + color ) / 8;
  pin %= 8;
  // set value
  if( value )
    BLM_X_LED_rows[row][sr] |= (1 << pin);//set pin
  else
    BLM_X_LED_rows[row][sr] &= ~(1 << pin);//clear pin
  return 0;
#else
  return -1;//LED's disabled
#endif
}

/////////////////////////////////////////////////////////////////////////////
// sets all colors of a LED
// IN: LED number in <led>, color in <color_mask> (each bit represents a color, LSB = color 0)
// OUT: returns < 0 if LED/color not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_LEDColorSet(u32 led, u32 color_mask){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  u32 row,sr,pin,c;
  // check if LED available
  if( led >= BLM_X_NUM_ROWS * BLM_X_LED_NUM_COLS )
    return -1;
  // compute row,SR
  row = led / BLM_X_LED_NUM_COLS;
  for(c = 0; c < BLM_X_LED_NUM_COLORS; c++){
    //compute sr,pin
    if(blm_x_config.color_mode == 0)
      sr = ( pin = led % BLM_X_LED_NUM_COLS + BLM_X_LED_NUM_COLS*c ) / 8;
    else
      sr = ( pin = (led % BLM_X_LED_NUM_COLS)*BLM_X_LED_NUM_COLORS + c ) / 8;
    pin %= 8;
    // set value
    if( color_mask & (1 << c) )
      BLM_X_LED_rows[row][sr] |= (1 << pin);//set pin
    else
      BLM_X_LED_rows[row][sr] &= ~(1 << pin);//clear pin
  }
  return 0;
#else
  return -1;//LED's disabled
#endif
}

/////////////////////////////////////////////////////////////////////////////
// gets all colors of a LED
// IN: LED number in <led>
// OUT: color mask of the LED (bit 0: color 0; bit 1: color 1 etc.)
//      returns 0 if LED not available
/////////////////////////////////////////////////////////////////////////////
u32 BLM_X_LEDColorGet(u32 led){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  u32 row,sr,pin,c,color;
  // check if LED available
  if( led >= BLM_X_NUM_ROWS * BLM_X_LED_NUM_COLS)
    return 0;
  // compute row,SR
  row = led / BLM_X_LED_NUM_COLS;
  color = 0;
  for(c = 0; c < BLM_X_LED_NUM_COLORS; c++){
    //compute sr,pin
    if(blm_x_config.color_mode == 0)
      sr = ( pin = led % BLM_X_LED_NUM_COLS + BLM_X_LED_NUM_COLS*c ) / 8;
    else
      sr = ( pin = (led % BLM_X_LED_NUM_COLS)*BLM_X_LED_NUM_COLORS + c ) / 8;
    pin %= 8;
    // get value
    if( BLM_X_LED_rows[row][sr] & (1 << pin) )
      color |= (1 << c);//set pin
    else
      color &= ~(1 << c);//clear pin
  }
  return color;
#else
  return 0;//LED's disabled
#endif
}

/////////////////////////////////////////////////////////////////////////////
// returns the status of a LED / color
// IN: LED number in <led>, color in <color>
// OUT: returns LED state value or < 0 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_LEDGet(u32 led, u32 color){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  u32 row,sr,pin;
  // check if pin available
  if( led >= BLM_X_NUM_ROWS * BLM_X_LED_NUM_COLS || color > BLM_X_LED_NUM_COLORS )
    return -1;
  // compute row,sr,pin
  row = led / BLM_X_LED_NUM_COLS;
  if(blm_x_config.color_mode == 0)
    sr = ( pin = led % BLM_X_LED_NUM_COLS + BLM_X_LED_NUM_COLS*color ) / 8;
  else
    sr = ( pin = (led % BLM_X_LED_NUM_COLS)*BLM_X_LED_NUM_COLORS + color ) / 8;
  pin %= 8;
  // return value
  return (BLM_X_LED_rows[row][sr] & (1 << pin)) ? 1 : 0;
#else
  return -1;//LED's disabled
#endif
}

/////////////////////////////////////////////////////////////////////////////
// returns the LED serial-register value for a row / SR
// IN: LED row in <row>, row-SR in <sr>
// OUT: serial register value (0 if register not available)
/////////////////////////////////////////////////////////////////////////////
u8 BLM_X_LEDSRGet(u8 row, u8 sr){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  if (row > BLM_X_NUM_ROWS -1 || sr > BLM_X_NUM_LED_SR - 1)
    return 0;
  return BLM_X_LED_rows[row][sr];
#else
  return 0;//LED's disabled
#endif
}


/////////////////////////////////////////////////////////////////////////////
// sets the LED serial-register value for a row / sr
// IN: LED row in <row>, row-SR in <sr>
// OUT: < 0 on error (SR not available), 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_LEDSRSet(u8 row, u8 sr, u8 sr_value){
#if (BLM_X_LED_FIRST_DOUT_SR > 0)
  if (row > BLM_X_NUM_ROWS -1 || sr > BLM_X_NUM_LED_SR - 1)
    return -1;
  BLM_X_LED_rows[row][sr] = sr_value;
  return 0;
#else
  return -1;//LED's disabled
#endif
}

/////////////////////////////////////////////////////////////////////////////
// sets the blm_x soft configurations
// IN: config, struct with members:
//    .rowsel_dout_sr
//    .led_first_dout_sr
//    .led_first_din_sr
//    .rowsel_inv_mask
//    .color_mode
//    .debounce_delay
// OUT: returns < 0 on error, 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 BLM_X_ConfigSet(blm_x_config_t config){
  blm_x_config = config;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// gets the blm_x soft configurations
// IN: -
// OUT: struct with members:
//    .rowsel_dout_sr
//    .led_first_dout_sr
//    .led_first_din_sr
//    .rowsel_inv_mask
//    .color_mode
//    .debounce_delay
/////////////////////////////////////////////////////////////////////////////
blm_x_config_t BLM_X_ConfigGet(void){
  return blm_x_config;
}
