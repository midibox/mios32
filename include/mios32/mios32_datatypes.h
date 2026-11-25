// $Id$
/*
 * Defines data types for 32bit processors
 * compatible with stm32f10x.h
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_DATATYPES_H
#define _MIOS32_DATATYPES_H

// following check to ensure that typedefs won't be declared again from stm32f10x.h
#if defined(MIOS32_FAMILY_EMULATION)
// host build (apps/sequencers/midibox_seq_v4/emu_macos): 'long' is 64 bit on the host, but MIOS32 and the files the
// firmware writes (e.g. seq_song_step_t) rely on 32 bit - use fixed width types
#include <stdint.h>
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;
typedef const int32_t  sc32;
typedef const int16_t  sc16;
typedef const int8_t   sc8;
typedef volatile int32_t  vs32;
typedef volatile int16_t  vs16;
typedef volatile int8_t   vs8;
typedef const volatile int32_t  vsc32;
typedef const volatile int16_t  vsc16;
typedef const volatile int8_t   vsc8;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef const uint32_t  uc32;
typedef const uint16_t  uc16;
typedef const uint8_t   uc8;
typedef volatile uint32_t  vu32;
typedef volatile uint16_t  vu16;
typedef volatile uint8_t   vu8;
typedef const volatile uint32_t  vuc32;
typedef const volatile uint16_t  vuc16;
typedef const volatile uint8_t   vuc8;
#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)
#elif !defined(__STM32F10x_H) && !defined(__STM32F4xx_H)

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

#endif

#endif /* _MIOS32_DATATYPES_H */

