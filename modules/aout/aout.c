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
//! The two "internal DAC" channels of the STM32F103RE are supported as well
//! (AOUT_IF_INTDAC)
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
//!  <LI>4 (AOUT_IF_MCP4922_1):<BR>
//!      CV: one MCP4922 with gain set to 1 with 2 channels at
//!      12bit resolution<BR>
//!      Digital: none</LI>
//!  <LI>4 (AOUT_IF_MCP4922_2):<BR>
//!      CV: one MCP4922 with gain set to 2 with 2 channels at
//!      12bit resolution<BR>
//!      Digital: none</LI>
//!  <LI>4 (AOUT_IF_INTDAC):<BR>
//!      CV: two channels available at pin RA4 (J16:RC1) and RA5 (J16:SC) at
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
//!     Maximum number of AOUT channels (1..32)<BR>
//!     Number of channels within this range can be changed via soft-configuration during runtime.
//!   <LI>AOUT_SPI (default: 2)<BR>
//!     allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
//!   <LI>AOUT_SPI_RC_PIN (default: 0)<BR>
//!     Only relevant for AOUT modules, not for AOUT_IF_INTDAC:<BR>
//!     Specifies the RC pin of the SPI port which should be used.<BR>
//!     Allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
//!   <LI>AOUT_SPI_OUTPUTS_OD (default: 1 on STM32, 0 on LPC17 module)<BR>
//!     Only relevant for AOUT modules connected to MBHP_CORE_STM32 module, not for AOUT_IF_INTDAC:<BR>
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
#include <string.h>

#include "aout.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u16  value;
  u16  original_value;
  u16  target_value;
  s32  incrementer;
  s16  pitch;
  u8   slewrate;
  u8   pitchrange;
} aout_channel_t;



// for AOUT interface testmode
// TODO: allow access to these pins via MIOS32_SPI driver
#if defined(MIOS32_FAMILY_STM32F10x)
#define MIOS32_SPI2_HIGH_VOLTAGE 4

#define MIOS32_SPI2_SCLK_PORT  GPIOB
#define MIOS32_SPI2_SCLK_PIN   GPIO_Pin_6
#define MIOS32_SPI2_MOSI_PORT  GPIOB
#define MIOS32_SPI2_MOSI_PIN   GPIO_Pin_5

#define MIOS32_SPI2_SCLK_INIT   { } // already configured as GPIO
#define MIOS32_SPI2_SCLK_SET(b) MIOS32_SYS_STM_PINSET(MIOS32_SPI2_SCLK_PORT, MIOS32_SPI2_SCLK_PIN, b);
#define MIOS32_SPI2_MOSI_INIT   { } // already configured as GPIO
#define MIOS32_SPI2_MOSI_SET(b) MIOS32_SYS_STM_PINSET(MIOS32_SPI2_MOSI_PORT, MIOS32_SPI2_MOSI_PIN, b);

#elif defined(MIOS32_FAMILY_STM32F4xx)
#define MIOS32_SPI2_HIGH_VOLTAGE 5

#define MIOS32_SPI2_SCLK_PORT  GPIOB
#define MIOS32_SPI2_SCLK_PIN   GPIO_Pin_3
#define MIOS32_SPI2_MOSI_PORT  GPIOB
#define MIOS32_SPI2_MOSI_PIN   GPIO_Pin_5

#define MIOS32_SPI2_SCLK_INIT   { GPIO_InitTypeDef GPIO_InitStructure; GPIO_StructInit(&GPIO_InitStructure); GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_SCLK_PIN; GPIO_Init(MIOS32_SPI2_SCLK_PORT, &GPIO_InitStructure); }
#define MIOS32_SPI2_SCLK_SET(b) MIOS32_SYS_STM_PINSET(MIOS32_SPI2_SCLK_PORT, MIOS32_SPI2_SCLK_PIN, b);
#define MIOS32_SPI2_MOSI_INIT   { GPIO_InitTypeDef GPIO_InitStructure; GPIO_StructInit(&GPIO_InitStructure); GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MOSI_PIN; GPIO_Init(MIOS32_SPI2_MOSI_PORT, &GPIO_InitStructure); }
#define MIOS32_SPI2_MOSI_SET(b) MIOS32_SYS_STM_PINSET(MIOS32_SPI2_MOSI_PORT, MIOS32_SPI2_MOSI_PIN, b);

#elif defined(MIOS32_FAMILY_LPC17xx)
#define MIOS32_SPI2_HIGH_VOLTAGE 5

#define MIOS32_SPI2_SCLK_INIT    { MIOS32_SYS_LPC_PINSEL(0, 15, 0); MIOS32_SYS_LPC_PINDIR(0, 15, 1); }
#define MIOS32_SPI2_SCLK_SET(v)  MIOS32_SYS_LPC_PINSET(0, 15, v)
#define MIOS32_SPI2_MOSI_INIT    { MIOS32_SYS_LPC_PINSEL(0, 18, 0); MIOS32_SYS_LPC_PINDIR(0, 18, 1); }
#define MIOS32_SPI2_MOSI_SET(v)  MIOS32_SYS_LPC_PINSET(0, 18, v)
#elif defined(MIOS32_FAMILY_EMULATION)
#define MIOS32_SPI2_HIGH_VOLTAGE 5
#else
# error "Please adapt MIOS32_SPI settings!"
#endif



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static aout_config_t aout_config;

static aout_channel_t aout_channel[AOUT_NUM_CHANNELS];

static u32 aout_update_req;
static u8  aout_num_devices;

static u32 aout_dig_value;
static u32 aout_dig_update_req;

static aout_cali_mode_t cali_mode;
static u8 cali_pin;

static u8 suspend_mode;

// include generate file which declares hz_v_table[128]
#include "aout_hz_v_table.inc"


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static u16 caliValue(u8 pin);


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

  // disable Hz/V option
  aout_config.chn_hz_v = 0;

  // number of devices is 0 (changed during re-configuration)
  aout_num_devices = 0;

  // set all AOUT pins to 0
  aout_channel_t *c = (aout_channel_t *)&aout_channel[0];
  for(pin=0; pin<AOUT_NUM_CHANNELS; ++pin, ++c) {
    c->value = 0;
    c->original_value = 0;
    c->target_value = 0;
    c->incrementer = 0;
    c->slewrate = 0;
    c->pitchrange = 2; // semitones
    c->pitch = 0; // pitch offset
  }

  // set all digital outputs to 0
  aout_dig_value = 0;

  // disable calibration mode
  cali_mode = AOUT_CALI_MODE_OFF;
  cali_pin = 0;

  // disable suspend mode
  suspend_mode = 0;

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

  // disable suspend mode
  suspend_mode = 0;

  // init SPI for all interfaces beside of NONE and INTDAC
  if( aout_config.if_type != AOUT_IF_NONE && aout_config.if_type != AOUT_IF_INTDAC ) {
    // ensure that internal DAC pins disabled
    int chn;
    for(chn=0; chn<2; ++chn)
      MIOS32_BOARD_DAC_PinInit(chn, 0);

#if AOUT_SPI_OUTPUTS_OD
    // pins in open drain mode (to pull-up the outputs to 5V)
    status |= MIOS32_SPI_IO_Init(AOUT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
    // pins in push-poll mode (3.3V output voltage)
    status |= MIOS32_SPI_IO_Init(AOUT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif
  }

  switch( aout_config.if_type ) {
    case AOUT_IF_NONE:
      return -2; // no interface selected

    case AOUT_IF_MAX525: {
      // determine number of connected MAX525 devices depending on number of channels
      aout_num_devices = aout_config.num_channels / 4;
      if( aout_config.num_channels % 4 )
	++aout_num_devices;

      // ensure that CS is deactivated
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

      // init SPI (NOTE: keep this aligned with re-init in AOUT_Update!)
      status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit
      
    } break;

    case AOUT_IF_74HC595: {
      // determine number of connected 74HC595 pairs depending on number of channels
      aout_num_devices = aout_config.num_channels / 2;
      if( aout_config.num_channels % 2 )
	++aout_num_devices;

      // set RCLK=0
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

      // init SPI (NOTE: keep this aligned with re-init in AOUT_Update!)
      status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit
    } break;

    case AOUT_IF_TLV5630: {
      // determine number of connected TLV5630 depending on number of channels
      aout_num_devices = aout_config.num_channels / 8;
      if( aout_config.num_channels % 8 )
	++aout_num_devices;

      // init SPI (NOTE: keep this aligned with re-init in AOUT_Update!)
      status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit

      // initialize CTRL0
      // DO=1 (DOUT Enable), R=3 (internal reference, 2V)
      u8 ctrl0 = (1 << 3) | (3 << 1);
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      MIOS32_SPI_TransferByte(AOUT_SPI, 0x8 << 4);
      MIOS32_SPI_TransferByte(AOUT_SPI, ctrl0);
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

      // initialize CTRL1
      u8 ctrl1 = 0;
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      MIOS32_SPI_TransferByte(AOUT_SPI, 0x9 << 4);
      MIOS32_SPI_TransferByte(AOUT_SPI, ctrl1);
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    } break;

    case AOUT_IF_MCP4922_2:
    case AOUT_IF_MCP4922_1: {
      // only 1 device per CS line
      aout_num_devices = 1;

      // ensure that CS is deactivated
      MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

      // init SPI (NOTE: keep this aligned with re-init in AOUT_Update!)
      status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit
      
    } break;

    case AOUT_IF_INTDAC: {
      // one device and up to 2 channels
      aout_num_devices = 1;

      int chn;
      for(chn=0; chn<aout_config.num_channels; ++chn)
	MIOS32_BOARD_DAC_PinInit(chn, 1);
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
// \return the interface name (8 chars)
/////////////////////////////////////////////////////////////////////////////
// located outside the function to avoid "core/seq_cv.c:168:3: warning: function returns address of local variable"
static const char *if_name[AOUT_NUM_IF] = {
  "none", // note: don't add spaces! this string is used for parsing as well
  "AOUT",
  "AOUT_LC",
  "AOUT_NG",
  "MCP_Gx1",
  "MCP_Gx2",
  "INTDAC",
};
const char* AOUT_IfNameGet(aout_if_t if_type)
{
  if( if_type >= AOUT_NUM_IF )
    if_type = 0; // select "none"

  return if_name[if_type];
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
  aout_update_req = 0xffffffff;

  // set values again to ensure that they will be updated depending on curve parameter
  aout_channel_t *c = (aout_channel_t *)&aout_channel[0];
  int pin;
  for(pin=0; pin<AOUT_NUM_CHANNELS; ++pin, ++c) {
    u16 original_value = c->original_value;
    c->original_value ^= 0xffff; // force update
    AOUT_PinSet(pin, original_value);
  }

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
//! gives direct access to the chn_inverted field of the AOUT configuration
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_ConfigChannelInvertedSet(u8 cv, u8 inverted)
{
  if( inverted )
    aout_config.chn_inverted |= (1 << cv);
  else
    aout_config.chn_inverted &= ~(1 << cv);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! gives direct access to the chn_hz_v field of the AOUT configuration
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_ConfigChannelHzVSet(u8 cv, u8 hz_v)
{
  if( hz_v )
    aout_config.chn_hz_v |= (1 << cv);
  else
    aout_config.chn_hz_v &= ~(1 << cv);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables calibration mode for a given pin
//!
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \param[in] mode the calibration mode
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_CaliModeSet(u8 pin, aout_cali_mode_t mode)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  if( mode >= AOUT_NUM_CALI_MODES )
    return -2; // invalid mode selected

  MIOS32_IRQ_Disable();
  aout_update_req |= 1 << cali_pin; // ensure that previous pin will be updated (if pin changes)
  cali_pin = pin; // switch to new pin
  cali_mode = mode; // and new mode
  aout_update_req |= 1 << pin; // update (new) pin
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the current calibration mode
//! \return the calibration mode
/////////////////////////////////////////////////////////////////////////////
aout_cali_mode_t AOUT_CaliModeGet(void)
{
  return cali_mode;
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the pin which is selected for calibration mode
//! \return the selected pin
/////////////////////////////////////////////////////////////////////////////
u8 AOUT_CaliPinGet(void)
{
  return cali_pin;
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the name of a given calibration mode
//!
//! \param[in] mode the calibration mode
//! \return 6 characters
/////////////////////////////////////////////////////////////////////////////
// located outside the function to avoid "core/seq_cv.c:168:3: warning: function returns address of local variable"
static const char cali_desc[AOUT_NUM_CALI_MODES][7] = {
  " off  ",
  " Min. ",
  "Middle",
  " Max. ",
  " 1.00V",
  " 2.00V",
  " 4.00V",
  " 8.00V",
};
const char* AOUT_CaliNameGet(aout_cali_mode_t mode)
{
  if( mode >= AOUT_NUM_CALI_MODES )
    mode = AOUT_CALI_MODE_OFF;

  return cali_desc[(u8)mode];
}


/////////////////////////////////////////////////////////////////////////////
// Returns 16bit calibration value depending on current mode
/////////////////////////////////////////////////////////////////////////////
static u16 caliValue(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return 0x0000; // pin not available

  u8 hz_v = aout_config.chn_hz_v & (1 << pin);

  switch( cali_mode ) {
  case AOUT_CALI_MODE_OFF: return 0x0000;
  case AOUT_CALI_MODE_MIN: return 0x0000;
  case AOUT_CALI_MODE_MIDDLE: return 0x8000;
  case AOUT_CALI_MODE_MAX: return 0xffff;
  case AOUT_CALI_MODE_1V: return hz_v ? hz_v_table[0x24] : (0x0c << 9);
  case AOUT_CALI_MODE_2V: return hz_v ? hz_v_table[0x30] : (0x18 << 9);
  case AOUT_CALI_MODE_4V: return hz_v ? hz_v_table[0x3c] : (0x30 << 9);
  case AOUT_CALI_MODE_8V: return hz_v ? hz_v_table[0x48] : (0x60 << 9);
  }

  return 0x0000;
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

  aout_channel_t *c = (aout_channel_t *)&aout_channel[pin];
  if( value != c->original_value ) {
    MIOS32_IRQ_Disable();
    c->original_value = value;

    // calculate pitch and consider Hz/V option
    if( aout_config.chn_hz_v & (1 << pin) ) {
      u8 ix = value >> 9; // 0..127

      if( c->pitch == 0 )
	value = hz_v_table[ix];
      else {
	if( c->pitch > 0 ) {
	  int uix = ix + c->pitchrange;
	  if( uix > 127 )
	    uix = 127;

	  int lvalue = hz_v_table[ix];
	  int uvalue = hz_v_table[uix];
	  value = lvalue + (((int)c->pitch * (uvalue-lvalue)) / 8191); 
	} else {
	  int lix = ix - c->pitchrange;
	  if( lix < 0 )
	    lix = 0;

	  int lvalue = hz_v_table[lix];
	  int uvalue = hz_v_table[ix];
	  value = uvalue + (((int)c->pitch * (uvalue-lvalue)) / 8192);
	}
      }
    } else {
      if( c->pitch > 0 ) {
	int newvalue = value + (((int)c->pitch * ((int)c->pitchrange << 9)) / 8191); 
	value = (newvalue > 0xffff) ? 0xffff : newvalue;
      } else if( c->pitch < 0 ) {
	int newvalue = value + (((int)c->pitch * ((int)c->pitchrange << 9)) / 8192); 
	value = (newvalue < 0) ? 0 : newvalue;
      }
    }

    c->target_value = value;

    if( c->slewrate == 0 || value == c->value ) {
      c->incrementer = 0;
      c->value = value;
    } else {
      c->incrementer = (value - c->value) / c->slewrate;
      if( c->incrementer == 0 )
	c->incrementer = 1;
    }

    aout_update_req |= 1 << pin;
    MIOS32_IRQ_Enable();
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the current (target) output value of an output channel.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \return -1 if pin not available
//! \return >= 0 if pin available (16bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinGet(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  return aout_channel[pin].value;
}


/////////////////////////////////////////////////////////////////////////////
// This internal function returns the actual output value which can change dynamically
// e.g. depending on slew rate, pitch/pitch range and calibration mode
// \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1) --- NO CHECK FOR RANGE TO IMPROVE RUNTIME
// \return -1 if pin not available
// \return >= 0 if pin available (16bit output value)
/////////////////////////////////////////////////////////////////////////////
static s32 currentValueGet(u8 pin)
{
#if 0
  // no check to improve runtime
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available
#endif

  aout_channel_t *c = (aout_channel_t *)&aout_channel[pin];
  u16 value = c->value;
  if( cali_mode != AOUT_CALI_MODE_OFF && pin == cali_pin )
    value = caliValue(pin);
  else {
    if( aout_config.chn_inverted & (1 << pin) )
      value ^= 0xffff;
  }

  return value;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets the slew rate for an output channel
//!
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \param[in] value the slew rate (0..255 mS)
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinSlewRateSet(u8 pin, u8 value)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  aout_channel[pin].slewrate = value;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the slew rate of an output channel.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \return -1 if pin not available
//! \return >= 0 if pin available (8bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinSlewRateGet(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  return aout_channel[pin].slewrate;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets the pitch range for an output channel
//!
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \param[in] value the pitch range (0..127) - only values between 2..12 are really useful
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinPitchRangeSet(u8 pin, u8 value)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  aout_channel_t *c = (aout_channel_t *)&aout_channel[pin];
  if( c->pitchrange != value ) {
    MIOS32_IRQ_Disable();
    c->pitchrange = value;
    u16 original_value = c->original_value;
    c->original_value ^= 0xffff; // force update
    AOUT_PinSet(pin, original_value);
    MIOS32_IRQ_Enable();
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the pitch range of an output channel.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \return -1 if pin not available
//! \return >= 0 if pin available (8bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinPitchRangeGet(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  return aout_channel[pin].pitchrange;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets the pitch offset for an output channel
//!
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \param[in] value the pitch (-0x8000..+0x7fff)
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinPitchSet(u8 pin, s16 value)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  aout_channel_t *c = (aout_channel_t *)&aout_channel[pin];
  if( c->pitch != value ) {
    MIOS32_IRQ_Disable();
    c->pitch = value;
    u16 original_value = c->original_value;
    c->original_value ^= 0xffff; // force update
    AOUT_PinSet(pin, original_value);
    MIOS32_IRQ_Enable();
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the pitch offset of an output channel.
//! \param[in] pin the pin number (0..AOUT_NUM_CHANNELS-1)
//! \return -1 if pin not available
//! \return >= 0 if pin available (signed 16bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_PinPitchGet(u8 pin)
{
  if( pin >= AOUT_NUM_CHANNELS ) // don't use aout_config.num_channels here, we want to avoid access outside the array
    return -1; // pin not available

  return aout_channel[pin].pitch;
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
//! This function allows to suspend any updates until suspend will be deactived
//!
//! \param[in] suspend if 1: AOUT_Update() has no effect, if 0: module will be
//!            re-initialized via AOUT_IF_Init(0) and AOUT_Update() works again
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_SuspendSet(u8 suspend)
{
  if( suspend )
    suspend_mode = 1;
  else {
    return AOUT_IF_Init(0); // will also clear suspend mode
  }

  return 0; // no error
}

s32 AOUT_SuspendGet(void)
{
  return suspend_mode;
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

  if( suspend_mode )
    return 0; // ignore in suspend mode

  // handle slew rate
  aout_channel_t *c = (aout_channel_t *)&aout_channel[0];
  int pin;
  for(pin=0; pin<AOUT_NUM_CHANNELS; ++pin, ++c) {
    MIOS32_IRQ_Disable();
    s32 inc = c->incrementer;
    if( inc ) {
      s32 new_value = c->value + inc;
      if( (inc > 0 && new_value >= c->target_value) ||
	  (inc < 0 && new_value <= c->target_value) ) {
	new_value = c->target_value;
	c->incrementer = 0;
      }
      c->value = new_value;
      aout_update_req |= 1 << pin;
    }
    MIOS32_IRQ_Enable();
  }


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

	// init SPI again
	// we will do this here, so that other handlers (e.g. AINSER) could use SPI in different modes
	status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit

	// each device has 4 channels
	int chn;
	for(chn=0; chn<4; ++chn) {

	  // check if channel has to be updated for any device
	  if( req & (0x11111111 << chn) ) {
	    // activate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	    // loop through devices (value of last device has to be shifted first)
	    int dev;
	    for(dev=aout_num_devices-1; dev>=0; --dev) {

	      // build command:
	      u8 chn_ix = 4*dev + chn;
	      u16 dac_value = currentValueGet(chn_ix) >> 4; // 16bit -> 12bit

	      // A[10]: channel number, C1=1, C0=1
	      u16 hword = (chn << 14) | (1 << 13) | (1 << 12) | dac_value;

	      // transfer word
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	    }

	    // deactivate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	  }
	}
      } break;

      case AOUT_IF_74HC595: {

	// init SPI again
	// we will do this here, so that other handlers (e.g. AINSER) could use SPI in different modes
	status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit

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
	    u16 dac0_value = currentValueGet(chn_ix+0) >> 8; // 16bit -> 8bit
	    u16 dac1_value = currentValueGet(chn_ix+1) >> 8; // 16bit -> 8bit
	    hword = (dac1_value << 8) | dac0_value;
	  } else {
	    // 12/4 configuration
	    u16 dac0_value = currentValueGet(chn_ix+0) >> 4; // 16bit -> 12bit
	    u16 dac1_value = currentValueGet(chn_ix+1) >> 12; // 16bit -> 4bit
	    hword = (dac1_value << 12) | dac0_value;
	  }

	  // transfer word
	  MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	  MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	}

	// toggle RCLK pin
	MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      } break;

      case AOUT_IF_TLV5630: {

	// init SPI again
	// we will do this here, so that other handlers (e.g. AINSER) could use SPI in different modes
	status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit

	// each device has 8 channels
	int chn;
	for(chn=0; chn<8; ++chn) {

	  // check if channel has to be updated for any device
	  if( req & (0x01010101 << chn) ) {
	    // activate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	    // loop through devices (value of last device has to be shifted first)
	    int dev;
	    for(dev=aout_num_devices-1; dev>=0; --dev) {

	      // build command:
	      u8 chn_ix = 8*dev + chn;
	      u16 dac_value = currentValueGet(chn_ix) >> 4; // 16bit -> 12bit

	      // [15]=0, [14:12] channel number, [11:0] DAC value
	      u16 hword = (chn << 12) | dac_value;

	      // transfer word
	      MIOS32_IRQ_Disable();
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	      MIOS32_IRQ_Enable();
	    }

	    // deactivate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	  }
	}
      } break;

      case AOUT_IF_MCP4922_1:
      case AOUT_IF_MCP4922_2: {
	// init SPI again
	// we will do this here, so that other handlers (e.g. AINSER) could use SPI in different modes
	status |= MIOS32_SPI_TransferModeInit(AOUT_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_16); // ca. 5 MBit

	// only 1 device can be connected to a CS line, and only 2 channels available
	int chn;
	for(chn=0; chn<2; ++chn) {

	  // check if channel has to be updated for any device
	  if( req & (1 << chn) ) {
	    // activate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	    {
	      // build command:
	      u8 chn_ix = chn;
	      u16 dac_value = currentValueGet(chn_ix) >> 4; // 16bit -> 12bit

	      // [15] channel select, [14] buffer enable, [13] gain select, [12] shutdown (low-active), [11:0] DAC value
	      u16 hword = (chn << 15) | (1 << 14) | (1 << 12) | dac_value;
	      if( aout_config.if_type == AOUT_IF_MCP4922_1 ) {
		hword |= (1 << 13); // Gain x1
	      }

	      // transfer word
	      MIOS32_IRQ_Disable();
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	      MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	      MIOS32_IRQ_Enable();
	    }

	    // deactivate chip select
	    MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
	  }
	}
      } break;

      case AOUT_IF_INTDAC: {
	// we have two channels
	int chn;
	for(chn=0; chn<2; ++chn) {

	  // set new value if requested
	  u8 chn_mask = (1 << chn);
	  if( req & chn_mask ) {
	    u16 dac_value = currentValueGet(chn);
	    MIOS32_BOARD_DAC_PinSet(chn, dac_value);
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
	MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

	// loop through devices (value of last device has to be shifted first)
	int dev;
	for(dev=aout_num_devices-1; dev>=0; --dev) {

	  // commands:
	  // UP0=low: A1=0, A0=0, C1=1, C0=0
	  // UP0=high: A1=0, A0=1, C1=1, C0=0
	  u16 a0 = (aout_dig_value & (1 << dev)) ? 1 : 0;
	  u16 hword = (0 << 15) | (a0 << 14) | (1 << 13) | (0 << 12);

	  // transfer word
	  MIOS32_SPI_TransferByte(AOUT_SPI, hword >> 8);
	  MIOS32_SPI_TransferByte(AOUT_SPI, hword & 0xff);
	}

	// deactivate chip select
	MIOS32_SPI_RC_PinSet(AOUT_SPI, AOUT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
      } break;

      case AOUT_IF_74HC595:
      case AOUT_IF_TLV5630:
      case AOUT_IF_MCP4922_1:
      case AOUT_IF_MCP4922_2:
      case AOUT_IF_INTDAC:
	// no digital outputs supported
	break;

      default:
        return -3; // invalid interface selected
    }
  }

  return status ? -4 : 0; // SPI transfer error?
}



/////////////////////////////////////////////////////////////////////////////
// help function which tests an AOUT pin
/////////////////////////////////////////////////////////////////////////////
static s32 AOUT_TestAoutPin(void *_output_function, u8 pin_number, u8 level)
{
  void (*out)(char *format, ...) = _output_function;
  s32 status = 0;

  switch( pin_number ) {
  case 0:
    AOUT_SuspendSet(0);
    out("Module has been re-initialized and can be used again!\n");
    break;

  case 1:
    AOUT_SuspendSet(1);
    out("Setting AOUT:CS pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI_RC_PinSet(2, 0, level ? 1 : 0); // spi, rc_pin, pin_value
#endif
    break;

  case 2:
    AOUT_SuspendSet(1);
    out("Setting AOUT:SI pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI2_MOSI_INIT;
    MIOS32_SPI2_MOSI_SET(level ? 1 : 0);
#endif
    break;

  case 3:
    AOUT_SuspendSet(1);
    out("Setting AOUT:SC pin to ca. %dV - please measure now!\n", level ? MIOS32_SPI2_HIGH_VOLTAGE : 0);
#if !defined(MIOS32_FAMILY_EMULATION)
    MIOS32_SPI2_SCLK_INIT;
    MIOS32_SPI2_SCLK_SET(level ? 1 : 0);
#endif
    break;

  default:
    out("ERROR: unsupported pin #%d", pin_number);
  }

  return status;
}



/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
//static s32 get_on_off(char *word)
//{
//  if( strcmp(word, "on") == 0 || strcmp(word, "1") == 0 )
//    return 1;
//
//  if( strcmp(word, "off") == 0 || strcmp(word, "0") == 0 )
//    return 0;
//
//  return -1;
//}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  testaoutpin:    type this command to get further informations about this testmode.");
  out("  caliaout:       type this command to get further informations about this testmode.");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  int input_len = strlen(input);

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcasecmp(parameter, "testaoutpin") == 0 ) {
      char *arg;
      int pin_number = -1;
      int level = -1;
      
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcasecmp(arg, "cs") == 0 )
	  pin_number = 1;
	else if( strcasecmp(arg, "si") == 0 )
	  pin_number = 2;
	else if( strcasecmp(arg, "sc") == 0 )
	  pin_number = 3;
	else if( strcasecmp(arg, "reset") == 0 ) {
	  pin_number = 0;
	  level = 0; // dummy
	}
      }
      
      if( pin_number < 0 ) {
	out("Please specifiy valid AOUT pin name: cs, si or sc\n");
      }

      if( (arg = strtok_r(NULL, separators, &brkt)) )
	level = get_dec(arg);
	
      if( level != 0 && level != 1 ) {
	out("Please specifiy valid logic level for AOUT pin: 0 or 1\n");
      }

      if( pin_number >= 0 && level >= 0 ) {
	AOUT_TestAoutPin(out, pin_number, level);
      } else {
	out("Following commands are supported:\n");
	out("testaoutpin cs 0  -> sets AOUT:CS to 0.4V");
	out("testaoutpin cs 1  -> sets AOUT:CS to ca. 4V");
	out("testaoutpin si 0  -> sets AOUT:SI to ca. 0.4V");
	out("testaoutpin si 1  -> sets AOUT:SI to ca. 4V");
	out("testaoutpin sc 0  -> sets AOUT:SC to ca. 0.4V");
	out("testaoutpin sc 1  -> sets AOUT:SC to ca. 4V");
	out("testaoutpin reset -> re-initializes AOUT module so that it can be used again.");
      }
      return 1; // command taken

    } else if( strcasecmp(parameter, "caliaout") == 0 ) {
      char *arg;
      int channel = AOUT_CaliPinGet() + 1;
      int mode = -1;
      
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcasecmp(arg, "off") == 0 )
	  mode = AOUT_CALI_MODE_OFF;
	else {
	  if( (channel=get_dec(arg)) < 1 || channel > AOUT_NUM_CHANNELS ) {
	    out("Please specifiy valid channel number (1..%d)\n", AOUT_NUM_CHANNELS);
	  }

	  if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	    if( strcasecmp(arg, "off") == 0 )
	      mode = AOUT_CALI_MODE_OFF;
	    if( strcasecmp(arg, "min") == 0 )
	      mode = AOUT_CALI_MODE_MIN;
	    else if( strcasecmp(arg, "middle") == 0 || strcasecmp(arg, "mid") == 0 )
	      mode = AOUT_CALI_MODE_MIDDLE;
	    else if( strcasecmp(arg, "max") == 0 )
	      mode = AOUT_CALI_MODE_MAX;
	    else if( strcasecmp(arg, "1v") == 0 || strcasecmp(arg, "1") == 0 )
	      mode = AOUT_CALI_MODE_1V;
	    else if( strcasecmp(arg, "2v") == 0 || strcasecmp(arg, "2") == 0 )
	      mode = AOUT_CALI_MODE_2V;
	    else if( strcasecmp(arg, "4v") == 0 || strcasecmp(arg, "4") == 0 )
	      mode = AOUT_CALI_MODE_4V;
	    else if( strcasecmp(arg, "8v") == 0 || strcasecmp(arg, "8") == 0 )
	      mode = AOUT_CALI_MODE_8V;
	  }

	  if( mode < 0 )
	    out("Please specifiy valid calibration mode (off, min, middle, max, 1V, 2V, 4V or 8V)\n");
	}
      }

      if( mode >= 0 ) {
	if( AOUT_CaliModeSet(channel-1, mode) >= 0 ) {
	  if( mode == AOUT_CALI_MODE_OFF )
	    out("Calibration Mode has been disabled!\n");
	  else
	    out("Now you should measure the %s value at channel %d!\n", AOUT_CaliNameGet(mode), channel);
	}
      } else {
	out("Following commands are supported:\n");
	out("caliaout off:              disables calibration");
	out("caliaout <channel> off:    disables calibration as well");
	out("caliaout <channel> min:    sets the given channel (1..%d) to minimum value", AOUT_NUM_CHANNELS);
	out("caliaout <channel> middle: sets the given channel (1..%d) to middle value", AOUT_NUM_CHANNELS);
	out("caliaout <channel> max:    sets the given channel (1..%d) to maximum value", AOUT_NUM_CHANNELS);
	out("caliaout <channel> 1V:     sets the given channel (1..%d) to 1V", AOUT_NUM_CHANNELS);
	out("caliaout <channel> 2V:     sets the given channel (1..%d) to 2V", AOUT_NUM_CHANNELS);
	out("caliaout <channel> 4V:     sets the given channel (1..%d) to 4V", AOUT_NUM_CHANNELS);
	out("caliaout <channel> 8V:     sets the given channel (1..%d) to 8V", AOUT_NUM_CHANNELS);
	if( AOUT_CaliModeGet() == AOUT_CALI_MODE_OFF )
	  out("Current calibration mode: off");
	else
	  out("Current calibration mode: %s at channel #%d", AOUT_CaliNameGet(AOUT_CaliModeGet()), AOUT_CaliPinGet() + 1);
      }

      return 1; // command taken
    }
  }

  // restore input line (replace NUL characters by spaces)
  int i;
  char *input_ptr = input;
  for(i=0; i<input_len; ++i, ++input_ptr)
    if( !*input_ptr )
      *input_ptr = ' ';

  return 0; // command not taken
}


/////////////////////////////////////////////////////////////////////////////
// Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 AOUT_TerminalPrintConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Print AOUT Config: TODO");

  return 0; // no error
}




//! \}
