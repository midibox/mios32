/*
 * Header file for fram.c
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _FRAM_H
#define _FRAM_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called
// or write the defines into your mios32_config.h file
#include <mios32_config.h>

#ifndef FRAM_IIC_PORT
#define FRAM_IIC_PORT 1
#endif

#ifndef FRAM_MULTIPLEX_ENABLE
#define FRAM_MULTIPLEX_ENABLE 1
#endif

#if FRAM_MULTIPLEX_ENABLE==1

// defaults to J19:RC1
#ifndef FRAM_MULTIPLEX_PORT_MSB
#define FRAM_MULTIPLEX_PORT_MSB GPIOC
#define FRAM_MULTIPLEX_PIN_MSB GPIO_Pin_13
#endif

// defaults to J19:RC2
#ifndef FRAM_MULTIPLEX_PORT_LSB
#define FRAM_MULTIPLEX_PORT_LSB GPIOC
#define FRAM_MULTIPLEX_PIN_LSB GPIO_Pin_14
#endif

#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum{
  FRAM_Read,
  FRAM_Write
}FRAM_transfer_t

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 FRAM_Init(void);
extern s32 FRAM_Transfer(FRAM_transfer_t transfer_type, u8 device_addr, u32 mem_addr, u8 *buffer, u8 buffer_len);
extern s32 FRAM_CheckDevice(u8 device_addr);

#endif /* _FRAM_H */
