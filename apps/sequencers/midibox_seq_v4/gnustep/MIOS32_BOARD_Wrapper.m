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


@end
