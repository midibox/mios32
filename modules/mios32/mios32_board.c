// $Id$
/*
 * Development Board specific functions for MIOS32
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
#if !defined(MIOS32_DONT_USE_BOARD)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes LEDs of the board
// IN: <mask> contains a flag for each LED which should be initialized
//     MBHP_CORE_STM32: 1 LED (flag 0: green)
//     STM32_PRIMER: 2 LEDs (flag 0: green, flag 1: red)
// OUT: returns 0 if initialisation passed
//      -1 if no LEDs specified for board
//      -2 if one or more LEDs not available on board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_LED_Init(u32 leds)
{
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // only one LED, connected to PD2
  if( leds & 1 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( leds & 1 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }
  if( leds & 2 ) {
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }

  if( leds & 0xfffffffc)
    return -2; // LED doesn't exist

  return 0; // no error
#else
  return -1; // no LED specified for board
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Sets one or more LEDs to the given value(s)
// IN: <mask> contains a flag for each LED which should be changed
//     <value> contains the value which should be set
// OUT: returns 0 if initialisation passed
//      -1 if no LEDs specified for board
//      -2 if one or more LEDs not available on board
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value)
{
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // only one LED, connected to PD2
  if( leds & 1 ) {
    GPIOD->BSRR = (value&1) ? GPIO_Pin_2 : (GPIO_Pin_2 << 16);
  }

  if( leds & 0xfffffffe)
    return -2; // LED doesn't exist

  return 0; // no error
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( leds & 1 ) {
    GPIOB->BSRR = (value&1) ? GPIO_Pin_8 : (GPIO_Pin_8 << 16);
  }
  if( leds & 2 ) {
    GPIOB->BSRR = (value&2) ? GPIO_Pin_9 : (GPIO_Pin_9 << 16);
  }

  if( leds & 0xfffffffc)
    return -2; // LED doesn't exist

  return 0; // no error
#else
  return -1; // no LED specified for board
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Returns the status of all LEDs
// IN: -
// OUT: status of all LEDs
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_BOARD_LED_Get(void)
{
  u32 values = 0;

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
  // only one LED, connected to PD2
  if( GPIOD->ODR & GPIO_Pin_2 )
    values |= (1 << 0);
#elif defined(MIOS32_BOARD_STM32_PRIMER)
  // two LEDs, connected to PB8 (green) and PB9 (red)
  if( GPIOB->ODR & GPIO_Pin_8 )
    values |= (1 << 0);
  if( GPIOB->ODR & GPIO_Pin_9 )
    values |= (1 << 1);
#endif

  return values;
}


#endif /* MIOS32_DONT_USE_BOARD */
