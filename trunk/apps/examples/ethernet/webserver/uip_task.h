// $Id: uip_task.h 817 2010-01-09 22:57:32Z tk $
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

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Ethernet configuration
// can be overruled in mios32_config.h

// By default the IP will be configured via DHCP
// By defining DONT_USE_DHCP we can optionally use static addresses

#ifdef DONT_USE_DHCP

# ifndef MY_IP_ADDRESS
//                      192        .  168        .    2       .  100
# define MY_IP_ADDRESS (192 << 24) | (168 << 16) | (  2 << 8) | (100 << 0)
# endif

# ifndef MY_NETMASK
//                      255        .  255        .  255       .    0
# define MY_NETMASK    (255 << 24) | (255 << 16) | (255 << 8) | (  0 << 0)
# endif

# ifndef MY_GATEWAY
//                      192        .  168        .    2       .    1
# define MY_GATEWAY    (192 << 24) | (168 << 16) | (  2 << 8) | (  1 << 0)
# endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 UIP_TASK_Init(u32 mode);
extern s32 UIP_TASK_ServicesRunning(void);

extern s32 UIP_TASK_UDP_AppCall(void);



struct udp_state {
  struct pt pt;
  char state;
  struct uip_udp_conn *conn;
  struct timer timer;
  u32 ticks;
  const void *mac_addr;
  int mac_len;
  
  u8_t serverid[4];

  u16_t lease_time[2];
  u16_t ipaddr[2];
  u16_t netmask[2];
  u16_t dnsaddr[2];
  u16_t default_router[2];

  // data required to keep track of the exact time
  struct ntp_time last_sync;
  ntp_adjust_t adjust;
    
};
 
void dhcpc_configured(const struct udp_state *s);

typedef struct udp_state uip_udp_appstate_t;

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _UIP_TASK_H */
