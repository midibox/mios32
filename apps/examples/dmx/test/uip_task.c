// $Id: uip_task.c 387 2009-03-04 23:15:36Z tk $
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

#include "uip.h"
#include "uip_arp.h"
#include "network-device.h"
#include "timer.h"

#include "uip_task.h"


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

// lower priority than MIOS32 hooks
#define PRIORITY_TASK_UIP		( tskIDLE_PRIORITY + 2 )


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



/////////////////////////////////////////////////////////////////////////////
// Initialize the uIP task
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet
    
  xUIPSemaphore = xSemaphoreCreateMutex();
  
  xTaskCreate(UIP_TASK_Handler, (signed portCHAR *)"uIP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_UIP, NULL);

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

  // take over exclusive access to UIP functions
  MUTEX_UIP_TAKE;

  // init uIP timers
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  // init the network driver
  network_device_init();

  // init uIP
  uip_init();

  // set my ethernet address
  unsigned char *mac_addr = network_device_mac_addr();
  {
    int i;
    for(i=0; i<6; ++i)
      uip_ethaddr.addr[i] = mac_addr[i];
  }
  uip_arp_init();

  dhcpc_init(uip_ethaddr.addr, sizeof(uip_ethaddr.addr));
  //dhcpc_request();
  /*
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
 */
  // release exclusive access to UIP functions
  MUTEX_UIP_GIVE;

  // endless loop
  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // take over exclusive access to UIP functions
    MUTEX_UIP_TAKE;

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

    // release exclusive access to UIP functions
    MUTEX_UIP_GIVE;

  }
}




void uip_log(char *msg)
{
  MIOS32_MIDI_SendDebugMessage(msg);
}
