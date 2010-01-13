// $Id: uip_task.c 817 2010-01-09 22:57:32Z tk $
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

#include "pt.h"
#include "timer.h"

#include "uip.h"
#include "uip_arp.h"
#include "dhcpc.h"
#include "ntpclient.h"
#include "uip_task.h"

#include "network-device.h"


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

// lower priority than MIOS32 hooks
#define PRIORITY_TASK_UIP		( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void UIP_TASK_Handler(void *pvParameters);
static s32 UIP_TASK_StartServices(void);
static s32 UIP_TASK_SendDebugMessage_IP(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 services_running;


/////////////////////////////////////////////////////////////////////////////
// Initialize the uIP task
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

  xTaskCreate(UIP_TASK_Handler, (signed portCHAR *)"uIP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_UIP, NULL);

  services_running = 0;

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// The uIP Task is executed each mS
/////////////////////////////////////////////////////////////////////////////
static void UIP_TASK_Handler(void *pvParameters)
{
  int i;
  uip_ipaddr_t ipaddr;
  struct timer periodic_timer, arp_timer;

  // Initialise the xLastExecutionTime variable on task entry
  portTickType xLastExecutionTime = xTaskGetTickCount();

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

#ifndef DONT_USE_DHCP
  dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] DHCP Client requests the IP settings...\n");
#else
  // set my IP address
  uip_ipaddr(ipaddr,
	     ((MY_IP_ADDRESS)>>24) & 0xff,
	     ((MY_IP_ADDRESS)>>16) & 0xff,
	     ((MY_IP_ADDRESS)>> 8) & 0xff,
	     ((MY_IP_ADDRESS)>> 0) & 0xff);
  uip_sethostaddr(ipaddr);

  // set my netmask
  uip_ipaddr(ipaddr,
	     ((MY_NETMASK)>>24) & 0xff,
	     ((MY_NETMASK)>>16) & 0xff,
	     ((MY_NETMASK)>> 8) & 0xff,
	     ((MY_NETMASK)>> 0) & 0xff);
  uip_setnetmask(ipaddr);

  // default router
  uip_ipaddr(ipaddr,
	     ((MY_GATEWAY)>>24) & 0xff,
	     ((MY_GATEWAY)>>16) & 0xff,
	     ((MY_GATEWAY)>> 8) & 0xff,
	     ((MY_GATEWAY)>> 0) & 0xff);
  uip_setdraddr(ipaddr);

  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] IP Address statically set:\n");

  // start services immediately
  UIP_TASK_StartServices();
#endif

  // endless loop
  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    if( !(clock_time_tick() % 100) ) {
      // each 100 mS: check availablility of network device
      network_device_check();
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
  }
}


/////////////////////////////////////////////////////////////////////////////
// used by uIP to print a debug message
/////////////////////////////////////////////////////////////////////////////
void uip_log(char *msg)
{
  MIOS32_MIDI_SendDebugMessage(msg);
}


/////////////////////////////////////////////////////////////////////////////
// Prints current IP settings
/////////////////////////////////////////////////////////////////////////////
static s32 UIP_TASK_SendDebugMessage_IP(void)
{
  uip_ipaddr_t ipaddr;
  uip_gethostaddr(&ipaddr);

  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] IP address: %d.%d.%d.%d\n",
			       uip_ipaddr1(ipaddr), uip_ipaddr2(ipaddr),
			       uip_ipaddr3(ipaddr), uip_ipaddr4(ipaddr));

  uip_ipaddr_t netmask;
  uip_getnetmask(&netmask);
  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] Netmask: %d.%d.%d.%d\n",
			       uip_ipaddr1(netmask), uip_ipaddr2(netmask),
			       uip_ipaddr3(netmask), uip_ipaddr4(netmask));

  uip_ipaddr_t draddr;
  uip_getdraddr(&draddr);
  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] Default Router (Gateway): %d.%d.%d.%d\n",
			       uip_ipaddr1(draddr), uip_ipaddr2(draddr),
			       uip_ipaddr3(draddr), uip_ipaddr4(draddr));

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// start services
/////////////////////////////////////////////////////////////////////////////
static s32 UIP_TASK_StartServices(void)
{
  // print IP settings
  UIP_TASK_SendDebugMessage_IP();

  // start NTP client (to set clock!)
  ntpclient_init();
  
  // start webserver
  httpd_init();

  
  // services available now
  services_running = 1;

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
// Called by UDP handler of uIP
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_UDP_AppCall(void)
{
  // DHCP client
  if( uip_udp_conn->rport == HTONS(DHCPC_SERVER_PORT) || uip_udp_conn->rport == HTONS(DHCPC_CLIENT_PORT) ) {
    dhcpc_appcall();
	// NTP Client
  } else if (uip_udp_conn->rport == HTONS(NTP_PORT)) {
    ntpclient_appcall();
	// DNS Client
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by DHCP client once it got IP addresses
/////////////////////////////////////////////////////////////////////////////
void dhcpc_configured(const struct udp_state *s)
{
  // set IP settings
  uip_sethostaddr(s->ipaddr);
  uip_setnetmask(s->netmask);
  uip_setdraddr(s->default_router);
  // start services
  UIP_TASK_StartServices();

  // print unused settings
  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] Got DNS server %d.%d.%d.%d\n",
			       uip_ipaddr1(s->dnsaddr), uip_ipaddr2(s->dnsaddr),
			       uip_ipaddr3(s->dnsaddr), uip_ipaddr4(s->dnsaddr));
  MIOS32_MIDI_SendDebugMessage("[UIP_TASK] Lease expires in %d hours\n",
			       (ntohs(s->lease_time[0])*65536ul + ntohs(s->lease_time[1]))/3600);
}


		
	