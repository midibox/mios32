// $Id$
/*
 * DIN functions for MIOS32
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

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes DIN driver
// IN: <mode>: currently only mode 0 supported
//             later we could provide operation modes
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_Init(u32 mode)
{
  u8 i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear DIN part of SRIO chain
  // TODO: here we could provide an option to invert the default value
  for(i=0; i<MIOS32_SRIO_NUM_MAX; ++i) {
    mios32_srio_din[i] = 0xff; // passive state
    mios32_srio_din_changed[i] = 0;
  }
  
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns value from a DIN Pin
// IN: pin number in <pin>
// OUT: 1 if pin is Vss, 0 if pin is 0V, -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_PinGet(u32 pin)
{
  // check if pin available
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( pin/8 >= MIOS32_SRIO_NUM_MAX )
    return -1;

  return (mios32_srio_din[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// returns value of DIN shift register
// IN: shift register number in <sr>
// OUT: 8bit value of shift register, -1 if SR not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_SRGet(u32 sr)
{
  // check if SR available
  // check against max number of SRIO instead of srio_num to ensure, that apps 
  // don't start to behave strangely if the user reduces the number of SRIOs)
  if( sr >= MIOS32_SRIO_NUM_MAX )
    return -1;

  return mios32_srio_din[sr];
}

/////////////////////////////////////////////////////////////////////////////
// disables the auto-repeat feature for the appr. pin
// IN: pin number in <pin>
// OUT: returns != 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatDisable(u32 pin)
{
  // not implemented yet
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
// enables the auto-repeat feature for the appr. pin
// IN: pin number in <pin>
// OUT: returns != 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatEnable(u32 pin)
{
  // not implemented yet
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
// returns != 0 if auto-repeat has been enabled for the appr. pin
// IN: pin number in <pin>
// OUT: != 0 if auto-repeat has been enabled for this pin
//      0 if auto-repeat has been disabled for this pin
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatGet(u32 pin)
{
  // not implemented yet
  return 0; // auto repeat not enabled
}

/////////////////////////////////////////////////////////////////////////////
// returns the debounce counter reload value of the DIN SR registers
// IN: -
// OUT: debounce counter reload value
/////////////////////////////////////////////////////////////////////////////
u32 MIOS_DIN_DebounceGet(void)
{
  // TODO
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Sets the debounce counter reload value for the DIN SR registers which are 
// not assigned to rotary encoders to debounce low-quality buttons.
//
// Debouncing is realized in the following way: on every button movement 
// the debounce preload value will be loaded into the debounce counter 
// register. The counter will be decremented on every SRIO update cycle. 
// So long as this counter isn't zero, button changes will still be recorded, 
// but they won't trigger the USER_DIN_NotifyToggle function (FIXME)
//
// No (intended) button movement will get lost, but the latency will be 
// increased. Example: if the update frequency is set to 1 mS, and the 
// debounce value to 32, the first button movement will be regognized 
// with a worst-case latency of 1 mS. Every additional button movement 
// which happens within 32 mS will be regognized within a worst-case 
// latency of 32 mS. After the debounce time has passed, the worst-case 
// latency is 1 mS again.
//
// This function affects all DIN registers. If the application should 
// record pin changes from digital sensors which are switching very fast, 
// then debouncing should be ommited.
// IN: -
// OUT: returns != 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_DebounceSet(u32 debounce_time)
{
  // TODO
  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Checks for pin changes, and calls given hook with following parameters:
//   - u32 pin: pin number
//   - u32 value: pin value
//
// Example: MIOS32_DIN_Handler(DIN_NotifyToggle);
//          will call
//          static void DIN_NotifyToggle(u32 pin, u32 value)
//          on pin changes
// OUT: returns -1 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_Handler(void *_notify_hook)
{
  s32 sr;
  s32 num_sr;
  s32 sr_pin;
  u8 changed;
  void (*notify_hook)(u32 pin, u32 value) = _notify_hook;

  // no SRIOs?
  if( (num_sr=MIOS32_SRIO_NumberGet()) <= 0 ) {
    return -1;
  }

  // check all shift registers for DIN pin changes
  for(sr=0; sr<num_sr; ++sr) {
    
    // get and clear changed flags - must be atomic!
    portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
    changed = mios32_srio_din_changed[sr];
    mios32_srio_din_changed[sr] = 0;
    portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    for(sr_pin=0; sr_pin<8; ++sr_pin)
      if( changed & (1 << sr_pin) )
	notify_hook(8*sr+sr_pin, (mios32_srio_din[sr] & (1 << sr_pin)) ? 1 : 0);
  }

  return 0;
}
