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

#include "uip.h"
#include "uip_arp.h"
#include "network-device.h"
#include "timer.h"


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



/////////////////////////////////////////////////////////////////////////////
// Initialize the uIP task
/////////////////////////////////////////////////////////////////////////////
s32 UIP_TASK_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

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

  // init uIP timers
  timer_set(&periodic_timer, CLOCK_SECOND / 2);
  timer_set(&arp_timer, CLOCK_SECOND * 10);

  // init the network driver
  network_device_init();

  // init uIP
  uip_init();

  // set my ethernet address
  uip_ethaddr.addr[0] = MIOS32_ENC28J60_MY_MAC_ADDR1;
  uip_ethaddr.addr[1] = MIOS32_ENC28J60_MY_MAC_ADDR2;
  uip_ethaddr.addr[2] = MIOS32_ENC28J60_MY_MAC_ADDR3;
  uip_ethaddr.addr[3] = MIOS32_ENC28J60_MY_MAC_ADDR4;
  uip_ethaddr.addr[4] = MIOS32_ENC28J60_MY_MAC_ADDR5;
  uip_ethaddr.addr[5] = MIOS32_ENC28J60_MY_MAC_ADDR6;

  // set my IP address
  uip_ipaddr(ipaddr, 10,0,0,3);
  uip_sethostaddr(ipaddr);
  uip_ipaddr(ipaddr, 255,255,255,0);
  uip_setnetmask(ipaddr);

  // default router
  uip_ipaddr(ipaddr, 10,0,0,1);
  uip_setdraddr(ipaddr);

  // start telnet daemon
  telnetd_init();

  // start OSC daemon
  UIP_OSC_Init(0);

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




void uip_log(char *msg)
{
  MIOS32_MIDI_SendDebugMessage(msg);
}


