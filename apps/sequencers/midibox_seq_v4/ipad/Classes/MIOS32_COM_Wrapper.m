//
//  MIOS32_COM_Wrapper.m
//
//  Created by Thorsten Klose on 06.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_COM_Wrapper.h"

#include <mios32.h>


@implementation MIOS32_COM_Wrapper

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
// COM functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendChar(mios32_com_port_t port, char c)
{
	// empty stub - no COM terminal implemented yet
	return 0; // no error
}

@end
