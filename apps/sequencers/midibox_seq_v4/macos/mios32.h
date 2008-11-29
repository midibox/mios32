// $Id$
/*
 * Header file for MIOS32 compatibility layer of MacOS
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_H
#define _MIOS32_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// include local config file
// (see MIOS32_CONFIG.txt for available switches)
/////////////////////////////////////////////////////////////////////////////

#include "mios32_config.h"


/////////////////////////////////////////////////////////////////////////////
// include C headers
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// comaptible with stm32f10x_type.h
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;

typedef signed long  const sc32;  /* Read Only */
typedef signed short const sc16;  /* Read Only */
typedef signed char  const sc8;   /* Read Only */

typedef volatile signed long  vs32;
typedef volatile signed short vs16;
typedef volatile signed char  vs8;

typedef volatile signed long  const vsc32;  /* Read Only */
typedef volatile signed short const vsc16;  /* Read Only */
typedef volatile signed char  const vsc8;   /* Read Only */

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef unsigned long  const uc32;  /* Read Only */
typedef unsigned short const uc16;  /* Read Only */
typedef unsigned char  const uc8;   /* Read Only */

typedef volatile unsigned long  vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char  vu8;

typedef volatile unsigned long  const vuc32;  /* Read Only */
typedef volatile unsigned short const vuc16;  /* Read Only */
typedef volatile unsigned char  const vuc8;   /* Read Only */

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

s32 MIOS32_LCD_Init(u32 mode);
s32 MIOS32_LCD_DeviceSet(u8 device);
u8  MIOS32_LCD_DeviceGet(void);
s32 MIOS32_LCD_CursorSet(u16 column, u16 line);
s32 MIOS32_LCD_Clear(void);
s32 MIOS32_LCD_PrintChar(char c);
s32 MIOS32_LCD_PrintString(char *str);
s32 MIOS32_LCD_PrintFormattedString(char *format, ...);

s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8]);
s32 MIOS32_LCD_SpecialCharsInit(u8 table[64]);

s32 MIOS32_COM_SendChar(u8 port, char c);

s32 MIOS32_DOUT_PinSet(u32 pin, u32 value);
s32 MIOS32_DOUT_SRSet(u32 sr, u8 value);

s32 MIOS32_BOARD_LED_Init(u32 leds);
s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value);
u32 MIOS32_BOARD_LED_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* _MIOS32_H */
