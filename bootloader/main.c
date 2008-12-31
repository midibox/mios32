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
  // send upload request to USB and UART MIDI
  ///////////////////////////////////////////////////////////////////////////
  BSL_SYSEX_SendUploadReq(UART0);    
  BSL_SYSEX_SendUploadReq(USB0);


  ///////////////////////////////////////////////////////////////////////////
  // clear timer pending flag and start wait loop
  TIM_ClearITPendingBit(BSL_DELAY_TIMER, TIM_IT_Update);
  do {
    // first LED flashes in intervals for 1s with 4:1 duty cycle
    MIOS32_BOARD_LED_Set(1 << BSL_LED_NUM, ((BSL_DELAY_TIMER->CNT % 10000) < 7500) ? (1 << BSL_LED_NUM) : 0);

    // handle USB messages
    MIOS32_USB_MIDI_Handler();

    // check for incoming MIDI messages - no hooks are used
    // SysEx requests will be parsed by MIOS32 internally, BSL_SYSEX_Cmd() will be called
    // directly by MIOS32 to enhance command set
    MIOS32_MIDI_Receive_Handler(NULL, NULL);

#if 0
    // another request after 5 seconds
    static int onlyonce = 0;
    if( !onlyonce && TIM_GetITStatus(BSL_DELAY_TIMER, TIM_IT_Update) != RESET ) {
      onlyonce=1;
      BSL_SYSEX_SendUploadReq(UART0);    
      BSL_SYSEX_SendUploadReq(USB0);
    }
#endif

  } while( TIM_GetITStatus(BSL_DELAY_TIMER, TIM_IT_Update) == RESET || 1 );

  // turn on LED permanently 
  MIOS32_BOARD_LED_Set(1 << BSL_LED_NUM, 1 << BSL_LED_NUM);

  // ensure that flash write access is locked
  FLASH_Lock();

  // TODO: here we will branch to the application later
  while( 1 );

  return 0; // will never be reached
}
