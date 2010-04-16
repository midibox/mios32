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

static NSRecursiveLock *MIOS32_IRQ_Lock;

//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
	MIOS32_IRQ_Lock = [[NSRecursiveLock alloc] init];
}


//////////////////////////////////////////////////////////////////////////////
// stubs for MIOS32 interrupt disable/enable functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IRQ_Disable(void)
{
	[MIOS32_IRQ_Lock lock];
	return 0; // no error
}

s32 MIOS32_IRQ_Enable(void)
{
	[MIOS32_IRQ_Lock unlock];
	return 0; // no error
}


@end
