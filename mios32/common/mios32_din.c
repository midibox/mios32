// $Id$
//! \defgroup MIOS32_DIN
//!
//! DIN functions for MIOS32
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_DIN)

/////////////////////////////////////////////////////////////////////////////
//! Initializes DIN driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_Init(u32 mode)
{
  u8 i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear DIN part of SRIO chain
  // TODO: here we could provide an option to invert the default value
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    mios32_srio_din[i] = 0xff; // passive state
    mios32_srio_din_changed[i] = 0;
  }
  
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns state of a DIN pin
//! \param[in] pin number (0..127)
//! \return 1 if pin is Vss (ie. 5V)
//! \return 0 if pin is 0V
//! \return -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_PinGet(u32 pin)
{
  // check if pin available
  if( pin/8 >= MIOS32_SRIO_NUM_SR )
    return -1;

  return (mios32_srio_din[pin >> 3] & (1 << (pin&7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns state of a DIN shift register
//! \param[in] sr shift register number (0..15)
//! \return 8bit value of shift register
//! \return -1 if shift register not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_SRGet(u32 sr)
{
  // check if SR available
  if( sr >= MIOS32_SRIO_NUM_SR )
    return -1;

  return mios32_srio_din[sr];
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the DIN pin toggle notifications after a SRIO scan. Should be used 
//! to check and clear these flags when processed by a DIN handler for exclusive
//! access (avoid propagation of already handled DIN pin changes)
//!
//! Examples:<BR>
//! 1) if a scan matrix handler processes the DIN pins of the first SR,
//! it should call
//! \code
//!   changed = MIOS32_DIN_SRChangedGetAndClear(0, 0xff)
//! \endcode
//! to get a notification, if (and which) pins have toggled during the last scan.
//! The flags will be cleared, so that the remaining handlers won't process
//! the pins of the first DIN register anymore.
//!
//! 2) it's also possible, to check and clear only dedicated pins of the DIN
//!    register by applying a different pin mask. E.g.
//! \code
//!   changed = MIOS32_DIN_SRChangedGetAndClear(0, 0xc0)
//! \endcode
//!    will only check/clear D6 and D7 of the DIN register, the remaining
//!    toggle notifications won't be touched and will be forwarded to remaining 
//!    handlers
//! \param[in] sr shift register number (0..15)
//! \param[in] mask pin mask (8bit value)
//! \return 8bit value which contains the selected (masked) change flags
//! \return no error status (-1)! - if unavailable SR selected, 0x00 will be returned
/////////////////////////////////////////////////////////////////////////////
u8 MIOS32_DIN_SRChangedGetAndClear(u32 sr, u8 mask)
{
  u8 changed;

  // check if SR available
  if( sr >= MIOS32_SRIO_NUM_SR )
    return 0x00;

  // get and clear changed flags - must be atomic!
  MIOS32_IRQ_Disable();
  changed = mios32_srio_din_changed[sr] & mask;
  mios32_srio_din_changed[sr] &= ~mask;
  MIOS32_IRQ_Enable();

  return changed;
}


/////////////////////////////////////////////////////////////////////////////
//! Disables the auto-repeat feature for the appr. pin
//! \param[in] pin pin number (0..127)
//! \return < 0 on errors
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatDisable(u32 pin)
{
  // not implemented yet
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//! Enables the auto-repeat feature for the appr. pin
//! \param[in] pin pin number (0..127)
//! \return < 0 on errors
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatEnable(u32 pin)
{
  // not implemented yet
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//! queries the auto-repeat state of a DIN pin
//! \param[in] pin pin number (0..127)
//! \return 1 if auto-repeat has been enabled for this pin
//! \return 0 if auto-repeat has been disabled for this pin
//! \return -1 if pin nor available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_PinAutoRepeatGet(u32 pin)
{
  // not implemented yet
  return 0; // auto repeat not enabled
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the debounce counter reload value of the DIN SR registers
//! \return debounce counter reload value
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
u32 MIOS_DIN_DebounceGet(void)
{
  // TODO
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the debounce counter reload value for the DIN SR registers which are 
//! not assigned to rotary encoders to debounce low-quality buttons.
//!
//! Debouncing is realized in the following way: on every button movement 
//! the debounce preload value will be loaded into the debounce counter 
//! register. The counter will be decremented on every SRIO update cycle. 
//! So long as this counter isn't zero, button changes will still be recorded, 
//! but they won't trigger the USER_DIN_NotifyToggle function (FIXME)
//!
//! No (intended) button movement will get lost, but the latency will be 
//! increased. Example: if the update frequency is set to 1 mS, and the 
//! debounce value to 32, the first button movement will be regognized 
//! with a worst-case latency of 1 mS. Every additional button movement 
//! which happens within 32 mS will be regognized within a worst-case 
//! latency of 32 mS. After the debounce time has passed, the worst-case 
//! latency is 1 mS again.
//!
//! This function affects all DIN registers. If the application should 
//! record pin changes from digital sensors which are switching very fast, 
//! then debouncing should be ommited.
//! \return < 0 on errors
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS_DIN_DebounceSet(u32 debounce_time)
{
  // TODO
  return -1;
}


/////////////////////////////////////////////////////////////////////////////
//! Checks for pin changes, and calls given callback function with following parameters:
//! \code
//!   void DIN_NotifyToggle(u32 pin, u32 value)
//! \endcode
//! \param[in] _callback pointer to callback function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DIN_Handler(void *_callback)
{
  s32 sr;
  s32 sr_pin;
  u8 changed;
  void (*callback)(u32 pin, u32 value) = _callback;

  // no SRIOs?
#if MIOS32_SRIO_NUM_SR == 0
  return -1;
#endif

  // no callback function?
  if( _callback == NULL )
    return -1;

  // check all shift registers for DIN pin changes
  for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr) {
    
    // check if there are pin changes (mask all pins)
    changed = MIOS32_DIN_SRChangedGetAndClear(sr, 0xff);

    // any pin change at this SR?
    if( !changed )
      continue;

    // check all 8 pins of the SR
    for(sr_pin=0; sr_pin<8; ++sr_pin)
      if( changed & (1 << sr_pin) )
	callback(8*sr+sr_pin, (mios32_srio_din[sr] & (1 << sr_pin)) ? 1 : 0);
  }

  return 0;
}

//! \}

#endif /* MIOS32_DONT_USE_DIN */
