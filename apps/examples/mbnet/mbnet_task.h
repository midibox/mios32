// $Id$
/*
 * Header file for MBNet Task
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNET_TASK_H
#define _MBNET_TASK_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


#include <FreeRTOS.h>
#include <semphr.h>

extern xSemaphoreHandle xMBNETSemaphore;
# define MUTEX_MBNET_TAKE { while( xSemaphoreTake(xMBNETSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MBNET_GIVE { xSemaphoreGive(xMBNETSemaphore); }


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNET_TASK_Init(u32 mode);

extern s32 MBNET_TASK_PatchWrite8(u8 sid, u16 addr, u8 value);
extern s32 MBNET_TASK_PatchWrite16(u8 sid, u16 addr, u16 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNET_TASK_H */
