//
//  MIOS32_BOARD_Wrapper.m
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_BOARD_Wrapper.h"

#include <mios32.h>


@implementation MIOS32_BOARD_Wrapper

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
// Stubs for Board specific functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_Init(u32 mode)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_LED_Init(u32 leds)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value)
{
	return -1; // not implemented
}

u32 MIOS32_BOARD_LED_Get(void)
{
	return 0; // not implemented, return all-zero
}

s32 MIOS32_BOARD_J5_PinInit(u8 pin, mios32_board_pin_mode_t mode)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_J5_Set(u16 value)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_J5_PinSet(u8 pin, u8 value)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_J5_Get(void)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_J5_PinGet(u8 pin)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_DAC_PinInit(u8 chn, u8 enable)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_DAC_PinSet(u8 chn, u16 value)
{
	return -1; // not implemented
}


@end
