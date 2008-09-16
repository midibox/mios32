// $Id$
/*
 * System Initialisation for MIOS32
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
#if !defined(MIOS32_DONT_USE_SYS)

/////////////////////////////////////////////////////////////////////////////
// Initializes the System for MIOS32
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  ErrorStatus HSEStartUpStatus;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // Start with the clocks in their expected state
  RCC_DeInit();

  // Enable HSE (high speed external clock)
  RCC_HSEConfig(RCC_HSE_ON);

  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if( HSEStartUpStatus == SUCCESS )
  {
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);

    // HCLK = SYSCLK
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    // PCLK2 = HCLK
    RCC_PCLK2Config(RCC_HCLK_Div1);

    // PCLK1 = HCLK/2
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* ADCCLK = PCLK2/6 */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* PLLCLK = 12MHz * 6 = 72 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while( RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET );

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while( RCC_GetSYSCLKSource() != 0x08 );
  }

  // Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
			 | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);

  // Set the Vector Table base address at 0x08000000
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  // Configure HCLK clock as SysTick clock source
  SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

  // Configure USB clock
#if !defined(MIOS32_DONT_USE_USB)
  // USBCLK = PLLCLK / 1.5
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  // Enable USB clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
#endif

  // error during clock configuration?
  return HSEStartUpStatus == SUCCESS ? 0 : -1;
}

#endif /* MIOS32_DONT_USE_SYS */
