//
//  MIOS32_STOPWATCH_Wrapper.m
//
//  Created by Thorsten Klose on 10.01.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_STOPWATCH_Wrapper.h"

#include <mios32.h>

@implementation MIOS32_STOPWATCH_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;

static NSDate *capturedDate;
static u32 resolution;

//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes the timer with the desired resolution:
//! <UL>
//!  <LI>1: 1 uS resolution, time measurement possible in the range of 0.001mS .. 65.535 mS
//!  <LI>10: 10 uS resolution: 0.01 mS .. 655.35 mS
//!  <LI>100: 100 uS resolution: 0.1 mS .. 6.5535 seconds
//!  <LI>1000: 1 mS resolution: 1 mS .. 65.535 seconds
//!  <LI>other values should not be used!
//! <UL>
//!
//! Example:<BR>
//! \code
//!   // initialize the stopwatch for 100 uS resolution
//!   // (only has to be done once, e.g. in APP_Init())
//!   MIOS32_STOPWATCH_Init(100);
//!
//!   // reset stopwatch
//!   MIOS32_STOPWATCH_Reset();
//!
//!   // execute function
//!   MyFunction();
//!
//!   // send execution time via DEFAULT COM interface
//!   u32 delay = MIOS32_STOPWATCH_ValueGet();
//!   printf("Execution time of MyFunction: ");
//!   if( delay == 0xffffffff )
//!     printf("Overrun!\n\r");
//!   else
//!     printf("%d.%d mS\n\r", delay/10, delay%10);
//! \endcode
//! \param[in] resolution 1, 10, 100 or 1000
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Init(u32 _resolution)
{
  resolution = _resolution;
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Resets the stopwatch
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Reset(void)
{
  capturedDate = [NSDate date];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns current value of stopwatch
//! \return 1..65535: valid stopwatch value
//! \return 0xffffffff: counter overrun
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_STOPWATCH_ValueGet(void)
{
  NSTimeInterval diff = [capturedDate timeIntervalSinceNow];  
//  NSLog([NSString stringWithFormat:@"Elapsed time: %f", -diff]);
  return (u16)(-diff*(1E6/resolution));
}

@end
