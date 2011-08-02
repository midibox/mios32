// $Id$
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

#include <mios32.h>
#include <FreeRTOS.h>
#include <semphr.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Ethernet configuration
// can be overruled in mios32_config.h

// By default the IP will be configured via DHCP
// By defining DONT_USE_DHCP we can optionally use static addresses

#ifndef MY_IP_ADDRESS
//                      192        .  168        .    2       .  100
#define MY_IP_ADDRESS (192 << 24) | (168 << 16) | (  2 << 8) | (100 << 0)
#endif

#ifndef MY_NETMASK
//                      255        .  255        .  255       .    0
#define MY_NETMASK    (255 << 24) | (255 << 16) | (255 << 8) | (  0 << 0)
#endif

#ifndef MY_GATEWAY
//                      192        .  168        .    2       .    1
#define MY_GATEWAY    (192 << 24) | (168 << 16) | (  2 << 8) | (  1 << 0)
#endif


// UDP monitor levels
// we assign names to the numbers for better readablilty
#define UDP_MONITOR_LEVEL_0_OFF              0
#define UDP_MONITOR_LEVEL_1_OSC_REC          1
#define UDP_MONITOR_LEVEL_2_OSC_REC_AND_SEND 2
#define UDP_MONITOR_LEVEL_3_ALL_GEQ_1024     3
#define UDP_MONITOR_LEVEL_4_ALL              4


#define UDP_MONITOR_SEND                     0
#define UDP_MONITOR_RECEIVED                 1



/////////////////////////////////////////////////////////////////////////////
// Semaphores
/////////////////////////////////////////////////////////////////////////////

// optionally, can be assigned to a mutex used by application in mios32_config.h
#ifndef UIP_TASK_MUTEX_MIDIOUT_TAKE
#define UIP_TASK_MUTEX_MIDIOUT_TAKE { }
#endif
#ifndef UIP_TASK_MUTEX_MIDIOUT_GIVE
#define UIP_TASK_MUTEX_MIDIOUT_GIVE { }
#endif

#ifndef UIP_TASK_MUTEX_MIDIIN_TAKE
#define UIP_TASK_MUTEX_MIDIIN_TAKE { }
#endif
#ifndef UIP_TASK_MUTEX_MIDIIN_GIVE
#define UIP_TASK_MUTEX_MIDIIN_GIVE { }
#endif


// this semaphore is defined by uip_task itself

#define MUTEX_UIP_TAKE { while( xSemaphoreTakeRecursive(xUIPSemaphore, (portTickType)1) != pdTRUE ); }
#define MUTEX_UIP_GIVE { xSemaphoreGiveRecursive(xUIPSemaphore); }


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 UIP_TASK_Init(u32 mode);
extern s32 UIP_TASK_InitFromPresets(u8 _dhcp_enabled, u32 _my_ip_address, u32 _my_netmask, u32 _my_gateway);

extern s32 UIP_TASK_NetworkDeviceAvailable(void);
extern s32 UIP_TASK_ServicesRunning(void);

extern s32 UIP_TASK_AppCall(void);

extern s32 UIP_TASK_UDP_AppCall(void);
extern s32 UIP_TASK_UDP_MonitorPacket(u8 received, char* prefix);
extern s32 UIP_TASK_UDP_MonitorLevelSet(u8 level);
extern s32 UIP_TASK_UDP_MonitorLevelGet(void);

extern s32 UIP_TASK_DHCP_EnableSet(u8 _dhcp_enabled);
extern s32 UIP_TASK_DHCP_EnableGet(void);

extern s32 UIP_TASK_IP_AddressSet(u32 ip);
extern s32 UIP_TASK_IP_AddressGet(void);
extern s32 UIP_TASK_NetmaskSet(u32 mask);
extern s32 UIP_TASK_NetmaskGet(void);
extern s32 UIP_TASK_GatewaySet(u32 ip);
extern s32 UIP_TASK_GatewayGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern xSemaphoreHandle xUIPSemaphore;


#endif /* _UIP_TASK_H */
