//
//  MIOS32_ENC_Wrapper.m
//
//  Created by Thorsten Klose on 06.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_ENC_Wrapper.h"

#include <mios32.h>


@implementation MIOS32_ENC_Wrapper

//////////////////////////////////////////////////////////////////////////////
// local variables to bridge objects to C functions
//////////////////////////////////////////////////////////////////////////////
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


//////////////////////////////////////////////////////////////////////////////
// Stub for encoder configuration
//////////////////////////////////////////////////////////////////////////////
mios32_enc_config_t MIOS32_ENC_ConfigGet(u32 encoder)
{
	const mios32_enc_config_t dummy = { .cfg.type=DISABLED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=0, .cfg.pos=0 };
    return dummy;
}

s32 MIOS32_ENC_ConfigSet(u32 encoder, mios32_enc_config_t config)
{
	return 0; // no error
}



// does an emulation really make sense here???
// encoder events are current directly forwarded to APP_ENC_NotifyChange by SEQEncoder objects

@end
