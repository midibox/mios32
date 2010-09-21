// $Id$
/*
 * uIP handler as FreeRTOS task
 *
 * Framework taken from $MIOS32_PATH/modules/uip/doc/example-mainloop-with-arp.c
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <mios32.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "tasks.h"

#include "app.h"
#include <seq_file.h>
#include <seq_file_hw.h>

#include "uip.h"
#include "uip_arp.h"
#include "network-device.h"
#include "timer.h"

#include "uip_task.h"

#include "osc_server.h"
#include "dhcpc.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_UIP		( tskIDLE_PRIORITY + 3 )


// for mutual exclusive access to uIP functions
// The mutex is handled with MUTEX_UIP_TAKE and MUTEX_UIP_GIVE macros
xSemaphoreHandle xUIPSemaphore;


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void UIP_TASK_Handler(void *pvParameters);
static s32 UIP_TASK_StartServices(void);
static s32 UIP_TASK_StopServices(void);
static s32 UIP_TASK_SendDebugMessage_IP(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 services_running;
static u8 dhcp_enabled = 1;
static u32 my_ip_address = MY_IP_ADDRESS;
static u32 my_netmask = MY_NETMASK;
static u32 my_gateway = MY_GATEWAY;


/////////////////////////////////////////////////////////////////////////////
// Initialize the uIP task
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

  xUIPSemaphore = xSemaphoreCreateRecursiveMutex();

  xTaskCreate(UIP_TASK_Handler, (signed portCHAR *)"uIP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_UIP, NULL);

  services_running = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Init function for presets (read before UIP_TASKS_Init()
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_InitFromPresets(u8 _dhcp_enabled, u32 _my_ip_address, u32 _my_netmask, u32 _my_gateway)
{
  dhcp_enabled = _dhcp_enabled;
  my_ip_address = _my_ip_address;
  my_netmask = _my_netmask;
  my_gateway = _my_gateway;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// The uIP Task is executed each mS
/////////////////////////////////////////////////////////////////////////////
static void UIP_TASK_Handler(void *pvParameters)
{
  int i;
  struct timer periodic_timer, arp_timer;

  // Initialise the xLastExecutionTime variable on task entry
  portTickType xLastExecutionTime = xTaskGetTickCount();

  // take over exclusive access to UIP functions
  MUTEX_UIP_TAKE;

  // init uIP timers
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  // init the network driver
  network_device_init();

  // init uIP
  uip_init();
  uip_arp_init();

  // set my ethernet address
  unsigned char *mac_addr = network_device_mac_addr();
  {
    int i;
    for(i=0; i<6; ++i)
      uip_ethaddr.addr[i] = mac_addr[i];
  }

  // enable dhcp mode (can be changed during runtime)
  UIP_TASK_DHCP_EnableSet(dhcp_enabled);

  // release exclusive access to UIP functions
  MUTEX_UIP_GIVE;

  // wait until HW config has been loaded
  do {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
  } while( !SEQ_FILE_HW_ConfigLocked() );

  // endless loop
  while( 1 ) {
    do {
      vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
    } while( TASK_MSD_EnableGet() ); // don't service ethernet if MSD mode enabled for faster transfer speed

    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;

    if( !(clock_time_tick() % 100) ) {
      // each 100 mS: check availablility of network device
      //network_device_check();
      // TK: no auto-detection for MBSEQ for best performance if no MBHP_ETH module connected
      // the user has to reboot MBSEQ to restart module detection
    }

    if( network_device_available() ) {
      uip_len = network_device_read();

      if( uip_len > 0 ) {
	if(BUF->type == htons(UIP_ETHTYPE_IP) ) {
	  uip_arp_ipin();
	  uip_input();
	
	  /* If the above function invocation resulted in data that
	     should be sent out on the network, the global variable
	     uip_len is set to a value > 0. */
	  if( uip_len > 0 ) {
	    uip_arp_out();
	    network_device_send();
	  }
	} else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
	  uip_arp_arpin();
	  /* If the above function invocation resulted in data that
	     should be sent out on the network, the global variable
	     uip_len is set to a value > 0. */
	  if(uip_len > 0) {
	    network_device_send();
	  }
	}

      } else if(timer_expired(&periodic_timer)) {
	timer_reset(&periodic_timer);
	for(i = 0; i < UIP_CONNS; i++) {
	  uip_periodic(i);
	  /* If the above function invocation resulted in data that
	     should be sent out on the network, the global variable
	     uip_len is set to a value > 0. */
	  if(uip_len > 0) {
	    uip_arp_out();
	    network_device_send();
	  }
	}

#if UIP_UDP
	for(i = 0; i < UIP_UDP_CONNS; i++) {
	  uip_udp_periodic(i);
	  /* If the above function invocation resulted in data that
	     should be sent out on the network, the global variable
	     uip_len is set to a value > 0. */
	  if(uip_len > 0) {
	    uip_arp_out();
	    network_device_send();
	  }
	}
#endif /* UIP_UDP */
      
	/* Call the ARP timer function every 10 seconds. */
	if(timer_expired(&arp_timer)) {
	  timer_reset(&arp_timer);
	  uip_arp_timer();
	}
      }
    }

    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;
  }
}


/////////////////////////////////////////////////////////////////////////////
// used by uIP to print a debug message
/////////////////////////////////////////////////////////////////////////////
void uip_log(char *msg)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG(msg);
  MUTEX_MIDIOUT_GIVE;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Called by UDP handler of uIP
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_AppCall(void)
{
  // no TCP service used yet...
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Prints current IP settings
/////////////////////////////////////////////////////////////////////////////
static s32 UIP_TASK_SendDebugMessage_IP(void)
{
  uip_ipaddr_t ipaddr;
  uip_gethostaddr(&ipaddr);

#if DEBUG_VERBOSE_LEVEL >= 1
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("[UIP_TASK] IP address: %d.%d.%d.%d\n",
	    uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
	    uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));

  uip_ipaddr_t netmask;
  uip_getnetmask(&netmask);
  DEBUG_MSG("[UIP_TASK] Netmask: %d.%d.%d.%d\n",
	    uip_ipaddr1(netmask), uip_ipaddr2(netmask),
	    uip_ipaddr3(netmask), uip_ipaddr4(netmask));

  uip_ipaddr_t draddr;
  uip_getdraddr(&draddr);
  DEBUG_MSG("[UIP_TASK] Default Router (Gateway): %d.%d.%d.%d\n",
	    uip_ipaddr1(draddr), uip_ipaddr2(draddr),
	    uip_ipaddr3(draddr), uip_ipaddr4(draddr));
  MUTEX_MIDIOUT_GIVE;
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Configure DHCP mode (can be changed during runtime)
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_DHCP_EnableSet(u8 _dhcp_enabled)
{
  uip_ipaddr_t ipaddr;

  dhcp_enabled = _dhcp_enabled;

  // branch depending on DHCP mode
  if( dhcp_enabled ) {
    // stop all services, will be started once we got the IP
    UIP_TASK_StopServices();

    // IP address/netmask/router must be 0
    uip_ipaddr(ipaddr, 0x00, 0x00, 0x00, 0x00);
    uip_sethostaddr(ipaddr);
    uip_setnetmask(ipaddr);
    uip_setdraddr(ipaddr);

    dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
#if DEBUG_VERBOSE_LEVEL >= 1
    MUTEX_MIDIOUT_TAKE;
    DEBUG_MSG("[UIP_TASK] DHCP Client requests the IP settings...\n");
    MUTEX_MIDIOUT_GIVE;
#endif
  } else {
    // set my IP address
    uip_ipaddr(ipaddr,
	       ((my_ip_address)>>24) & 0xff,
	       ((my_ip_address)>>16) & 0xff,
	       ((my_ip_address)>> 8) & 0xff,
	       ((my_ip_address)>> 0) & 0xff);
    uip_sethostaddr(ipaddr);

    // set my netmask
    uip_ipaddr(ipaddr,
	       ((my_netmask)>>24) & 0xff,
	       ((my_netmask)>>16) & 0xff,
	       ((my_netmask)>> 8) & 0xff,
	       ((my_netmask)>> 0) & 0xff);
    uip_setnetmask(ipaddr);

    // default router
    uip_ipaddr(ipaddr,
	       ((my_gateway)>>24) & 0xff,
	       ((my_gateway)>>16) & 0xff,
	       ((my_gateway)>> 8) & 0xff,
	       ((my_gateway)>> 0) & 0xff);
    uip_setdraddr(ipaddr);

#if DEBUG_VERBOSE_LEVEL >= 1
    MUTEX_MIDIOUT_TAKE;
    DEBUG_MSG("[UIP_TASK] IP Address statically set:\n");
    MUTEX_MIDIOUT_GIVE;
#endif

    // start services immediately
    UIP_TASK_StartServices();
  }

  return 0; // no error
}

s32 UIP_TASK_DHCP_EnableGet(void)
{
  return dhcp_enabled;
}


/////////////////////////////////////////////////////////////////////////////
// Set/Get IP values
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_IP_AddressSet(u32 ip)
{
  uip_ipaddr_t ipaddr;

  my_ip_address = ip;
  uip_ipaddr(ipaddr,
	     ((ip)>>24) & 0xff,
	     ((ip)>>16) & 0xff,
	     ((ip)>> 8) & 0xff,
	     ((ip)>> 0) & 0xff);
  uip_sethostaddr(ipaddr);

  return 0; // no error
}

s32 UIP_TASK_IP_AddressGet(void)
{
  return my_ip_address;
}


s32 UIP_TASK_NetmaskSet(u32 mask)
{
  uip_ipaddr_t ipaddr;

  my_netmask = mask;
  uip_ipaddr(ipaddr,
	     ((mask)>>24) & 0xff,
	     ((mask)>>16) & 0xff,
	     ((mask)>> 8) & 0xff,
	     ((mask)>> 0) & 0xff);
  uip_setnetmask(ipaddr);

  return 0; // no error
}

s32 UIP_TASK_NetmaskGet(void)
{
  return my_netmask;
}


s32 UIP_TASK_GatewaySet(u32 ip)
{
  uip_ipaddr_t ipaddr;

  my_gateway = ip;
  uip_ipaddr(ipaddr,
	     ((ip)>>24) & 0xff,
	     ((ip)>>16) & 0xff,
	     ((ip)>> 8) & 0xff,
	     ((ip)>> 0) & 0xff);
  uip_setdraddr(ipaddr);

  return 0; // no error
}

s32 UIP_TASK_GatewayGet(void)
{
  return my_gateway;
}


/////////////////////////////////////////////////////////////////////////////
// start services
/////////////////////////////////////////////////////////////////////////////
static s32 UIP_TASK_StartServices(void)
{
  // print IP settings
  MUTEX_MIDIOUT_TAKE;
  UIP_TASK_SendDebugMessage_IP();
  MUTEX_MIDIOUT_GIVE;

  // start OSC daemon
  OSC_SERVER_Init(0);

  // services available now
  services_running = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// stop services
/////////////////////////////////////////////////////////////////////////////
static s32 UIP_TASK_StopServices(void)
{
  // stop all services
  services_running = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Status flag for external functions
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_ServicesRunning(void)
{
  return services_running;
}


/////////////////////////////////////////////////////////////////////////////
// network device connected to core?
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_NetworkDeviceAvailable(void)
{
  return network_device_available();
}


/////////////////////////////////////////////////////////////////////////////
// Called by UDP handler of uIP
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_UDP_AppCall(void)
{
  // DHCP client
  if( uip_udp_conn->rport == HTONS(DHCPC_SERVER_PORT) || uip_udp_conn->rport == HTONS(DHCPC_CLIENT_PORT) ) {
    dhcpc_appcall();

  // OSC Server checks for IP/port locally
  } else {
    OSC_SERVER_AppCall();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by DHCP client once it got IP addresses
/////////////////////////////////////////////////////////////////////////////
void dhcpc_configured(const struct dhcpc_state *s)
{
  // set IP settings
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router);

  // start services
  UIP_TASK_StartServices();

  // print unused settings
#if DEBUG_VERBOSE_LEVEL >= 1
  MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("[UIP_TASK] Got DNS server %d.%d.%d.%d\n",
	    uip_ipaddr1(s->dnsaddr), uip_ipaddr2(s->dnsaddr),
	    uip_ipaddr3(s->dnsaddr), uip_ipaddr4(s->dnsaddr));
  DEBUG_MSG("[UIP_TASK] Lease expires in %d hours\n",
	    (ntohs(s->lease_time[0])*65536ul + ntohs(s->lease_time[1]))/3600);
  MUTEX_MIDIOUT_GIVE;
#endif
}
