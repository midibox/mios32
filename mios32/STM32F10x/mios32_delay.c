// $Id$
//! \defgroup MIOS32_DELAY
//!
//! Delay functions for MIOS32
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
#if !defined(MIOS32_DONT_USE_DELAY)

// timers clocked at CPU clock
#define TIM_PERIPHERAL_FRQ MIOS32_SYS_CPU_FREQUENCY

/////////////////////////////////////////////////////////////////////////////
//! Initializes the Timer used by MIOS32_DELAY functions<BR>
//! This function has to be executed before wait functions are used
//! (already done in main.c of the programming model)
//!
//! Currently TIM1 is allocated by MIOS32_DELAY functions - if this clashes with
//! your application, just switch to another timer by overriding 
//! MIOS32_DELAY_TIMER and MIOS32_DELAY_TIMER_RCC in your mios32_config.h file
//!
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DELAY_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // enable timer clock
  if( MIOS32_DELAY_TIMER_RCC == RCC_APB2Periph_TIM1 || MIOS32_DELAY_TIMER_RCC == RCC_APB2Periph_TIM8 )
    RCC_APB2PeriphClockCmd(MIOS32_DELAY_TIMER_RCC, ENABLE);
  else
    RCC_APB1PeriphClockCmd(MIOS32_DELAY_TIMER_RCC, ENABLE);

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 65535; // maximum value
  TIM_TimeBaseStructure.TIM_Prescaler = (TIM_PERIPHERAL_FRQ/1000000)-1; // for 1 uS accuracy
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(MIOS32_DELAY_TIMER, &TIM_TimeBaseStructure);

  // enable counter
  TIM_Cmd(MIOS32_DELAY_TIMER, ENABLE);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Waits for a specific number of uS<BR>
//! Example:<BR>
//! \code
//!   // wait for 500 uS
//!   MIOS32_DELAY_Wait_uS(500);
//! \endcode
//! \param[in] uS delay (1..65535 microseconds)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DELAY_Wait_uS(u16 uS)
{
  u16 start = MIOS32_DELAY_TIMER->CNT;

  // note that this even works on 16bit counter wrap-arounds
  while( (u16)(MIOS32_DELAY_TIMER->CNT - start) <= uS );

  return 0; // no error
}

//! \}

#endif /* MIOS32_DONT_USE_DELAY */
