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
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define STOPWATCH_TIMER_BASE                 TIM6
#define STOPWATCH_TIMER_RCC   RCC_APB1Periph_TIM6

// timers clocked at CPU clock
#define TIM_PERIPHERAL_FRQ MIOS32_SYS_CPU_FREQUENCY


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
//! \note: this function uses TIM6 of the STM32 chip
//! \param[in] resolution 1, 10, 100 or 1000
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Init(u32 resolution)
{
  // enable timer clock
  if( STOPWATCH_TIMER_BASE == TIM1 || STOPWATCH_TIMER_BASE == TIM8 )
    RCC_APB2PeriphClockCmd(STOPWATCH_TIMER_RCC, ENABLE);
  else
    RCC_APB1PeriphClockCmd(STOPWATCH_TIMER_RCC, ENABLE);

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 0xffff; // max period
  TIM_TimeBaseStructure.TIM_Prescaler = ((TIM_PERIPHERAL_FRQ/1000000) * resolution)-1; // <resolution> uS accuracy
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(STOPWATCH_TIMER_BASE, &TIM_TimeBaseStructure);

  // enable interrupt request
  TIM_ITConfig(STOPWATCH_TIMER_BASE, TIM_IT_Update, ENABLE);

  // start counter
  TIM_Cmd(STOPWATCH_TIMER_BASE, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Resets the stopwatch
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_STOPWATCH_Reset(void)
{
  // reset counter
  STOPWATCH_TIMER_BASE->CNT = 1; // set to 1 instead of 0 to avoid new IRQ request
  TIM_ClearITPendingBit(STOPWATCH_TIMER_BASE, TIM_IT_Update);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns current value of stopwatch
//! \return 1..65535: valid stopwatch value
//! \return 0xffffffff: counter overrun
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_STOPWATCH_ValueGet(void)
{
  u32 value = STOPWATCH_TIMER_BASE->CNT;

  if( TIM_GetITStatus(STOPWATCH_TIMER_BASE, TIM_IT_Update) != RESET )
    value = 0xffffffff;

  return value;
}

#endif /* MIOS32_DONT_USE_STOPWATCH */
