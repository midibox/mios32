//
//  MIOS32_TIMER_Wrapper.m
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_TIMER_Wrapper.h"

#include <mios32.h>

@implementation MIOS32_TIMER_Wrapper

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_TIMERS 3

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static NSObject *_self;

static void (*timer_callback[NUM_TIMERS])(void);
static NSTimeInterval timer_period[NUM_TIMERS];


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;

	// set initial period and install timer threads
	int i;
	NSNumber *timerIx[NUM_TIMERS];
	for(i=0; i<NUM_TIMERS; ++i) {
		timer_period[i] = 0.001;
		
		timerIx[i] = [NSNumber numberWithInt:i]; 
		[NSThread detachNewThreadSelector:@selector(timerThread:) toTarget:self withObject:timerIx[i]];
	}
}



/////////////////////////////////////////////////////////////////////////////
// Initialize a timer
// IN: timer (0..2)
//     Timer allocation on STM32: 0=TIM2, 1=TIM3, 2=TIM5
//     period in uS accuracy (1..65536)
//     irq_handler (function-name)
//     irq_priority: one of these values
//       MIOS32_IRQ_PRIO_LOW      // lower than RTOS
//       MIOS32_IRQ_PRIO_MID      // higher than RTOS
//       MIOS32_IRQ_PRIO_HIGH     // same like SRIO, AIN, etc...
//       MIOS32_IRQ_PRIO_HIGHEST  // higher than SRIO, AIN, etc...
// OUT: returns 0 if initialisation passed
//      -1 if invalid timer number
//      -2 if invalid period
// EXAMPLE:
//
//   // initialize timer for 1000 uS (= 1 mS) period
//   MIOS32_TIMER_Init(0, 1000, MyTimer, MIOS32_IRQ_PRIO_MID);
//
// this will call following function periodically:
//
// void MyTimer(void)
// {
//    // your code
// }
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_Init(u8 timer, u32 period, void *_irq_handler, u8 irq_priority)
{
	// check if valid timer
	if( timer >= NUM_TIMERS )
		return -1; // invalid timer selected

	// check if valid period
	if( period < 1 || period >= 65537 )
		return -2;

	// kill old thread (if active from previous configuration)
	// ...

	// copy callback function
	timer_callback[timer] = _irq_handler;

	// convert period
	timer_period[timer] = (NSTimeInterval)(period/1E6);
	
	// install new thread
	// ...

	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Re-Initialize a timer with given period
// IN: timer (0..2)
//     period in uS accuracy (1..65536)
// OUT: returns 0 if initialisation passed
//      -1 if invalid timer number
//      -2 if invalid period
// EXAMPLE:
//
//   // change timer period to 2 mS
//   MIOS32_TIMER_ReInit(0, 2000);
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_ReInit(u8 timer, u32 period)
{
	// check if valid timer
	if( timer >= NUM_TIMERS )
		return -1; // invalid timer selected

	// check if valid period
	if( period < 1 || period >= 65537 )
		return -2;

	// new time base configuration
	timer_period[timer] = (NSTimeInterval)(period/1E6);

	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// De-Initialize a timer
// IN: timer (0..2)
// OUT: returns 0 if timer has been disabled
//      -1 if invalid timer number
// EXAMPLE:
//
//   // disable timer
//   MIOS32_TIMER_DeInit(0);
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_DeInit(u8 timer)
{
	// check if valid timer
	if( timer >= NUM_TIMERS )
		return -1; // invalid timer selected

	// deinitialize thread
	timer_callback[timer] = NULL;

	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// "Interrupt" handlers (these are multiple threads which have been initialized in awakeFromNib)
/////////////////////////////////////////////////////////////////////////////
- (void)timerThread:(id)timerNumber
{
	int timer = [timerNumber intValue];
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSDate *now = [NSDate date];
	while (YES) {
		MIOS32_IRQ_Disable(); // normaly timer would run with higher priority than FreeRTOS tasks...
		if( timer_callback[timer] != NULL )
			timer_callback[timer]();
		MIOS32_IRQ_Enable();
		
		now = [now initWithTimeInterval:timer_period[timer] sinceDate:now];
        [NSThread sleepUntilDate:now];
    }
	[pool release];
	[NSThread exit];
}

@end
