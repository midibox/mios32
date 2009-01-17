// $Id$
/*
 * MIOS32 Bootloader dummy (directly branches to application)
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

#include <usb_lib.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// LEDs used by BSL to notify the status
#define BSL_LED_MASK 0x00000001 // green LED


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  // branch to application if reset vector is valid (should be inside flash range)
  u32 *reset_vector = (u32 *)0x08004004;
  if( (*reset_vector >> 24) == 0x08 ) {
    // reset all peripherals
    RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
    RCC_APB1PeriphResetCmd(0xff7fffff, ENABLE); // don't reset USB, so that the connection can survive!
    RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
    RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);

    // no hold mode: ensure that USB interrupts won't be triggered while jumping into application
    _SetCNTR(0); // clear interrupt mask
    _SetISTR(0); // clear all requests

    // change stack pointer
    u32 *stack_pointer = (u32 *)0x08004000;
    __MSR_MSP(*stack_pointer);

    // branch to application
    void (*application)(void) = (void *)*reset_vector;
    application();
  }

  // otherwise flash LED fast (BSL failed to start application)
  MIOS32_SYS_Init(0);
  MIOS32_BOARD_LED_Init(BSL_LED_MASK);
  u32 delay;
  while( 1 ) {
    for(delay=0; delay<100000; ++delay)
      MIOS32_BOARD_LED_Set(BSL_LED_MASK, BSL_LED_MASK);
    for(delay=0; delay<100000; ++delay)
      MIOS32_BOARD_LED_Set(BSL_LED_MASK, 0);
  }

  return 0; // will never be reached
}
