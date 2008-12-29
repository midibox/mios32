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


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// number of LED used by BSL to notify status (counting from 0)
#define BSL_LED_NUM 0 // green LED


// timer used to measure delays (TIM1..TIM8)
#ifndef BSL_DELAY_TIMER
#define BSL_DELAY_TIMER  TIM1
#endif

#ifndef BSL_DELAY_TIMER_RCC
#define BSL_DELAY_TIMER_RCC RCC_APB2Periph_TIM1
#endif


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void BSL_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte);


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  ///////////////////////////////////////////////////////////////////////////
  // initialize system
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_SYS_Init(0);

  MIOS32_BOARD_LED_Init(1 << BSL_LED_NUM);


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
  TIM_TimeBaseStructure.TIM_Period = 50000; // waiting for 5 seconds (-> 50000 * 100 uS)
  TIM_TimeBaseStructure.TIM_Prescaler = 7200-1; // for 100 uS accuracy @ 72 MHz
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(BSL_DELAY_TIMER, &TIM_TimeBaseStructure);

  // enable interrupt
  TIM_ITConfig(BSL_DELAY_TIMER, TIM_IT_Update, ENABLE);

  // start counter
  TIM_Cmd(BSL_DELAY_TIMER, ENABLE);


  ///////////////////////////////////////////////////////////////////////////
  // initialize remaining peripherals
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_USB_Init(0);
  MIOS32_UART_Init(0);
  MIOS32_MIDI_Init(0);


  ///////////////////////////////////////////////////////////////////////////
  // initialize SysEx parser
  ///////////////////////////////////////////////////////////////////////////
  BSL_SYSEX_Init(0);


  ///////////////////////////////////////////////////////////////////////////
  // clear timer pending flag and start wait loop
  TIM_ClearITPendingBit(BSL_DELAY_TIMER, TIM_IT_Update);
  do {
    // first LED flashes in intervals for 1s with 4:1 duty cycle
    MIOS32_BOARD_LED_Set(1 << BSL_LED_NUM, ((BSL_DELAY_TIMER->CNT % 10000) < 7500) ? (1 << BSL_LED_NUM) : 0);

    // handle USB messages
    MIOS32_USB_MIDI_Handler();

    // check for incoming MIDI messages and call SysEx hook (event hook not used)
    MIOS32_MIDI_Receive_Handler(NULL, BSL_NotifyReceivedSysEx);

  } while( TIM_GetITStatus(BSL_DELAY_TIMER, TIM_IT_Update) == RESET || 1 );

  // turn on LED permanently 
  MIOS32_BOARD_LED_Set(1 << BSL_LED_NUM, 1 << BSL_LED_NUM);

  // TODO: here we will branch to the application later
  while( 1 );

  return 0; // will never be reached
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
static void BSL_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
  // forward to SysEx parser
  // reset timer on success (so that we wait for 5 seconds again)
  if( BSL_SYSEX_Parser(port, sysex_byte) > 0 ) {
    BSL_DELAY_TIMER->CNT = 1; // not 0 to avoid that pending flag will be set unintentionally
    TIM_ClearITPendingBit(BSL_DELAY_TIMER, TIM_IT_Update);
  }
}
