// $Id: uip_task.h 387 2009-03-04 23:15:36Z tk $
/*
 * Header file for uIP Task
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _UIP_TASK_H
#define _UIP_TASK_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Ethernet configuration
// can be overruled in mios32_config.h

#ifndef MY_IP_ADDRESS
//                      10        .    0        .    0       .    3
#define MY_IP_ADDRESS ( 10 << 24) | (  0 << 16) | (  0 << 8) | (  3 << 0)
#endif

#ifndef MY_NETMASK
//                     255        .  255        .  255       .    0
#define MY_NETMASK    (255 << 24) | (255 << 16) | (255 << 8) | (  0 << 0)
#endif

#ifndef MY_GATEWAY
//                      10        .    0        .    0       .    1
#define MY_GATEWAY    (  0 << 24) | (  0 << 16) | (  0 << 8) | (  1 << 0)
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 UIP_TASK_Init(u32 mode);


#include <FreeRTOS.h>
#include <semphr.h>

extern xSemaphoreHandle xUIPSemaphore;
# define MUTEX_UIP_TAKE { while( xSemaphoreTake(xUIPSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_UIP_GIVE { xSemaphoreGive(xUIPSemaphore); }


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _UIP_TASK_H */
