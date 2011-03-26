/******************************************************************************
 * @file:    system_LPC177x_8x.h
 * @purpose: CMSIS Cortex-M3 Device Peripheral Access Layer Header File
 *           for the NXP LPC177x_8x Device Series
 * @version: V1.11
 * @date:    10. November. 2010
 *----------------------------------------------------------------------------
 *
 * @note
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M
 * processor based microcontrollers.  This file can be freely distributed
 * within development tools that are supporting such ARM based processors.
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/


#ifndef __SYSTEM_LPC177x_8x_H
#define __SYSTEM_LPC177x_8x_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t SystemCoreClock;      /*!< System Clock Frequency (Core Clock)  	*/
extern uint32_t PeripheralClock;	    /*!< Peripheral Clock Frequency (Pclk) 	    */
extern uint32_t EMCClock;			        /*!< EMC Clock                              */
extern uint32_t USBClock;			        /*!< USB Frequency 						              */
extern uint32_t SPIFIClock;			      /*!< SPI Flash Interface Frequency 		      */

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
extern void SystemInit (void);

/**
 * Update SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
extern void SystemCoreClockUpdate (void);

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define XTAL        (12000000UL)        /* Oscillator frequency               */
#define OSC_CLK     (      XTAL)        /* Main oscillator frequency          */
#define RTC_CLK     (   32768UL)        /* RTC oscillator frequency           */
#define IRC_OSC     (12000000UL)        /* Internal RC oscillator frequency   */
#define WDT_OSC		  (  500000UL)		/* Internal WDT oscillator frequency  */



/*
//-------- <<< end of configuration section >>> ------------------------------
*/

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_LPC177x_8x_H */
