// $Id$
/*
 * Timer interrupt functions for MIOS32
 *
 * ==========================================================================
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
#define TIMER0_IRQ                  TIM2_IRQChannel
#define TIMER0_IRQ_HANDLER     void TIM2_IRQHandler(void)

#define TIMER1_BASE                 TIM3
#define TIMER1_RCC   RCC_APB1Periph_TIM3
#define TIMER1_IRQ                  TIM3_IRQChannel
#define TIMER1_IRQ_HANDLER     void TIM3_IRQHandler(void)

#define TIMER2_BASE                 TIM5
#define TIMER2_RCC   RCC_APB1Periph_TIM5
#define TIMER2_IRQ                  TIM5_IRQChannel
#define TIMER2_IRQ_HANDLER     void TIM5_IRQHandler(void)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static TIM_TypeDef *timer_base[NUM_TIMERS] = { TIMER0_BASE, TIMER1_BASE, TIMER2_BASE };
static u32 rcc[NUM_TIMERS] = { TIMER0_RCC, TIMER1_RCC, TIMER2_RCC };
static u32 timer_irq_chn[NUM_TIMERS] = { TIMER0_IRQ, TIMER1_IRQ, TIMER2_IRQ };
static void (*timer_callback[NUM_TIMERS])(void);


/////////////////////////////////////////////////////////////////////////////
// Initialize a timer
// IN: timer (0..2)
//     period in uS accuracy (1..65536)
//     irq_handler (function-name)
//     irq_priority: one of these values
//       MIOS32_IRQ_PRIO_LOW      // lower than RTOS
//       MIOS32_IRQ_PRIO_MID      // higher than RTOS
//       MIOS32_IRQ_PRIO_HIGH     // same like SRIO, AIN, etc...
//       MIOS32_IRQ_PRIO_HIGHEST  // same like SRIO, AIN, etc...
// OUT: returns 0 if initialisation passed
//      -1 if invalid timer number
//      -2 if invalid period
// EXAMPLE:
//
//   // initialize timer for 1000 uS (= 1 mS) period
//   MIOS32_TIMER_Init(0, 1000, MyTimer, MIOS32_IRQ_PRIO_MID);
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

  // enable timer clock
  RCC_APB1PeriphClockCmd(rcc[timer], ENABLE);

  // disable interrupt (if active from previous configuration)
  TIM_ITConfig(timer_base[timer], TIM_IT_Update, DISABLE);

  // copy callback function
  timer_callback[timer] = _irq_handler;

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = period-1;
  TIM_TimeBaseStructure.TIM_Prescaler = 72; // for 1 uS accuracy. TODO: dynamic change depending on system frequency
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(timer_base[timer], &TIM_TimeBaseStructure);

  // enable interrupt
  TIM_ITConfig(timer_base[timer], TIM_IT_Update, ENABLE);

  // enable counter
  TIM_Cmd(timer_base[timer], ENABLE);

  // enable global interrupt
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = timer_irq_chn[timer];
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = irq_priority;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
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

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = period - 1;
  TIM_TimeBaseStructure.TIM_Prescaler = 72; // for 1 uS accuracy. TODO: dynamic change depending on system frequency
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(timer_base[timer], &TIM_TimeBaseStructure);
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

  // deinitialize timer
  TIM_DeInit(timer_base[timer]);
}


/////////////////////////////////////////////////////////////////////////////
// Interrupt handlers
/////////////////////////////////////////////////////////////////////////////
TIMER0_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER0_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER0_BASE, TIM_IT_Update);
    timer_callback[0]();
  }
}

TIMER1_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER1_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER1_BASE, TIM_IT_Update);
    timer_callback[1]();
  }
}

TIMER2_IRQ_HANDLER
{
  if( TIM_GetITStatus(TIMER2_BASE, TIM_IT_Update) != RESET ) {
    TIM_ClearITPendingBit(TIMER2_BASE, TIM_IT_Update);
    timer_callback[2]();
  }
}

#endif /* MIOS32_DONT_USE_TIMER */
