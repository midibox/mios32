// $Id$
//! \defgroup MAX72XX
//!
//! MAX72xx module driver
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "max72xx.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 num_used_chains = MAX72XX_NUM_CHAINS;
static u8 chain_enable_mask;
static u8 num_used_devices_per_chain[MAX72XX_NUM_CHAINS];

static u8 max72xx_digits[MAX72XX_NUM_CHAINS][MAX72XX_NUM_DEVICES_PER_CHAIN][8];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MAX72XX_SetCs(u8 chain, u8 value);


/////////////////////////////////////////////////////////////////////////////
//! Initializes MAX72xx driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_Init(u32 mode)
{
  s32 status = 0;
  int chain, device;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MAX72XX_SPI_OUTPUTS_OD
  // pins in open drain mode (to pull-up the outputs to 5V)
  status |= MIOS32_SPI_IO_Init(MAX72XX_SPI, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
  // pins in push-poll mode (3.3V output voltage)
  status |= MIOS32_SPI_IO_Init(MAX72XX_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif

  // SPI Port will be initialized in MAX72XX_Update()

  num_used_chains = MAX72XX_NUM_CHAINS;
#if MAX72XX_NUM_MODULES > 8
# error "If more than 8 MAX72XX_NUM_CHAINS should be supported, the chain_enable_mask variable type has to be changed from u8 to u16 (up to 16) or u32 (up to 32)"
#endif

  for(chain=0; chain<MAX72XX_NUM_CHAINS; ++chain) {
    num_used_devices_per_chain[chain] = MAX72XX_NUM_DEVICES_PER_CHAIN;

    // ensure that CS is deactivated
    MAX72XX_SetCs(chain, 1);

    MAX72XX_EnabledSet(chain, 1);
    MAX72XX_NumDevicesPerChainSet(chain, MAX72XX_NUM_DEVICES_PER_CHAIN);

    // clear all digits
    for(device=0; device<MAX72XX_NUM_DEVICES_PER_CHAIN; ++device) {
      int i;
      for(i=0; i<8; ++i) {
	max72xx_digits[chain][device][i] = 0;
      }
    }

    // enter normal operation mode
    MAX72XX_WriteAllRegs(chain, MAX72XX_REG_SHUTDOWN, 0x01);

    // set decode mode to 0 (no decoding)
    MAX72XX_WriteAllRegs(chain, MAX72XX_REG_DECODE_MODE, 0x00);

    // set maximum intensity
    MAX72XX_WriteAllRegs(chain, MAX72XX_REG_INTENSITY, 0x0f);

    // scan all digits
    MAX72XX_WriteAllRegs(chain, MAX72XX_REG_SCAN_LIMIT, 0x07);

    // ensure that display test mode disabled
    MAX72XX_WriteAllRegs(chain, MAX72XX_REG_TESTMODE, 0x00);

    // update the digits
    MAX72XX_UpdateAllDigits(chain);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! \return the number of chains which are scanned
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_NumChainsGet(void)
{
  return num_used_chains;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the number of chains which should be scanned
//! \return < 0 on error (e.g. unsupported number of chains)
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_NumChainsSet(u8 num_chains)
{
  if( num_chains >= MAX72XX_NUM_CHAINS )
    return -1;

  num_used_chains = num_chains;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \return the enable mask for chains which should be scanned
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_EnabledGet(u8 chain)
{
  if( chain >= MAX72XX_NUM_CHAINS )
    return 0;

  return (chain_enable_mask & (1 << chain)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the enable mask for chains which should be scanned
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_EnabledSet(u8 chain, u8 enabled)
{
  if( chain >= MAX72XX_NUM_CHAINS )
    return -1; // invalid chain

  if( enabled )
    chain_enable_mask |= (1 << chain);
  else
    chain_enable_mask &= ~(1 << chain);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return the number of devices per chain which are scanned
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_NumDevicesPerChainGet(u8 chain)
{
  if( chain >= MAX72XX_NUM_CHAINS )
    return 0; // invalid chain (return 0 pins)

  return num_used_devices_per_chain[chain];
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the number of devices per chain which should be scanned
//! \return < 0 on error (e.g. unsupported number of devices)
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_NumDevicesPerChainSet(u8 chain, u8 num_devices)
{
  if( chain >= MAX72XX_NUM_CHAINS )
    return -1; // invalid chain

  if( num_devices > MAX72XX_NUM_DEVICES_PER_CHAIN )
    return -2;

  num_used_devices_per_chain[chain] = num_devices;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sets a digit of the given chain and device
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_DigitSet(u8 chain, u8 device, u8 digit, u8 value)
{
  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  if( device >= num_used_devices_per_chain[chain] )
    return -2; // invalid device

  if( digit >= 8 )
    return -3; // invalid digit

  max72xx_digits[chain][device][digit] = value;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Sets all digits of the given chain and device
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_AllDigitsSet(u8 chain, u8 device, u8 *values)
{
  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  if( device >= num_used_devices_per_chain[chain] )
    return -2; // invalid device

  u8 *ptr = (u8 *)&max72xx_digits[chain][device];
  int i;
  for(i=0; i<8; ++i) {
    *ptr++ = *values++;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns a digit of the given chain and device
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_DigitGet(u8 chain, u8 device, u8 digit)
{
  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  if( device >= num_used_devices_per_chain[chain] )
    return -2; // invalid device

  if( digit >= 8 )
    return -3; // invalid digit

  return max72xx_digits[chain][device][digit];
}


/////////////////////////////////////////////////////////////////////////////
//! Load a 16bit value into a selected MAX72XX device of the given chain
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_WriteReg(u8 chain, u8 device, u8 reg, u8 value)
{
  s32 status = 0;

  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  if( device >= num_used_devices_per_chain[chain] )
    return -2; // invalid device

  // init SPI port for fast frequency access
  // we will do this here, so that other handlers (e.g. AOUT) could use SPI in different modes
  // Maxmimum allowed SCLK is 10 MHz according to datasheet
  // We select prescaler 32 @120 MHz (-> ca. 250 nS period)
  status |= MIOS32_SPI_TransferModeInit(MAX72XX_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_32);

  // activate chip select
  status |= MAX72XX_SetCs(chain, 0);

  // shift data
  int i;
  for(i=num_used_devices_per_chain[chain]-1; i>=0; --i) {
    if( i == device ) {
      status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, reg);
      status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, value);
    } else {
      status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, MAX72XX_REG_NOP);
      status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, 0x00);
    }
  }

  // deactivate chip select (resp. load shifted values)
  status |= MAX72XX_SetCs(chain, 1);

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Load a 8bit value into all MAX72XX devices of the given chain
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_WriteAllRegs(u8 chain, u8 reg, u8 value)
{
  s32 status = 0;

  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  // init SPI port for fast frequency access
  // we will do this here, so that other handlers (e.g. AOUT) could use SPI in different modes
  // Maxmimum allowed SCLK is 10 MHz according to datasheet
  // We select prescaler 32 @120 MHz (-> ca. 250 nS period)
  status |= MIOS32_SPI_TransferModeInit(MAX72XX_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_32);

  // activate chip select
  status |= MAX72XX_SetCs(chain, 0);

  // shift data
  int i;
  for(i=num_used_devices_per_chain[chain]-1; i>=0; --i) {
    status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, reg);
    status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, value);
  }

  // deactivate chip select (resp. load shifted values)
  status |= MAX72XX_SetCs(chain, 1);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Updates a digit (0..7) of the given chain
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_UpdateDigit(u8 chain, u8 digit)
{
  s32 status = 0;

  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  if( digit >= 8 )
    return -2; // invalid digit

  // init SPI port for fast frequency access
  // we will do this here, so that other handlers (e.g. AOUT) could use SPI in different modes
  // Maxmimum allowed SCLK is 10 MHz according to datasheet
  // We select prescaler 32 @120 MHz (-> ca. 250 nS period)
  status |= MIOS32_SPI_TransferModeInit(MAX72XX_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_32);

  // activate chip select
  status |= MAX72XX_SetCs(chain, 0);

  // shift data
  int i;
  for(i=num_used_devices_per_chain[chain]-1; i>=0; --i) {
    status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, MAX72XX_REG_DIGIT0 + digit);
    status |= MIOS32_SPI_TransferByte(MAX72XX_SPI, max72xx_digits[chain][i][digit]);
  }

  // deactivate chip select (resp. load shifted values)
  status |= MAX72XX_SetCs(chain, 1);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Updates all digits of the given chain
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_UpdateAllDigits(u8 chain)
{
  s32 status = 0;

  if( chain >= num_used_chains || !(chain_enable_mask & (1 << chain)) )
    return -1; // invalid chain

  int digit;
  for(digit=0; digit<8; ++digit) {
    status |= MAX72XX_UpdateDigit(chain, digit);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Updates all chains
/////////////////////////////////////////////////////////////////////////////
s32 MAX72XX_UpdateAllChains(void)
{
  s32 status = 0;

  int chain;
  for(chain=0; chain<num_used_chains; ++chain) {
    status |= MAX72XX_UpdateAllDigits(chain);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Internal function to set CS line depending on chain
/////////////////////////////////////////////////////////////////////////////
static s32 MAX72XX_SetCs(u8 chain, u8 value)
{
  switch( chain ) {
  case 0: return MIOS32_SPI_RC_PinSet(MAX72XX_SPI, MAX72XX_SPI_RC_PIN_CHAIN1, value); // spi, rc_pin, pin_value
  case 1: return MIOS32_SPI_RC_PinSet(MAX72XX_SPI, MAX72XX_SPI_RC_PIN_CHAIN2, value); // spi, rc_pin, pin_value

#if MAX72XX_NUM_CHAINS > 2
# error "CS Line for more than 2 chains not prepared yet - please enhance here!"
#endif
  }


  return -1;
}


//! \}
