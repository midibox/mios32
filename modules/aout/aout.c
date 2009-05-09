// $Id$
//! \defgroup AOUT
//!
//! AOUT module driver
//!
//! The application interface (API) has been tailored around AOUT modules, which
//! are part of the MIDIbox Hardware Platform (MBHP), and MIOS based applications
//! like MIDIbox CV (CV control via MIDI), MIDIbox SEQ (CV control from a 
//! sequencer) and MIDIbox SID/FM (CV control from a synthesizer)
//! 
//! Up to 32 analog outputs are supported (*)
//! An interface to control digital pins is available as well (**)
//! 
//! Output voltages are managed in 16bit resolution. Although none of the current
//! modules support the full resolution, from programming and performance
//! perspective it makes sense to organize CV values this way.
//! 
//! If 7bit values should be output, the caller has to left-align the value, e.g.
//! \code
//! AOUT_PinSet(0, value_7bit << 9); // left-align MSB to 16bit
//! \endcode
//! 
//! Before voltage changes are transfered to the external hardware, the 
//! AOUT_PinSet function compares the new value with the current one.
//! If equal, the register transfer will be omitted, otherwise it
//! will be requested and performed once AOUT_Update() is called.
//! 
//! This method has two advantages:
//! <UL>
//!   <LI>if AOUT_PinSet doesn't notice value changes, the appr. AOUT channels
//!     won't be updated to save CPU performance
//!   <LI>all CV pins will be updated at the same moment
//! </UL>
//!
//! (*) currently only limited by the aout_update_req variable. This could be enhanced 
//! in future if really required, but this would cost performance!
//!
//! (**) currently only supported for MBHP_AOUT, since MAX525 provides such a digital output pin
//!
//!
//! Supported interface types:
//! <UL>
//!  <LI>0 (AOUT_IF_NONE): interface disabled, but SPI port will be initialized
//!  <LI>1 (AOUT_IF_MAX525):<BR>
//!      CV: up to 8 cascaded MAX525 (MBHP_AOUT module), each MAX525 provides 
//!      4 channels with 12bit resolution<BR>
//!      Digital: 1 pin per IC</LI>
//!  <LI>2 (AOUT_IF_74HC595):<BR>
//!      CV: up to 32 cascaded 74HC595 (each MBHP_AOUT_LC module provides 2 74HC595)<BR>
//!      Resolution is selectable with interface_option, each 74HC595 has an own bit
//!      which define:
//!      <UL>
//!        <LI>0: 12/4 bit configuration
//!        <LI>1: 8/8 bit configuration
//!      </UL>
//!      Examples:
//!      <UL>
//!        <LI>0x00000000: all AOUT_LC modules used for 12/4 bit configuration
//!        <LI>0xffffffff: all AOUT_LC modules used for 8/8 bit configuration
//!        <LI>0x00000003: first AOUT_LC used for 8/8 bit configuration, all others for 12/4
//!        <LI>0x0000000c: second AOUT_LC used for 8/8 bit configuration, all others for 12/4
//!      </UL>
//!      Digital: none</LI>
//!  <LI>3 (AOUT_IF_TLV5630):<BR>
//!      CV: one TLV5630 (MBHP_AOUT_NG module) with 8 channels at
//!      12bit resolution<BR>
//!      Digital: none</LI>
//! </UL>
//!
//! Example for a complete module configuration:
//! \code
//!   // initialize AOUT module
//!  AOUT_Init(0);
//!
//!  // configure interface
//!  // see AOUT module documentation for available interfaces and options
//!  aout_config_t config;
//!  config = AOUT_ConfigGet();
//!  config.if_type = AOUT_IF_MAX525;
//!  config.if_option = 0;
//!  config.num_channels = 8;
//!  config.chn_inverted = 0;
//!  AOUT_ConfigSet(config);
//!  AOUT_IF_Init(0);
//! \endcode
//!
//! 
//! Hardware configuration (settings are located in aout.h, and can be
//! overloaded in mios32_config.h if required):
//! <UL>
//!   <LI>AOUT_NUM_CHANNELS (default: 8)<BR>
//!     maximum number of AOUT channels (1..32)<BR>
//!     Can be changed via soft-configuration during runtime
//!   <LI>AOUT_SPI (default: 2)<BR>
//!     allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
//!   <LI>AOUT_SPI_RC_PIN (default: 0)<BR>
//!     Specifies the RC pin of the SPI port which should be used.<BR>
//!     Allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
//!   <LI>AOUT_SPI__OUTPUTS_OD (default: 1)<BR>
//!     Specifies if output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
//! </UL>
//!
//! For the default setup, the Module is connected to J19 of the MBHP_CORE_STM32 module:
//! <UL>
//!   <LI>J19:Vs to AOUT Vs
//!   <LI>J19:Vd to AOUT Vd
//!   <LI>J19:SO to AOUT SI
//!   <LI>J19:SC to AOUT SC
//!   <LI>J19:RC1 to AOUT CS
//! </UL>
//! Note that this is *not* an 1:1 pin assignment (an adapter has to be used)!
//!
//! The voltage configuration jumper of J19 has to be set to 5V, and
//! a 4x1k Pull-Up resistor array should be installed, since the IO pins
//! are configured in open-drain mode for 3.3V->5V level shifting.
//!
//! An usage example can be found under $MIOS32_PATH/apps/examples/aout
//!
//! \todo Add AOUT driver for integrated 2-channel DAC of STM32F103RE
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "aout.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static aout_config_t aout_config;

static u16 aout_value[AOUT_NUM_CHANNELS];
static u32 aout_update_req;
static u8  aout_num_devices;

static u32 aout_dig_value;
static u32 aout_dig_update_req;



/////////////////////////////////////////////////////////////////////////////
//! Initializes AOUT driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_Init(u32 mode)
{
  s32 status = 0;
  int pin;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // init interface type (disable it by default)
  aout_config.if_type = AOUT_IF_NONE;
  aout_config.num_channels = AOUT_NUM_CHANNELS;
  aout_config.if_option = 0;  

  // disable inversion
  aout_config.chn_inverted = 0;

  // number of devices is 0 (changed during re-configuration)
  aout_num_devices = 0;

  // set all AOUT pins to 0
  for(pin=0; pin<AOUT_NUM_CHANNELS; ++pin)
    aout_value[pin] = 0;

  // set all digital outputs to 0
  aout_dig_value = 0;

  // init SPI
#if AOUT_SPI_OUTPUTS_OD
  // pins in open drain mode (to pull-up the outputs to 5V)
  status |= MIOS32_SPI_IO_Init(AOUT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
  // pins in push-poll mode (3.3V output voltage)
  status |= MIOS32_SPI_IO_Init(AOUT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif

  // init SPI port for fast frequency access
  status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_4);


  // init hardware
  // (not required, since no interface is selected by default!)
  // AOUT_IF_Init(mode);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! This function re-initializes the AOUT module, and requests a refresh
//! of all output channels according to the stored values in aout_values
//! 
//! The update will take place once AOUT_Update is called (e.g. from a
//! periodically called task)
//! 
//! Background info: the AOUT_PinSet function only request an update if the
//! output value has been changed (comparison between new and previous value)
//! This improves performance a lot, especially when AOUT values are written
//! periodically from a timer interrupt (see MIDIbox SID and MIDIbox FM)
//! 
//! However, sometimes it is useful to force a refresh, e.g. after a patch
//! change, to ensure that all AOUT channels are up-to-date, because it allows
//! to connect (or exchange) the AOUT module during runtime.
//!
//! This function is also important after a configuration update, e.g. after
//! a different AOUT interface type has been selected.
//!
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_IF_Init(u32 mode)
{
  s32 status = 0;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  switch( aout_config.if_type ) {
    case AOUT_IF_NONE:
      return -2; // no interface selected

    case AOUT_IF_MAX525: {
      // determine number of connected MAX525 devices depending on number of channels
      aout_num_devices = aout_config.num_channels / 4;
      if( aout_config.num_channels % 4 )
	++aout_num_devices;

      // ensure that CS is deactivated
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    } break;

    case AOUT_IF_74HC595: {
      // determine number of connected 74HC595 pairs depending on number of channels
      aout_num_devices = aout_config.num_channels / 2;
      if( aout_config.num_channels % 2 )
	++aout_num_devices;

      // set RCLK=0
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
    } break;

    case AOUT_IF_TLV5630: {
      // determine number of connected TLV5630 depending on number of channels
      aout_num_devices = aout_config.num_channels / 8;
      if( aout_config.num_channels % 8 )
	++aout_num_devices;

      // initialize CTRL0
      // DO=1 (DOUT Enable), R=3 (internal reference, 2V)
      u8 ctrl0 = (1 << 3) | (3 << 1);
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      status |= MIOS32_SPI_TransferByte(AOUT_SPI, 0x8 << 4);
      status |= MIOS32_SPI_TransferByte(AOUT_SPI, ctrl0);
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

      // initialize CTRL1
      u8 ctrl1 = 0;
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      status |= MIOS32_SPI_TransferByte(AOUT_SPI, 0x9 << 4);
      status |= MIOS32_SPI_TransferByte(AOUT_SPI, ctrl1);
      status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    } break;

    default:
      return -3; // invalid interface selected
  }

  // request update of all channels
  aout_update_req = 0xffffffff;
  aout_dig_update_req = 0xffffffff;

  return status ? -4 : 0; // SPI transfer error?
}


/////////////////////////////////////////////////////////////////////////////
//! Configures the AOUT driver.
//!
//! It is recommented to call AOUT_IF_Init() if a different AOUT interface type
//! has been selected (e.g. switch from MBHP_AOUT to MBHP_AOUT_NG)
//!
//! \param[in] config a structure with following members:
//! <UL>
//!   <LI>config.if_type: selects the interface
//!   <LI>config.if_option: allows to pass additional options to the IF driver
//!   <LI>config.num_channels: number of channels (1..AOUT_NUM_CHANNELS)
//!   <LI>config.chn_inverted: allows to invert the output of AOUT pins (each
//!       pin has a dedicated bit)
//! </UL>
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_ConfigSet(aout_config_t config)
{
  // ensure that update is atomic
  MIOS32_IRQ_Disable();
  aout_config = config;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns current AOUT configuration
//! \return config.if_type: the interface
//! \return config.if_option: interface options
//! \return config.num_channels: number of channels
//! \return config.chn_inverted: pin inversion flags
/////////////////////////////////////////////////////////////////////////////
aout_config_t AOUT_ConfigGet(void)
{
  return aout_config;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets an output channel to a given 16-bit value.
//!
//! The output value won't be transfered to the module immediately, but will 
//! be buffered instead. By calling AOUT_Update() the requested changes
//! will take place.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \param[in] value the 16bit value
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinSet(u8 pin, u16 value)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  if( value != aout_value[pin] ) {
    aout_value[pin] = value;
    aout_update_req |= 1 << pin;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the current output value of an output channel.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \return -1 if pin not available
//! \return >= 0 if pin available (16bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinGet(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  return aout_value[pin];
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets a digital pin of an AOUT module.
//!
//! Currently only the MAX5225 based MBHP_AOUT module supports two digital
//! pins
//! \param[in] pin the pin number (0..31)
//! \param[in] value 0 or 1
//! \return -1 if pin number too high (only 32 pins supported)
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_DigitalPinSet(u8 pin, u8 value)
{
  if( pin >= 32 )
    return -1; // pin not available

  u32 mask = 1 << pin;
  u32 prev_aout_dig_value = aout_dig_value;
  
  if( value )
    aout_dig_value |= mask;
  else
    aout_dig_value &= ~mask;

  // only request update on changes
  aout_dig_update_req |= aout_dig_value ^ prev_aout_dig_value;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of a digital pin.
//!
//! Currently only the MAX5225 based MBHP_AOUT module supports two digital
//! pins
//! \param[in] pin the pin number (0..31)
//! \return -1 if pin number too high (only 32 pins supported)
//! \return 0 or 1 if pin available, depending on pin state
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_DigitalPinGet(u8 pin)
{
  if( pin >= 32 )
    return -1; // pin not available

  return (aout_dig_value & (1 << pin)) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets all digital pins of an AOUT module.
//!
//! Currently only the MAX5225 based MBHP_AOUT module supports two digital
//! pins
//! \param[in] value each pin has a dedicated flag
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_DigitalPinsSet(u32 value)
{
  u32 prev_aout_dig_value = aout_dig_value;

  aout_dig_value = value;

  // only request update on changes
  aout_dig_update_req |= aout_dig_value ^ prev_aout_dig_value;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the state of all digital pins.
//!
//! Currently only the MAX5225 based MBHP_AOUT module supports two digital
//! pins
//! \return a 32bit value which contains the state of all pins
/////////////////////////////////////////////////////////////////////////////
u32 AOUT_DigitalPinsGet(void)
{
  return aout_dig_value;
}


/////////////////////////////////////////////////////////////////////////////
//! Updates the output channels of the connected AOUT module
//!
//! Should be called, whenever changes have been requested via AOUT_Pin*Set
//! or AOUT_DigitalPin*Set
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_Update(void)
{
  s32 status = 0;

  if( !aout_num_devices )
    return -1; // no device available

  // check for AOUT channel update requests
  MIOS32_IRQ_Disable();
  u32 req = aout_update_req;
  aout_update_req = 0;
  MIOS32_IRQ_Enable();

  if( req ) {
    switch( aout_config.if_type ) {
      case AOUT_IF_NONE:
	return -2; // no interface selected

      case AOUT_IF_MAX525: {
	// each device has 4 channels
	int chn;
	for(chn=0; chn<4; ++chn) {

	  // check if channel has to be updated for any device
	  if( req & (0x11111111 << chn) ) {
	    // activate chip select
	    status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	    // loop through devices (value of last device has to be shifted first)
	    int dev;
	    for(dev=aout_num_devices-1; dev>=0; --dev) {

	      // build command:
	      u8 chn_ix = 4*dev + chn;
	      u16 dac_value = aout_value[chn_ix] >> 4; // 16bit -> 12bit
	      if( aout_config.chn_inverted & (1 << chn_ix) )
		dac_value ^= 0xfff;

	      // A[10]: channel number, C1=1, C0=1
	      u16 hword = (chn << 14) | (1 << 13) | (1 << 12) | dac_value;

	      // transfer word
	      status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	      status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	    }

	    // deactivate chip select
	    status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	  }
	}
      } break;

      case AOUT_IF_74HC595: {
	// the complete chain has to be updated!

	// loop through devices (value of last device has to be shifted first)
	int dev;
	for(dev=aout_num_devices-1; dev>=0; --dev) {
	  // build DAC value depending on interface option
	  u8 mode = (aout_config.if_option >> (2*dev)) & 3;
	  u8 chn_ix = 2*dev;
	  u16 hword;
	  
	  if( mode ) {
	    // 8/8 configuration
	    u16 dac0_value = aout_value[chn_ix+0] >> 8; // 16bit -> 8bit
	    if( aout_config.chn_inverted & (1 << chn_ix) )
	      dac0_value ^= 0xff;
	    u16 dac1_value = aout_value[chn_ix+1] >> 8; // 16bit -> 8bit
	    if( aout_config.chn_inverted & (1 << chn_ix) )
	      dac1_value ^= 0xff;
	    hword = (dac1_value << 8) | dac0_value;
	  } else {
	    // 12/4 configuration
	    u16 dac0_value = aout_value[chn_ix+0] >> 4; // 16bit -> 12bit
	    if( aout_config.chn_inverted & (1 << chn_ix) )
	      dac0_value ^= 0xfff;
	    u16 dac1_value = aout_value[chn_ix+1] >> 12; // 16bit -> 4bit
	    if( aout_config.chn_inverted & (1 << chn_ix) )
	      dac1_value ^= 0xf;
	    hword = (dac1_value << 12) | dac0_value;
	  }

	  // transfer word
	  status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	  status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	}

	// toggle RCLK pin
	status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      } break;

      case AOUT_IF_TLV5630: {
	// each device has 8 channels
	int chn;
	for(chn=0; chn<8; ++chn) {

	  // check if channel has to be updated for any device
	  if( req & (0x01010101 << chn) ) {
	    // activate chip select
	    status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	    // loop through devices (value of last device has to be shifted first)
	    int dev;
	    for(dev=aout_num_devices-1; dev>=0; --dev) {

	      // build command:
	      u8 chn_ix = 8*dev + chn;
	      u16 dac_value = aout_value[chn_ix] >> 4; // 16bit -> 12bit
	      if( aout_config.chn_inverted & (1 << chn_ix) )
		dac_value ^= 0xfff;

	      // [15]=0, [14:12] channel number, [11:0] DAC value
	      u16 hword = (chn << 12) | dac_value;

	      // transfer word
	      status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	      status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	    }

	    // deactivate chip select
	    status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	  }
	}
      } break;

      default:
        return -3; // invalid interface selected
    }
  }


  // check for AOUT digital pin update requests
  MIOS32_IRQ_Disable();
  req = aout_dig_update_req;
  aout_dig_update_req = 0;
  MIOS32_IRQ_Enable();

  if( req ) {
    switch( aout_config.if_type ) {
      case AOUT_IF_NONE:
	return -2; // no interface selected

      case AOUT_IF_MAX525: {
	// activate chip select
	status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	// loop through devices (value of last device has to be shifted first)
	int dev;
	for(dev=aout_num_devices-1; dev>=0; --dev) {

	  // commands:
	  // UP0=low: A1=0, A0=0, C1=1, C0=0
	  // UP0=high: A1=0, A0=1, C1=1, C0=0
	  u16 a0 = (aout_dig_value & (1 << dev)) ? 1 : 0;
	  u16 hword = (0 << 15) | (a0 << 14) | (1 << 13) | (0 << 12);

	  // transfer word
	  status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	  status |= MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	}

	// deactivate chip select
	status |= MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
      } break;

      case AOUT_IF_74HC595:
      case AOUT_IF_TLV5630:
	// no digital outputs supported
	break;

      default:
        return -3; // invalid interface selected
    }
  }

  return status ? -4 : 0; // SPI transfer error?
}

//! \}
