//
//  MIOS32_IRQ_Wrapper.m
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_IRQ_Wrapper.h"

#include <mios32.h>

@implementation MIOS32_IRQ_Wrapper

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
// stubs for MIOS32 interrupt disable/enable functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Disable(void)
{
	return 0; // no error
}

s32 MIOS32_IRQ_Enable(void)
{
	return 0; // no error
}


@end
