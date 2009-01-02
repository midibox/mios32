// $Id: main.c 42 2008-09-30 21:49:43Z tk $
/*
 * MIOS32 Bootloader
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
#include "bsl_sysex.h"

#include <usb_lib.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// LEDs used by BSL to notify the status
#define BSL_LED_MASK 0x00000001 // green LED

// the "hold" pin (low-active)
// we are using Boot1 = B2, since it suits perfectly for this purpose:
//   o jumper already available (J27)
//   o pull-up already available
//   o normaly used in Open Drain mode, so that no short circuit if jumper is stuffed
#define BSL_HOLD_PORT        GPIOB
#define BSL_HOLD_PIN         GPIO_Pin_2
#define BSL_HOLD_STATE       ((BSL_HOLD_PORT->IDR & BSL_HOLD_PIN) ? 0 : 1)


// timer used to measure delays (TIM1..TIM8)
#ifndef BSL_DELAY_TIMER
#define BSL_DELAY_TIMER  TIM1
#endif

#ifndef BSL_DELAY_TIMER_RCC
#define BSL_DELAY_TIMER_RCC RCC_APB2Periph_TIM1
#endif


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  ///////////////////////////////////////////////////////////////////////////
  // initialize system
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_SYS_Init(0);

  MIOS32_BOARD_LED_Init(BSL_LED_MASK);


  ///////////////////////////////////////////////////////////////////////////
  // get and store state of hold pin
  ///////////////////////////////////////////////////////////////////////////
  u8 hold_mode_active_after_reset = BSL_HOLD_STATE;


  ///////////////////////////////////////////////////////////////////////////
  // initialize timer which is used to measure a 5 second delay before application
  // will be started
  ///////////////////////////////////////////////////////////////////////////

  // enable timer clock
  if( BSL_DELAY_TIMER_RCC == RCC_APB2Periph_TIM1 || BSL_DELAY_TIMER_RCC == RCC_APB2Periph_TIM8 )
    RCC_APB2PeriphClockCmd(BSL_DELAY_TIMER_RCC, ENABLE);
  else
    RCC_APB1PeriphClockCmd(BSL_DELAY_TIMER_RCC, ENABLE);

  // time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 20000; // waiting for 2 seconds (-> 20000 * 100 uS)
  TIM_TimeBaseStructure.TIM_Prescaler = 7200-1; // for 100 uS accuracy @ 72 MHz
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(BSL_DELAY_TIMER, &TIM_TimeBaseStructure);

  // enable interrupt
  TIM_ITConfig(BSL_DELAY_TIMER, TIM_IT_Update, ENABLE);

  // start counter
  TIM_Cmd(BSL_DELAY_TIMER, ENABLE);


  ///////////////////////////////////////////////////////////////////////////
  // initialize USB only if already done (-> not after Power On) or Hold mode enabled
  ///////////////////////////////////////////////////////////////////////////

  if( MIOS32_USB_IsInitialized() || BSL_HOLD_STATE ) {
    // if initialized, this function will only set some variables - it won't re-init the peripheral.
    // if hold mode activated via external pin, force re-initialisation by resetting USB
    if( hold_mode_active_after_reset ) {
      RCC_APB1PeriphResetCmd(0x00800000, ENABLE); // reset USB device
      RCC_APB1PeriphResetCmd(0x00800000, DISABLE);
    }

    MIOS32_USB_Init(0);
  }


  ///////////////////////////////////////////////////////////////////////////
  // initialize remaining peripherals
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_MIDI_Init(0); // will also initialise UART


  ///////////////////////////////////////////////////////////////////////////
  // initialize SysEx parser
  ///////////////////////////////////////////////////////////////////////////
  BSL_SYSEX_Init(0);


  ///////////////////////////////////////////////////////////////////////////
  // send upload request to USB and UART MIDI
  ///////////////////////////////////////////////////////////////////////////
  BSL_SYSEX_SendUploadReq(UART0);    
  BSL_SYSEX_SendUploadReq(USB0);


  ///////////////////////////////////////////////////////////////////////////
  // clear timer pending flag and start wait loop
  TIM_ClearITPendingBit(BSL_DELAY_TIMER, TIM_IT_Update);
  do {
    // This is a simple way to pulsate a LED via PWM
    // A timer in incrementer mode is used as reference, the counter value is incremented each 100 uS
    // Within the given PWM period, we define a duty cycle based on the current counter value
    // We periodically sweep the PWM duty cycle 100 steps up, and 100 steps down
    u32 cnt = BSL_DELAY_TIMER->CNT;  // the reference counter (incremented each 100 uS)
    const u32 pwm_period = 50;       // *100 uS -> 5 mS
    const u32 pwm_sweep_steps = 100; // * 5 mS -> 500 mS
    u32 pwm_duty = ((cnt / pwm_period) % pwm_sweep_steps) / (pwm_sweep_steps/pwm_period);
    if( (cnt % (2*pwm_period*pwm_sweep_steps)) > pwm_period*pwm_sweep_steps )
      pwm_duty = pwm_period-pwm_duty; // negative direction each 50*100 ticks
    u32 led_on = ((cnt % pwm_period) > pwm_duty) ? 1 : 0;
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, led_on ? BSL_LED_MASK : 0);

    // handle USB messages
    MIOS32_USB_MIDI_Handler();

    // check for incoming MIDI messages - no hooks are used
    // SysEx requests will be parsed by MIOS32 internally, BSL_SYSEX_Cmd() will be called
    // directly by MIOS32 to enhance command set
    MIOS32_MIDI_Receive_Handler(NULL, NULL);

  } while( TIM_GetITStatus(BSL_DELAY_TIMER, TIM_IT_Update) == RESET || 
	   BSL_SYSEX_HaltStateGet() ||
	   (hold_mode_active_after_reset && BSL_HOLD_STATE));

  // ensure that flash write access is locked
  FLASH_Lock();

  // branch to application if reset vector is valid (should be inside flash range)
  u32 *reset_vector = (u32 *)0x08004004;
  if( (*reset_vector >> 24) == 0x08 ) {
    // reset all peripherals
#ifdef MIOS32_BOARD_STM32_PRIMER
    RCC_APB2PeriphResetCmd(0xfffffff7, ENABLE); // Primer: don't reset GPIOB due to USB disconnect pin
#else
    RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
#endif
    RCC_APB1PeriphResetCmd(0xff7fffff, ENABLE); // don't reset USB, so that the connection can survive!
    RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
    RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);

    if( hold_mode_active_after_reset ) {
      // if hold mode activated via external pin, force re-initialisation by resetting USB
      RCC_APB1PeriphResetCmd(0x00800000, ENABLE); // reset USB device
      RCC_APB1PeriphResetCmd(0x00800000, DISABLE);
    } else {
      // no hold mode: ensure that USB interrupts won't be triggered while jumping into application
      _SetCNTR(0); // clear interrupt mask
      _SetISTR(0); // clear all requests
    }

    // change stack pointer
    u32 *stack_pointer = (u32 *)0x08004000;
    __MSR_MSP(*stack_pointer);

    // branch to application
    void (*application)(void) = (void *)*reset_vector;
    application();
  }

  // otherwise flash LED fast (BSL failed to start application)
  while( 1 ) {
    u32 led_on = ((BSL_DELAY_TIMER->CNT % 1000) > 500) ? 1 : 0;
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, led_on ? BSL_LED_MASK : 0);
  }

  return 0; // will never be reached
}
