// $Id$
//! \defgroup MIOS32_TIMER
//!
//! Timer functions for MIOS32
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
#if !defined(MIOS32_DONT_USE_TIMER)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_TIMERS 3

#define TIMER0_BASE                 TIM2
#define TIMER0_RCC   RCC_APB1Periph_TIM2
#define TIMER0_IRQ                  TIM2_IRQn
#define TIMER0_IRQ_HANDLER     void TIM2_IRQHandler(void)

#define TIMER1_BASE                 TIM3
#define TIMER1_RCC   RCC_APB1Periph_TIM3
#define TIMER1_IRQ                  TIM3_IRQn
#define TIMER1_IRQ_HANDLER     void TIM3_IRQHandler(void)

#define TIMER2_BASE                 TIM5
#define TIMER2_RCC   RCC_APB1Periph_TIM5
#define TIMER2_IRQ                  TIM5_IRQn
#define TIMER2_IRQ_HANDLER     void TIM5_IRQHandler(void)


// timers clocked at CPU clock
#define TIM_PERIPHERAL_FRQ MIOS32_SYS_CPU_FREQUENCY


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static TIM_TypeDef *timer_base[NUM_TIMERS] = { TIMER0_BASE, TIMER1_BASE, TIMER2_BASE };
static u32 rcc[NUM_TIMERS] = { TIMER0_RCC, TIMER1_RCC, TIMER2_RCC };
static const u32 timer_irq_chn[NUM_TIMERS] = { TIMER0_IRQ, TIMER1_IRQ, TIMER2_IRQ };
static void (*timer_callback[NUM_TIMERS])(void);


/////////////////////////////////////////////////////////////////////////////
//! Initialize a timer
//! \param[in] timer (0..2)<BR>
//!     Timer allocation on STM32: 0=TIM2, 1=TIM3, 2=TIM5
//! \param[in] period in uS accuracy (1..65536)
//! \param[in] _irq_handler (function name)
//! \param[in] irq_priority: one of these values:
//! <UL>
//!   <LI>MIOS32_IRQ_PRIO_LOW      // lower than RTOS
//!   <LI>MIOS32_IRQ_PRIO_MID      // higher than RTOS
//!   <LI>MIOS32_IRQ_PRIO_HIGH     // same like SRIO, AIN, etc...
//!   <LI>MIOS32_IRQ_PRIO_HIGHEST  // higher than SRIO, AIN, etc...
//! </UL>
//!
//! Example:<BR>
//! \code
//!   // initialize timer for 1000 uS (= 1 mS) period
//!   MIOS32_TIMER_Init(0, 1000, MyTimer, MIOS32_IRQ_PRIO_MID);
//! \endcode
//! this will call following function periodically:
//! \code
//! void MyTimer(void)
//! {
//!    // your code
//! }
//! \endcode
//! \return 0 if initialisation passed
//! \return -1 if invalid timer number
//! \return -2 if invalid period
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_Init(u8 timer, u32 period, void (*_irq_handler)(void), u8 irq_priority)
{
  // check if valid timer
  if( timer >= NUM_TIMERS )
    return -1; // invalid timer selected

  // check if valid period
  if( period < 1 || period >= 65537 )
    return -2;

  // enable timer clock
  if( timer_base[timer] == TIM1 || timer_base[timer] == TIM8 )
    RCC_APB2PeriphClockCmd(rcc[timer], ENABLE);
  else
    RCC_APB1PeriphClockCmd(rcc[timer], ENABLE);

  // disable interrupt (if active from previous configuration)
  TIM_ITConfig(timer_base[timer], TIM_IT_Update, DISABLE);

  // copy callback function
  timer_callback[timer] = _irq_handler;

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = period-1;
  TIM_TimeBaseStructure.TIM_Prescaler = (TIM_PERIPHERAL_FRQ/1000000)-1; // for 1 uS accuracy
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(timer_base[timer], &TIM_TimeBaseStructure);

  // enable interrupt
  TIM_ITConfig(timer_base[timer], TIM_IT_Update, ENABLE);

  // enable counter
  TIM_Cmd(timer_base[timer], ENABLE);

  // enable global interrupt
  MIOS32_IRQ_Install(timer_irq_chn[timer], irq_priority);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Re-Initialize a timer with given period
//!
//! Example:<BR>
//! \code
//!   // change timer period to 2 mS
//!   MIOS32_TIMER_ReInit(0, 2000);
//! \endcode
//! \param[in] timer (0..2)<BR>
//!     Timer allocation on STM32: 0=TIM2, 1=TIM3, 2=TIM5
//! \param[in] period in uS accuracy (1..65536)
//! \return 0 if initialisation passed
//! \return if invalid timer number
//! \return -2 if invalid period
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_ReInit(u8 timer, u32 period)
{
  // check if valid timer
  if( timer >= NUM_TIMERS )
    return -1; // invalid timer selected

  // check if valid period
  if( period < 1 || period >= 65537 )
    return -2;

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = period - 1;
  TIM_TimeBaseStructure.TIM_Prescaler = (TIM_PERIPHERAL_FRQ/1000000)-1; // for 1 uS accuracy
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(timer_base[timer], &TIM_TimeBaseStructure);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! De-Initialize a timer
//!
//! Example:<BR>
//! \code
//!   // disable timer
//!   MIOS32_TIMER_DeInit(0);
//! \endcode
//! \param[in] timer (0..2)
//! \return 0 if timer has been disabled
//! \return -1 if invalid timer number
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_TIMER_DeInit(u8 timer)
{
  // check if valid timer
  if( timer >= NUM_TIMERS )
    return -1; // invalid timer selected

  // deinitialize timer
  TIM_DeInit(timer_base[timer]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Interrupt handlers
//! \note don't call them directly from application
/////////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_ALLOCATE_TIM2_IRQn
TIMER0_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER0_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER0_BASE, TIM_IT_Update);
    timer_callback[0]();
  }
}
#endif

#ifndef MIOS32_DONT_ALLOCATE_TIM3_IRQn
TIMER1_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER1_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER1_BASE, TIM_IT_Update);
    timer_callback[1]();
  }
}
#endif

#ifndef MIOS32_DONT_ALLOCATE_TIM5_IRQn
TIMER2_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER2_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER2_BASE, TIM_IT_Update);
    timer_callback[2]();
  }
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_TIMER */
