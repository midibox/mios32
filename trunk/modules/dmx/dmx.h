/*
 * Header file for MBHP_DMX module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Phil Taylor (phil@taylor.org.uk)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _DMX_H
#define _DMX_H

#define DMX_UNIVERSE_SIZE	512
#define DMX_BAUDRATE 250000
#define BREAK_BAUDRATE 40000	// Baud rate used for sending a nice long break!
//	ANSI Spec: Break: 176uS-352uS MAB 12uS-88uS IB-Gab < 32uS */
// Baudrate 30000 (measured with scope): Break ca. 200 uS, MAB ca. 80 uS

#define DMX_IDLE	0
#define DMX_BREAK	1
#define DMX_SENDING	2

#define DMX_TX_PORT     GPIOA
#define DMX_TX_PIN      GPIO_Pin_9
#define DMX_RX_PORT     GPIOA
#define DMX_RX_PIN      GPIO_Pin_10
#define DMX             USART1
#define DMX_IRQ_CHANNEL USART1_IRQn
#define DMX_IRQHANDLER_FUNC void USART1_IRQHandler(void)


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////
s32 DMX_Init(u32 mode);
s32 DMX_SetChannel(u16 channel, u8 value);
s32 DMX_GetChannel(u16 channel);



#endif /* _DMX_H */
