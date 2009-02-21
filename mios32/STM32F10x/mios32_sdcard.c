// $Id$
//! \defgroup MIOS32_SDCARD
//!
//! SD Card is accessed via SPI1 (J16) or alternatively via SPI2 (J8/9)
//! A bitbanging solution (for using alternative ports) will be provided later.
//!
//! The card has to be supplied with 3.3V!
//!
//! The SDIO peripheral is not used to ensure compatibility with "mid density"
//! devices of the STM32 family, and future derivatives != STM32
//!
//! MIOS32_SDCARD_Init(0) has to be called only once to initialize the GPIO pins,
//! clocks, SPI and DMA
//!
//! MIOS32_SDCARD_PowerOn() should be called to connect with the SD Card. If an error
//! is returned, it can be assumed that no SD Card is connected. The function
//! can be called again (after a certain time) to retry a connection, resp. for
//! an auto-detection during runtime
//!
//! MIOS32_SDCARD_SectorRead/SectorWrite allow to read/write a 512 byte sector.
//!
//! If such an access returns an error, it can be assumed that the SD Card has
//! been disconnected during the transfer.
//!
//! Since DMA is used for serial transfers, Reading/Writing a sector typically
//! takes ca. 500 uS, accordingly the achievable transfer rate is ca. 1 MByte/s
//! (8 MBit/s)
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
#if !defined(MIOS32_DONT_USE_SDCARD)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define SDCMD_GO_IDLE_STATE	0x40
#define SDCMD_GO_IDLE_STATE_CRC	0x95

#define SDCMD_SEND_OP_COND	0x41
#define SDCMD_SEND_OP_COND_CRC	0xf9

#define SDCMD_SEND_STATUS	0x4d
#define SDCMD_SEND_STATUS_CRC	0xaf

#define SDCMD_READ_SINGLE_BLOCK	0x51
#define SDCMD_READ_SINGLE_BLOCK_CRC 0xff

#define SDCMD_WRITE_SINGLE_BLOCK 0x58
#define SDCMD_WRITE_SINGLE_BLOCK_CRC 0xff


/////////////////////////////////////////////////////////////////////////////
//! Initializes SPI pins and peripheral to access MMC/SD Card
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // ensure that fast pin drivers are activated
  MIOS32_SPI_IO_Init(MIOS32_SDCARD_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // init SPI port for slow frequency access (ca. 0.3 MBit/s)
  MIOS32_SPI_TransferModeInit(MIOS32_SDCARD_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_256);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Connects to SD Card
//! \return < 0 if initialisation sequence failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOn(void)
{
  s32 status;
  int i;

  // ensure that chip select line deactivated
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // init SPI port for slow frequency access (ca. 0.3 MBit/s)
  MIOS32_SPI_TransferModeInit(MIOS32_SDCARD_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_256);

  // send 80 clock cycles to start up
  for(i=0; i<10; ++i)
    MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // activate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

  // wait for 1 mS
  MIOS32_DELAY_Wait_uS(1000);

  // send CMD0 to reset the media
  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_GO_IDLE_STATE, 0, SDCMD_GO_IDLE_STATE_CRC)) < 0 )
    return status; // return error code

  // deactivate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // send CMD1 to reset the media - this must be repeated until SD Card is fully initialized
  for(i=0; i<16384; ++i) {
    if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_SEND_OP_COND, 0, SDCMD_SEND_OP_COND_CRC)) < 0 )
      return status; // return error code

    if( status == 0x00 )
      break;
  }

  if( i == 16384 ) {
     // deactivate chip select and return error code
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    return -2; // return error code
  }

  // init SPI port for fast frequency access (ca. 18 MBit/s)
  MIOS32_SPI_TransferModeInit(MIOS32_SDCARD_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_4);

  // deactivate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Disconnects from SD Card
//! \return < 0 if initialisation sequence failed
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOff(void)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! If SD card was previously available: Checks if the SD Card is still 
//! available by sending the STATUS command.<BR>
//! This takes ca. 10 uS
//!
//! If SD card was previously not available: Checks if the SD Card is
//! available by sending the IDLE command at low speed.<BR>
//! This takes ca. 500 uS!<BR>
//! Once we got a positive response, MIOS32_SDCARD_PowerOn() will be 
//! called by this function to initialize the card completely.
//!
//! Example for Connection/Disconnection detection:
//! \code
//! // this function is called each second from a low-priority task
//! // If multiple tasks are accessing the SD card, add a semaphore/mutex
//! //  to avoid IO access collisions with other tasks!
//! u8 sdcard_available;
//! s32 SEQ_FILE_CheckSDCard(void)
//! {
//!   // check if SD card is available
//!   // High speed access if the card was previously available
//!   u8 prev_sdcard_available = sdcard_available;
//!   sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);
//! 
//!   if( sdcard_available && !prev_sdcard_available ) {
//!     // SD Card has been connected
//! 
//!     // now it's possible to read/write sectors
//! 
//!   } else if( !sdcard_available && prev_sdcard_available ) {
//!     // SD Card has been disconnected
//! 
//!     // here you can notify your application about this state
//!   }
//! 
//!   return 0; // no error
//! }
//! \endcode
//! \param[in] was_available should only be set if the SD card was previously available
//! \return 0 if no response from SD Card
//! \return 1 if SD card is accessible
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_CheckAvailable(u8 was_available)
{
  if( was_available ) {
    // send STATUS command to check if media is available
    if( MIOS32_SDCARD_SendSDCCmd(SDCMD_SEND_STATUS, 0, SDCMD_SEND_STATUS_CRC) < 0 )
      return 0; // SD card not available anymore

    // deactivate chip select
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
  } else {
    // ensure that SPI interface is clocked at low speed
    MIOS32_SPI_TransferModeInit(MIOS32_SDCARD_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_256);

    // send CMD0 to reset the media
    if( MIOS32_SDCARD_SendSDCCmd(SDCMD_GO_IDLE_STATE, 0, SDCMD_GO_IDLE_STATE_CRC) < 0 )
      return 0; // SD card still not available

    // deactivate chip select
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

    // run power-on sequence
    if( MIOS32_SDCARD_PowerOn() < 0 )
      return 0; // SD card not available anymore
  }
  
  return 1; // SD card available
}


/////////////////////////////////////////////////////////////////////////////
//! Sends command to SD card
//! \param[in] cmd SD card command
//! \param[in] addr 32bit address
//! \param[in] crc precalculated CRC
//! \return >= 0x00 if command has been sent successfully (contains received byte)
//! \return -1 if no response from SD Card (timeout)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SendSDCCmd(u8 cmd, u32 addr, u8 crc)
{
  int i;
  u8 ret;

  // activate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

  // transfer to card
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, cmd);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, (addr >> 24) & 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, (addr >> 16) & 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, (addr >>  8) & 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, (addr >>  0) & 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, crc);

  u8 timeout = 0;

  if( cmd == SDCMD_SEND_STATUS ) {
    // one dummy read
    MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
    // read two bytes (only last value will be returned)
    ret=MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
    ret=MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

    // all-one read: we expect that SD card is not connected: notify timeout!
    if( ret == 0xff )
      timeout = 1;
  } else {
    // wait for response
    for(i=0; i<8; ++i) {
      if( (ret=MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff)) != 0xff )
	break;
    }
    if( i == 8 )
      timeout = 1;
  }

  // required clocking (see spec)
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // deactivate chip-select on timeout, and return error code
  if( timeout ) {
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    return -1;
  }

  // else return received value - don't deactivate chip select (for continuous access)
  return ret;
}


/////////////////////////////////////////////////////////////////////////////
//! Reads 512 bytes from selected sector
//! \param[in] sector 32bit sector
//! \param[in] *buffer pointer to 512 byte buffer
//! \return 0 if whole sector has been successfully read
//! \return -error if error occured during read operation:<BR>
//! <UL>
//!   <LI>Bit 0              - In idle state if 1
//!   <LI>Bit 1              - Erase Reset if 1
//!   <LI>Bit 2              - Illgal Command if 1
//!   <LI>Bit 3              - Com CRC Error if 1
//!   <LI>Bit 4              - Erase Sequence Error if 1
//!   <LI>Bit 5              - Address Error if 1
//!   <LI>Bit 6              - Parameter Error if 1
//!   <LI>Bit 7              - Not used, always '0'
//! </UL>
//! \return -256 if timeout during command has been sent
//! \return -257 if timeout while waiting for start token
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SectorRead(u32 sector, u8 *buffer)
{
  s32 status;
  int i;

  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_READ_SINGLE_BLOCK, sector << 9, SDCMD_READ_SINGLE_BLOCK_CRC)) )
    return (status < 0) ? -256 : status; // return timeout indicator or error flags

  // wait for start token of the data block
  for(i=0; i<65536; ++i) { // TODO: check if sufficient
    u8 ret = MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
    if( ret != 0xff )
      break;
  }
  if( i == 65536 ) {
    // deactivate chip select and return error code
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    return -257;
  }

  // read 512 bytes via DMA
  MIOS32_SPI_TransferBlock(MIOS32_SDCARD_SPI, NULL, buffer, 512, NULL);

  // read (and ignore) CRC
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // required for clocking (see spec)
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // deactivate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Writes 512 bytes into selected sector
//! \param[in] sector 32bit sector
//! \param[in] *buffer pointer to 512 byte buffer
//! \return 0 if whole sector has been successfully read
//! \return -error if error occured during read operation:<BR>
//! <UL>
//!   <LI>Bit 0              - In idle state if 1
//!   <LI>Bit 1              - Erase Reset if 1
//!   <LI>Bit 2              - Illgal Command if 1
//!   <LI>Bit 3              - Com CRC Error if 1
//!   <LI>Bit 4              - Erase Sequence Error if 1
//!   <LI>Bit 5              - Address Error if 1
//!   <LI>Bit 6              - Parameter Error if 1
//!   <LI>Bit 7              - Not used, always '0'
//! </UL>
//! \return -256 if timeout during command has been sent
//! \return -257 if write operation not accepted
//! \return -258 if timeout during write operation
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SectorWrite(u32 sector, u8 *buffer)
{
  s32 status;
  int i;

  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_WRITE_SINGLE_BLOCK, sector << 9, SDCMD_WRITE_SINGLE_BLOCK_CRC)) )
    return (status < 0) ? -256 : status; // return timeout indicator or error flags

  // send start token
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xfe);

  // send 512 bytes of data via DMA
  MIOS32_SPI_TransferBlock(MIOS32_SDCARD_SPI, buffer, NULL, 512, NULL);

  // send CRC
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // read response
  u8 response = MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
  if( (response & 0x0f) != 0x5 ) {
    // deactivate chip select and return error code
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    return -257;
  }

  // wait for write completion
  for(i=0; i<32*65536; ++i) { // TODO: check if sufficient
    u8 ret = MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);
    if( ret != 0x00 )
      break;
  }
  if( i == 32*65536 ) {
    // deactivate chip select and return error code
    MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
    return -258;
  }

  // required for clocking (see spec)
  MIOS32_SPI_TransferByte(MIOS32_SDCARD_SPI, 0xff);

  // deactivate chip select
  MIOS32_SPI_RC_PinSet(MIOS32_SDCARD_SPI, MIOS32_SDCARD_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  return 0; // no error
}


//! \}

#endif /* MIOS32_DONT_USE_SDCARD */

