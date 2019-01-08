//
//  MIOS32_SPI_Wrapper.m
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_SPI_Wrapper.h"

#include <mios32.h>


@implementation MIOS32_SPI_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


//////////////////////////////////////////////////////////////////////////////
// Stubs for SPI specific functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_Init(u32 mode)
{
	return -1; // not implemented
}

s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver)
{
	return -1; // not implemented
}

s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler)
{
	return -1; // not implemented
}


s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value)
{
	return -1; // not implemented
}

s32 MIOS32_SPI_TransferByte(u8 spi, u8 b)
{
	return -1; // not implemented
}

s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback)
{
	return -1; // not implemented
}

@end
