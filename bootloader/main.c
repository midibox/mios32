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

  // initialize stopwatch which is used to measure a 2 second delay 
  // before application will be started
  MIOS32_STOPWATCH_Init(100); // 100 uS accuracy


  ///////////////////////////////////////////////////////////////////////////
  // get and store state of hold pin
  ///////////////////////////////////////////////////////////////////////////
  u8 hold_mode_active_after_reset = BSL_HOLD_STATE;


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
  // reset stopwatch timer and start wait loop
  MIOS32_STOPWATCH_Reset();
  do {
    // This is a simple way to pulsate a LED via PWM
    // A timer in incrementer mode is used as reference, the counter value is incremented each 100 uS
    // Within the given PWM period, we define a duty cycle based on the current counter value
    // We periodically sweep the PWM duty cycle 100 steps up, and 100 steps down
    u32 cnt = MIOS32_STOPWATCH_ValueGet();  // the reference counter (incremented each 100 uS)
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

  } while( MIOS32_STOPWATCH_ValueGet() < 20000 ||             // wait for 2 seconds
	   BSL_SYSEX_HaltStateGet() ||                        // BSL not halted due to flash write operation
	   (hold_mode_active_after_reset && BSL_HOLD_STATE)); // BSL not actively halted by pin

  // ensure that flash write access is locked
  FLASH_Lock();

  // turn of LED
  MIOS32_BOARD_LED_Set(BSL_LED_MASK, 0);

  // branch to application if reset vector is valid (should be inside flash range)
  u32 *reset_vector = (u32 *)0x08004004;
  if( (*reset_vector >> 24) == 0x08 ) {
    // reset all peripherals
#ifdef MIOS32_BOARD_STM32_PRIMER
    RCC_APB2PeriphResetCmd(0xfffffff0, ENABLE); // Primer: don't reset GPIOA/AF + GPIOB due to USB detach pin
#else
    RCC_APB2PeriphResetCmd(0xfffffff8, ENABLE); // MBHP_CORE_STM32: don't reset GPIOA/AF due to USB pins
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
    MIOS32_STOPWATCH_Reset();
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, BSL_LED_MASK);
    while( MIOS32_STOPWATCH_ValueGet() < 500 );

    MIOS32_STOPWATCH_Reset();
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, 0);
    while( MIOS32_STOPWATCH_ValueGet() < 500 );
  }

  return 0; // will never be reached
}
