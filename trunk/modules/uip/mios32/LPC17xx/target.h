/*****************************************************************************
 *   target.h:  Header file for NXP LPC23xx/24xx Family Microprocessors
 *
 *   Copyright(C) 2006, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2006.09.20  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/

/*
 *  CodeRed - minor modifications for port to RDB1768 development board
 */

#ifndef __TARGET_H 
#define __TARGET_H

#ifdef __cplusplus
   extern "C" {
#endif

// board selection
#if defined (_MCB1700_)
#define KEIL_BOARD_MCB17XX
#elif defined ( _RDB1768_ )
#define CODERED_BOARD_RDB1768
#elif defined (MIOS32_BOARD_LPCXPRESSO) || defined (MIOS32_BOARD_MBHP_CORE_LPC17)
     // great!
#else
#error "No board is selected!!!"
#endif

/* On EA and IAR boards, they use Micrel PHY.
   on ENG and KEIL boards, they use National PHY */
#define NATIONAL_PHY			1
#define MICREL_PHY				2

/*Fosc=12MHz, M=12, N=1 => Fcco=288MHz, 
	CCLK=288/4=72MHz, PCLK=CCLK/4=18MHz*/
#define Fosc	12000000
#define Fcclk	72000000
#define Fcco	288000000
#define Fpclk	18000000


#ifdef __cplusplus
   }
#endif
 
#endif /* end __TARGET_H */
/******************************************************************************
**                            End Of File
******************************************************************************/
