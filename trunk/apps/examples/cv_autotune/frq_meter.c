// $Id$
/*
 * Frequency Meter
 *
 * ==========================================================================
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
#include "frq_meter.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

u32 frq_meter_current_frq;


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// how many frequency captures to calculate the mean value?
#define NUM_CAP_SAMPLES 4

// samples and sample counter
static u16 cap_samples[NUM_CAP_SAMPLES];
static u16 cap_sample_ctr;
static u16 timeout_ctr;
static u8 frq_updated;


/////////////////////////////////////////////////////////////////////////////
// Initializes the frequency meter
/////////////////////////////////////////////////////////////////////////////
s32 FRQ_METER_Init(u32 mode)
{
  // clear the sample counter
  cap_sample_ctr = 0;
  timeout_ctr = 0;
  frq_updated = 0;

  frq_meter_current_frq = FRQ_METER_TOO_LOW;

#ifdef MIOS32_FAMILY_STM32F10x
  // STM32:
  // Timer configuration (we use TIM2)
  // We use TIM2, note that it is normaly assigned to MIOS32_TIMER0 (don't use it in your app)

  // enable peripheral clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  // ensure that interrupt is disabled
  TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);

  // configure PA0 (J5B.A4) as digital input
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // internal pull-up enabled
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // time base configuration
  // we use a prescaler value of 32, this results into 2.25 MHz timer clock
  // Due to the 16 bit timer resolution the lowest frequency which can be measured is 34.3 Hz
  // The highest frequency is ca. 20 kHz (based on input filter value)
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 65535; // full range
  TIM_TimeBaseStructure.TIM_Prescaler = 32-1; // -> 72 MHz / 32 = 2.25 MHz
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  // Overrun detection: the SR.UIF flag (update) can't be used, as it is set on any update, also on slave request
  // CR.URS=1 doesn't help, as it doesn't allow the slave channel to reset the timer.
  // This seems to be a conceptional flaw...
  // However, we can "fake" the flag by setting the compare value of an unused channel to maximum
  TIM_SetCompare4(TIM2, 0xffff);

  // TIM configuration: PWM input mode
  // External signal is connected to TIM2 CH1 pin PA0
  // Falling edge used as active edge (thats better since input is driven by open-drain driver)
  // TIM CCR1 is used to compute the frequency value
  // TIM CCR2 is used to compute the duty cycle value
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x8; // increased value for stable results
  TIM_PWMIConfig(TIM2, &TIM_ICInitStructure);

  // Select the TIM2 Input Trigger: TI1FP1
  TIM_SelectInputTrigger(TIM2, TIM_TS_TI1FP1);

  // Select the slave Mode: Reset Mode
  TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Reset);

  // Enable the Master/Slave Mode
  TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Enable);

  // enable interrupt
  TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

  // TIM enable counter
  TIM_Cmd(TIM2, ENABLE);

  // enable global interrupt
  MIOS32_IRQ_Install(TIM2_IRQn, MIOS32_IRQ_PRIO_MID);
#elif MIOS32_FAMILY_LPC17xx
  // LPC17:
  // Timer configuration (we use LPC_TIM2)
  // Note that it is normaly assigned to MIOS32_TIMER2 (don't use it in your app)
  LPC_TIM_TypeDef *tim = (LPC_TIM_TypeDef *)LPC_TIM2;

  // configure CAP2 input, which is available at P0.4
  // (J18, a pull-up resistor is already available on this pin)
  MIOS32_SYS_LPC_PINSEL(0, 4, 3);
  MIOS32_SYS_LPC_PINMODE(0, 4, 0);

  // reset timer
  tim->TCR |= (1 << 1);
  tim->TCR &= ~(1 << 1);

  // time base configuration
  // we use a prescaler value which results into 2.5 MHz timer clock
  // Due to the 32 bit timer resolution the lowest frequency which can be measured is much less than 1 Hz! :-)
  tim->CTCR = 0x00;               // timer mode
#if MIOS32_SYS_CPU_FREQUENCY != 120000000
# warning "Adapt here to achieve 2.5 MHz"
#endif
  tim->PR = ((MIOS32_SYS_CPU_FREQUENCY/4)/1200000)-1; // 0.25 uS accuracy @ CCLK/4 Peripheral Clock
  tim->MR0 = 10000000;            // interrupt event when frequency less than 1 Hz
  tim->MCR = (1 << 1) | (1 << 0); // generate event on match (overrun)
  tim->CCR = (1 << 1) | (1 << 2); // capture on CAP2.0 falling edge and generate interrupt
  tim->IR = ~0;                   // clear all interrupts

  // enable timer
  tim->TCR = 1;

  // enable global interrupt
  MIOS32_IRQ_Install(TIMER2_IRQn, MIOS32_IRQ_PRIO_MID);
#else
# error "frq_meter not adapted to this MIOS32_FAMILY yet!"
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called each millisecond to check for capture events
// Return value:
// - FRQ_METER_NO_UPDATE: no update since last call
// - FRQ_METER_TOO_LOW: if frequency too low
// - FRQ_METER_TOO_HIGH: if frequency too high
// - otherwise: the frequency * 1000
/////////////////////////////////////////////////////////////////////////////
u32 FRQ_METER_Tick(void)
{
  // check for timeout
  if( ++timeout_ctr >= 30*NUM_CAP_SAMPLES ) { // sufficient for 30 Hz
    timeout_ctr = 0;
    frq_meter_current_frq = FRQ_METER_TOO_LOW;
    return FRQ_METER_TOO_LOW;
  } else if( !frq_updated ) {
    return FRQ_METER_NO_UPDATE;
  }

  MIOS32_IRQ_Disable();
  u32 frq = frq_meter_current_frq;
  frq_updated = 0;
  MIOS32_IRQ_Enable();

  return frq;
}


/////////////////////////////////////////////////////////////////////////////
//! Interrupt handler for TIM2
/////////////////////////////////////////////////////////////////////////////
#ifdef MIOS32_FAMILY_STM32F10x
void TIM2_IRQHandler(void)
#elif MIOS32_FAMILY_LPC17xx
void TIMER2_IRQHandler(void)
#else
# error "frq_meter not adapted to this MIOS32_FAMILY yet!"
#endif

{
#ifdef MIOS32_FAMILY_STM32F10x
  // STM32 capture event handling
  u8 irq_triggered = TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET;
  u16 cap1 = 0;
  if( irq_triggered ) {
    TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
    cap1 = TIM_GetCapture1(TIM2); // this will clear the CC1 flag
    //u16 cap2 = TIM_GetCapture2(TIM2); // this will clear the CC2 flag
  }

  u8 overcaptured = TIM2->SR & TIM_FLAG_CC1OF;
  if( overcaptured )
      TIM2->SR = ~TIM_FLAG_CC1OF;

  u8 overrun = TIM2->SR & TIM_FLAG_CC4;
  if( overrun )
    TIM2->SR = ~TIM_FLAG_CC4;
#elif MIOS32_FAMILY_LPC17xx
  // LPC17 capture event handling
  // Note: for some reasons it isn't possible to reset the timer on a capture event (unfortunately!!!)
  // Accordingly we have to use the timer in freerunning mode, and compare the difference between
  // two capture values to determine the period length.
  // We also have to handle the overrun (== frequency too low) detection
  LPC_TIM_TypeDef *tim = (LPC_TIM_TypeDef *)LPC_TIM2;
  u8 irq_triggered = tim->IR & (1 << 4); // check for CR0 interrupt
  static u16 prev_cr0 = 0;
  static u8 event_captured = 0;
  u32 cap1 = 0;
  if( irq_triggered ) {
    tim->IR &= (1 << 4); // clear CR0 flag

    u32 cr0 = tim->CR0; // captured timer value
    int diff = cr0 - prev_cr0;
    if( diff < 0 )
      diff += tim->MR0+1;
    cap1 = diff;
    prev_cr0 = cr0;
    event_captured = 1;
  }

  u8 overcaptured = 0; // not available for LPC17
  u8 overrun = 0;
  if( tim->IR & (1 << 0) ) { // check for MR0 interrupt
    tim->IR &= (1 << 0); // clear MR0 flag
    // set overrun if no new capture event within 1 second
    if( !event_captured )
      overrun = 1;
    event_captured = 0;
  }
#else
# error "frq_meter not adapted to this MIOS32_FAMILY yet!"
#endif
  
  if( irq_triggered ) {
    // clear timeout
    timeout_ctr = 0;

    // overcaptured? (means: register not read early enough, will happen at frequencies > 20 kHz)
    // SR should be read after Capture1 read
    if( overcaptured ) {
      cap_sample_ctr = 0;
      frq_meter_current_frq = FRQ_METER_TOO_HIGH;
      frq_updated = 1;
    } else if( overrun ) {
      // timer overrun!
      cap_sample_ctr = 0;
      frq_meter_current_frq = FRQ_METER_TOO_LOW;
      frq_updated = 1;
    } else {
      // store capture value
      cap_samples[cap_sample_ctr++] = cap1;

      if( cap_sample_ctr >= NUM_CAP_SAMPLES ) {
	cap_sample_ctr = 0;

	u32 sum = 0;
	int i;
	for(i=0; i<NUM_CAP_SAMPLES; ++i)
	  sum += cap_samples[i];
	u32 mean = sum / NUM_CAP_SAMPLES;

	// ooof! 2.25 MHz * 1000 still in 32bit range
	frq_meter_current_frq = 2250000000U / mean;
	frq_updated = 1;
      }
    }
  }
}
