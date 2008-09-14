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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#if defined(_STM32x_)
# include <stm32f10x_map.h>
# include <stm32f10x_rcc.h>
# include <stm32f10x_gpio.h>
# include <stm32f10x_spi.h>
# include <stm32f10x_nvic.h>
#else
  XXX unsupported derivative XXX
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes the System for MIOS32
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // Start with the clocks in their expected state
  RCC_DeInit();

  // Enable HSE (high speed external clock)
  RCC_HSEConfig(RCC_HSE_ON);

  // Wait till HSE is ready
  while( RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET );

  // 2 wait states required on the flash
  *( ( unsigned portLONG * ) 0x40022000 ) = 0x02;

  // HCLK = SYSCLK
  RCC_HCLKConfig(RCC_SYSCLK_Div1);

  // PCLK2 = HCLK
  RCC_PCLK2Config(RCC_HCLK_Div1);

  // PCLK1 = HCLK/2
  RCC_PCLK1Config(RCC_HCLK_Div2);

  // PLLCLK = 12MHz * 6 = 72 MHz
  RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);

  // Enable PLL
  RCC_PLLCmd(ENABLE);

  // Wait till PLL is ready
  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

  // Select PLL as system clock source
  RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );

  // Wait till PLL is used as system clock source
  while( RCC_GetSYSCLKSource() != 0x08 );

  // Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
			 | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);

  // Set the Vector Table base address at 0x08000000
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  // Configure HCLK clock as SysTick clock source
  SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

  return 0;
}

