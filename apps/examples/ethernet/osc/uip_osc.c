// $Id$
/*
 * OSC daemon for uIP
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

#include "uip_osc.h"
#include "uip.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 2


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static struct uip_udp_conn *osc_conn = NULL;


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 UIP_OSC_Init(u32 mode)
{
  if(osc_conn != NULL) {
    uip_udp_remove(osc_conn);
  }

  uip_ipaddr_t ripaddr;
  uip_ipaddr(ripaddr, 10,0,0,1);

  if( (osc_conn=uip_udp_new(&ripaddr, HTONS(UIP_OSC_PORT))) != NULL ) {
    uip_udp_bind(osc_conn, HTONS(UIP_OSC_PORT));
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[UIP_OSC] listen to %d.%d.%d.%d:%d\n", 
				 (osc_conn->ripaddr[0] >> 0) & 0xff,
				 (osc_conn->ripaddr[0] >> 8) & 0xff,
				 (osc_conn->ripaddr[1] >> 0) & 0xff,
				 (osc_conn->ripaddr[1] >> 8) & 0xff,
				 osc_conn->rport);
#endif
  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[UIP_OSC] FAILED to create connection (no free ports)\n");
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by UIP when a new UDP package has been received
// (UIP_UDP_APPCALL has been configured accordingly in uip-conf.h)
/////////////////////////////////////////////////////////////////////////////
s32 UIP_OSC_AppCall(void)
{
  if( uip_udp_conn->rport == HTONS(UIP_OSC_PORT) ) {
    if( uip_poll() ) {
      // called each second
    }

    if( uip_newdata() ) {
      // new UDP package has been received
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("Received Datagram from %d.%d.%d.%d:%d (%d bytes)\n", 
				   (uip_udp_conn->ripaddr[0] >> 0) & 0xff,
				   (uip_udp_conn->ripaddr[0] >> 8) & 0xff,
				   (uip_udp_conn->ripaddr[1] >> 0) & 0xff,
				   (uip_udp_conn->ripaddr[1] >> 8) & 0xff,
				   uip_udp_conn->rport,
				   uip_len);
      MIOS32_MIDI_SendDebugHexDump((u8 *)uip_appdata, uip_len);
#endif
    }
  }
}
