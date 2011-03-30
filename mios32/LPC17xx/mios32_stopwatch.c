// $Id$
//! \defgroup MIOS32_STOPWATCH
//!
//! Stopwatch functions for MIOS32
//!
//! Allows to measure delays, which is especially useful for measuring the
//! performance of a function, but also for user relevant features (e.g. Tap Tempo)
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_STOPWATCH)

// timers clocked at CCLK/4 MHz
#define TIM_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define STOPWATCH_TIMER_BASE     LPC_TIM3


/////////////////////////////////////////////////////////////////////////////
//! Initializes the 16bit stopwatch timer with the desired resolution:
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
//! \note: this function uses TIM4 on the STM32 chip
//! \param[in] resolution 1, 10, 100 or 1000
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Init(u32 resolution)
{
  // time base configuration
  STOPWATCH_TIMER_BASE->CTCR = 0x00;               // timer mode
  STOPWATCH_TIMER_BASE->PR = ((TIM_PERIPHERAL_FRQ/1000000) * resolution)-1;  // <resolution> uS accuracy @ CCLK/4 MHz Peripheral Clock
  STOPWATCH_TIMER_BASE->MR0 = 0xffffffff;          // interrupt event on overrun
  STOPWATCH_TIMER_BASE->MCR = (1 << 1) | (1 << 0); // generate event on match, reset counter on match
  STOPWATCH_TIMER_BASE->IR = ~0;                   // clear all interrupts

  // enable timer
  STOPWATCH_TIMER_BASE->TCR = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Resets the stopwatch
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Reset(void)
{
  STOPWATCH_TIMER_BASE->TCR |= (1 << 1);
  STOPWATCH_TIMER_BASE->TCR &= ~(1 << 1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns current value of stopwatch
//! \return 1..65535: valid stopwatch value
//! \return 0xffffffff: counter overrun
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_STOPWATCH_ValueGet(void)
{
  u32 value = STOPWATCH_TIMER_BASE->TC;

  if( STOPWATCH_TIMER_BASE->IR & (1 << 0) )
    value = 0xffffffff;

  return value;
}

#endif /* MIOS32_DONT_USE_STOPWATCH */
