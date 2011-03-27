// $Id$
/*
 * Header file for UART functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_UART_H
#define _MIOS32_UART_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// temporary disabled for STM32 Primer due to pin conflicts
#if defined(MIOS32_BOARD_STM32_PRIMER)
# define MIOS32_UART_NUM 0
#elif defined(MIOS32_BOARD_LPCXPRESSO)
# define MIOS32_UART_NUM 1
#endif

// number of UART interfaces (0..3)
#ifndef MIOS32_UART_NUM
#define MIOS32_UART_NUM 2
#endif

// Tx buffer size (1..256)
#ifndef MIOS32_UART_TX_BUFFER_SIZE
#define MIOS32_UART_TX_BUFFER_SIZE 64
#endif

// Rx buffer size (1..256)
#ifndef MIOS32_UART_RX_BUFFER_SIZE
#define MIOS32_UART_RX_BUFFER_SIZE 64
#endif

// Baudrate of UART first interface
#ifndef MIOS32_UART0_BAUDRATE
#define MIOS32_UART0_BAUDRATE 31250
#endif

// should UART0 Tx pin configured for open drain (default) or push-pull mode?
#ifndef MIOS32_UART0_TX_OD
#define MIOS32_UART0_TX_OD 1
#endif

// Baudrate of UART second interface
#ifndef MIOS32_UART1_BAUDRATE
#define MIOS32_UART1_BAUDRATE 31250
#endif

// should UART1 Tx pin configured for open drain (default) or push-pull mode?
#ifndef MIOS32_UART1_TX_OD
#define MIOS32_UART1_TX_OD 1
#endif

// Baudrate of UART third interface
#ifndef MIOS32_UART2_BAUDRATE
#define MIOS32_UART2_BAUDRATE 31250
#endif

// should UART1 Tx pin configured for open drain (default) or push-pull mode?
#ifndef MIOS32_UART2_TX_OD
#define MIOS32_UART2_TX_OD 1
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, 2 = COM
#ifndef MIOS32_UART0_ASSIGNMENT
#define MIOS32_UART0_ASSIGNMENT 1
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, 2 = COM
#ifndef MIOS32_UART1_ASSIGNMENT
#define MIOS32_UART1_ASSIGNMENT 1
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, 2 = COM
#ifndef MIOS32_UART2_ASSIGNMENT
#define MIOS32_UART2_ASSIGNMENT 1
#endif

// Interface assignment: 0 = disabled, 1 = MIDI, 2 = COM
#ifndef MIOS32_UART3_ASSIGNMENT
#define MIOS32_UART3_ASSIGNMENT 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_UART_Init(u32 mode);

extern s32 MIOS32_UART_BaudrateSet(u8 uart, u32 baudrate);
extern u32 MIOS32_UART_BaudrateGet(u8 uart);

extern s32 MIOS32_UART_RxBufferFree(u8 uart);
extern s32 MIOS32_UART_RxBufferUsed(u8 uart);
extern s32 MIOS32_UART_RxBufferGet(u8 uart);
extern s32 MIOS32_UART_RxBufferPeek(u8 uart);
extern s32 MIOS32_UART_RxBufferPut(u8 uart, u8 b);
extern s32 MIOS32_UART_TxBufferFree(u8 uart);
extern s32 MIOS32_UART_TxBufferUsed(u8 uart);
extern s32 MIOS32_UART_TxBufferGet(u8 uart);
extern s32 MIOS32_UART_TxBufferPut_NonBlocking(u8 uart, u8 b);
extern s32 MIOS32_UART_TxBufferPut(u8 uart, u8 b);
extern s32 MIOS32_UART_TxBufferPutMore_NonBlocking(u8 uart, u8 *buffer, u16 len);
extern s32 MIOS32_UART_TxBufferPutMore(u8 uart, u8 *buffer, u16 len);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_UART_H */
