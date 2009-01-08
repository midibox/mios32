//
//  MIOS32_SDCARD_Wrapper.m
//
//  Intention: later we will mount a file in FAT32 format and read/write it via SD card functions
//  Advantage: we can use (and debug) the original DOSFS functions in emulation
//
//  Created by Thorsten Klose on 06.01.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_SDCARD_Wrapper.h"

#include <mios32.h>

@implementation MIOS32_SDCARD_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes SPI pins and peripheral to access MMC/SD Card
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_Init(u32 mode)
{
	// here we will open the filesystem file later
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Connects to SD Card
//! \return < 0 if initialisation sequence failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOn(void)
{
	// Nothing to do
	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Disconnects from SD Card
//! \return < 0 if initialisation sequence failed
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOff(void)
{
	// nothing to do
	return 0; // no error
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
	// nothing to do
	return 0;
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
	// TODO
	// currently we return all-1
	int i;
	for(i=0; i<512; ++i)
		buffer[i] = 0xff;
	
	return 0;
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
	// TODO
	return 0;
}


@end
