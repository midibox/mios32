// $Id$
//! \defgroup AINSER
//!
//! AINSER module driver
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

#include "ainser.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 num_used_modules = AINSER_NUM_MODULES;
static u8 num_used_pins = AINSER_NUM_PINS;

static u16 ain_pin_values[AINSER_NUM_MODULES][AINSER_NUM_PINS];

static u8 ain_deadband = MIOS32_AIN_DEADBAND;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 AINSER_SetCs(u8 module, u8 value);


/////////////////////////////////////////////////////////////////////////////
//! Initializes AINSER driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_Init(u32 mode)
{
  s32 status = 0;
  int module, pin;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if AINSER_SPI_OUTPUTS_OD
  // pins in open drain mode (to pull-up the outputs to 5V)
  status |= MIOS32_SPI_IO_Init(AINSER_SPI, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
  // pins in push-poll mode (3.3V output voltage)
  status |= MIOS32_SPI_IO_Init(AINSER_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif

  // SPI Port will be initialized in AINSER_Update()

  num_used_modules = AINSER_NUM_MODULES;
  num_used_pins = AINSER_NUM_PINS;
  ain_deadband = MIOS32_AIN_DEADBAND;

  for(module=0; module<AINSER_NUM_MODULES; ++module) {
    // ensure that CS is deactivated
    AINSER_SetCs(module, 1);

    // clear all values
    for(pin=0; pin<AINSER_NUM_PINS; ++pin) {
      ain_pin_values[module][pin] = 0;
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! \return the number of modules which are scanned
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_NumModulesGet(void)
{
  return num_used_modules;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the number of modules which should be scanned
//! \return < 0 on error (e.g. unsupported number of modules)
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_NumModulesSet(u8 num_modules)
{
  if( num_modules >= AINSER_NUM_MODULES )
    return -1;

  num_used_modules = num_modules;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \return the number of AIN pins per module which are scanned
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_NumPinsGet(void)
{
  return num_used_pins;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the number of AIN pins per module which should be scanned
//! \return < 0 on error (e.g. unsupported number of pins)
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_NumPinsSet(u8 num_pins)
{
  if( num_pins >= AINSER_NUM_PINS )
    return -1;

  num_used_pins = num_pins;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return the deadband which is used to notify changes
//! \return < 0 on error
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_DeadbandGet(void)
{
  return ain_deadband;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the difference between last and current pot value which has to
//! be achieved to trigger the callback function passed to AINSER_Handler()
//! \return < 0 on error
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_DeadbandSet(u8 deadband)
{
  ain_deadband = deadband;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return the AIN pin value of the given module and pin
//! \return < 0 if wrong module or pin selected!
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_PinGet(u8 module, u8 pin)
{
  if( module >= AINSER_NUM_MODULES )
    return -1;

  if( pin >= AINSER_NUM_PINS )
    return -1;

  return ain_pin_values[module][pin];
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be periodically called to scan AIN pin changes.
//!
//! A scan of a single multiplexer selection takes ca. 50 uS on a LPC1769 with MIOS32_SPI_PRESCALER_8
//!
//! Whenever a pin has changed, the given callback function will be called.\n
//! Example:
//! \code
//!   void AINSER_NotifyChanged(u32 pin, u16 value);
//! \endcode
//! \param[in] _callback pointer to callback function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 AINSER_Handler(void (*_callback)(u32 module, u32 pin, u32 value))
{
  // the mux_ctr -> pin mappin is layout dependend
  //const u8 mux_pin_map[8] = {0, 1, 2, 3, 4, 5, 6, 7 };
  const u8 mux_pin_map[8] = {1, 4, 3, 5, 2, 7, 0, 6 }; // order of MUX channels: 6, 0, 4, 2, 1, 3, 7, 5
  static u8 mux_ctr = 0; // will be incremented on each update to select the next AIN pin
  static u8 first_scan_done = 0;
  s32 status = 0;

  // init SPI port for fast frequency access
  // we will do this here, so that other handlers (e.g. AOUT) could use SPI in different modes
  // Maxmimum allowed SCLK is 2 MHz according to datasheet
  // We select prescaler 64 @120 MHz (-> ca. 500 nS period)
  status |= MIOS32_SPI_TransferModeInit(AINSER_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_64);

  // determine next MUX selection
  int next_mux_ctr = (mux_ctr + 1) % 8;

  // loop over connected modules
  int module;
  for(module=0; module<num_used_modules; ++module) {

    // loop over channels
    int chn;
    for(chn=0; chn<8; ++chn) {
      // CS=0
      status |= AINSER_SetCs(module, 0);

      // retrieve conversion values
      // shift in start bit + SGL + MSB of channel selection, shift out dummy byte
      MIOS32_SPI_TransferByte(AINSER_SPI, 0x06 | (chn>>2));
      // shift in remaining 2 bits of channel selection, shift out MSBs of conversion value
      u8 b1 = MIOS32_SPI_TransferByte(AINSER_SPI, chn << 6);
      // shift in mux_ctr + "Link LED" status to 74HC595, shift out LSBs of conversion value
      u8 b2 = MIOS32_SPI_TransferByte(AINSER_SPI, ((chn == 7 ? next_mux_ctr : mux_ctr) << 5) | 1);

      // CS=1 (the rising edge will update the 74HC595)
      AINSER_SetCs(module, 1);

      // store conversion value if difference to old value is outside the deadband
      u16 pin = mux_pin_map[mux_ctr] + 8*(7-chn); // the mux/chn -> pin mapping is layout dependend
      u16 value = (b2 | (b1 << 8)) & 0xfff;
      int diff = value - ain_pin_values[module][pin];
      int abs_diff = (diff > 0 ) ? diff : -diff;

      if( !first_scan_done || abs_diff > ain_deadband ) {
	ain_pin_values[module][pin] = value;

	// notify callback function
	// check pin number as well... just to ensure
	if( first_scan_done && _callback && pin < num_used_pins )
	  _callback(module, pin, value);
      }
    }
  }

  // select MUX input
  mux_ctr = next_mux_ctr;

  // one complete scan done?
  if( next_mux_ctr == 0 )
    first_scan_done = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Internal function to set CS line depending on module
/////////////////////////////////////////////////////////////////////////////
static s32 AINSER_SetCs(u8 module, u8 value)
{
  switch( module ) {
  case 0: return MIOS32_SPI_RC_PinSet(AINSER_SPI, AINSER_SPI_RC_PIN_MODULE1, value); // spi, rc_pin, pin_value
  case 1: return MIOS32_SPI_RC_PinSet(AINSER_SPI, AINSER_SPI_RC_PIN_MODULE2, value); // spi, rc_pin, pin_value

#if AINSER_NUM_MODULES > 2
# error "CS Line for more than 2 modules not prepared yet - please enhance here!"
#endif
  }


  return -1;
}


//! \}
