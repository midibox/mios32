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
