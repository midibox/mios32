// $Id$
//! \defgroup MIOS32_TIMER
//!
//! Timer functions for MIOS32
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
#if !defined(MIOS32_DONT_USE_TIMER)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// timers clocked at CCLK/4 MHz
#define TIM_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)

#define NUM_TIMERS 3

#define TIMER0_BASE                 LPC_TIM0
#define TIMER0_IRQ                  TIMER0_IRQn
#define TIMER0_IRQ_HANDLER     void TIMER0_IRQHandler(void)

#define TIMER1_BASE                 LPC_TIM1
#define TIMER1_IRQ                  TIMER1_IRQn
#define TIMER1_IRQ_HANDLER     void TIMER1_IRQHandler(void)

#define TIMER2_BASE                 LPC_TIM2
#define TIMER2_IRQ                  TIMER2_IRQn
#define TIMER2_IRQ_HANDLER     void TIMER2_IRQHandler(void)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static LPC_TIM_TypeDef *timer_base[NUM_TIMERS] = { TIMER0_BASE, TIMER1_BASE, TIMER2_BASE };
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
  // TODO: should we allow 2^32-1 for LPC17xx?
  // Problem: this would make applications incompatible to STM32
  if( period < 1 || period >= 65537 )
    return -2;

  LPC_TIM_TypeDef *tim = (LPC_TIM_TypeDef *)timer_base[timer];

  // reset timer
  tim->TCR |= (1 << 1);
  tim->TCR &= ~(1 << 1);

  // copy callback function
  timer_callback[timer] = _irq_handler;

  // time base configuration
  tim->CTCR = 0x00;               // timer mode
  tim->PR = ((TIM_PERIPHERAL_FRQ/1000000) * 1)-1; // <resolution> uS accuracy @ CCLK/4 Peripheral Clock
  tim->MR0 = period;              // interrupt event on overrun
  tim->MCR = (1 << 1) | (1 << 0); // generate event on match, reset counter on match
  tim->IR = ~0;                   // clear all interrupts

  // enable timer
  tim->TCR = 1;

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
  // TODO: should we allow 2^32-1 for LPC17xx?
  // Problem: this would make applications incompatible to STM32
  if( period < 1 || period >= 65537 )
    return -2;

  LPC_TIM_TypeDef *tim = (LPC_TIM_TypeDef *)timer_base[timer];

  // time base configuration
  tim->MCR = 0;                   // disable timer
  tim->CTCR = 0x00;               // timer mode
  tim->PR = ((TIM_PERIPHERAL_FRQ/1000000) * 1)-1; // <resolution> uS accuracy @ CCLK/4 Peripheral Clock
  tim->MR0 = period;              // interrupt event on overrun
  tim->TC = 0;                    // reset counter
  tim->MCR = (1 << 1) | (1 << 0); // generate event on match, reset counter on match

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

  LPC_TIM_TypeDef *tim = (LPC_TIM_TypeDef *)timer_base[timer];

  // reset timer
  tim->TCR |= (1 << 1);
  tim->TCR &= ~(1 << 1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Interrupt handlers
//! \note don't call them directly from application
/////////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_ALLOCATE_TIMER0_IRQ
TIMER0_IRQ_HANDLER
{
  // clear all interrupt flags
  TIMER0_BASE->IR = ~0;
  // -> callback
  timer_callback[0]();
}
#endif

#ifndef MIOS32_DONT_ALLOCATE_TIMER1_IRQ
TIMER1_IRQ_HANDLER
{
  // clear all interrupt flags
  TIMER1_BASE->IR = ~0;
  // -> callback
  timer_callback[1]();
}
#endif

#ifndef MIOS32_DONT_ALLOCATE_TIMER2_IRQ
TIMER2_IRQ_HANDLER
{
  // clear all interrupt flags
  TIMER2_BASE->IR = ~0;
  // -> callback
  timer_callback[2]();
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_TIMER */
