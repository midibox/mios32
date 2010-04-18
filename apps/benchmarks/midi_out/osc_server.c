// $Id$
/*
 * OSC daemon/server for uIP
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
#include <string.h>

#include "uip.h"
#include "uip_task.h"

#include "osc_server.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static struct uip_udp_conn *osc_conn = NULL;

static mios32_osc_search_tree_t parse_root[];

static u8 *osc_send_packet;
static u32 osc_send_len;

static u32 osc_remote_ip = OSC_REMOTE_IP;
static u16 osc_remote_port = OSC_SERVER_PORT;

static u8 running_sysex;

/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_Init(u32 mode)
{
  // disable sysex status
  running_sysex = 0;

  // disable send packet
  osc_send_packet = NULL;

  // remove open connection
  if(osc_conn != NULL) {
    uip_udp_remove(osc_conn);
  }

  // create new connection
  uip_ipaddr_t ripaddr;
  uip_ipaddr(ripaddr,
	     ((osc_remote_ip)>>24) & 0xff,
	     ((osc_remote_ip)>>16) & 0xff,
	     ((osc_remote_ip)>> 8) & 0xff,
	     ((osc_remote_ip)>> 0) & 0xff);

  if( (osc_conn=uip_udp_new(&ripaddr, HTONS(osc_remote_port))) != NULL ) {
    uip_udp_bind(osc_conn, HTONS(osc_remote_port));
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] listen to %d.%d.%d.%d:%d\n", 
				 (osc_conn->ripaddr[0] >> 0) & 0xff,
				 (osc_conn->ripaddr[0] >> 8) & 0xff,
				 (osc_conn->ripaddr[1] >> 0) & 0xff,
				 (osc_conn->ripaddr[1] >> 8) & 0xff,
				 HTONS(osc_conn->rport));
#endif
  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] FAILED to create connection (no free ports)\n");
#endif
    return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set functions
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_RemoteIP_Set(u32 ip)
{
  osc_remote_ip = ip;
  return OSC_SERVER_Init(0);
}

u32 OSC_SERVER_RemoteIP_Get(void)
{
  return osc_remote_ip;
}


s32 OSC_SERVER_RemotePortSet(u16 port)
{
  osc_remote_port = port;
  return OSC_SERVER_Init(0);
}

u16 OSC_SERVER_RemotePortGet(void)
{
  return osc_remote_port;
}


/////////////////////////////////////////////////////////////////////////////
// Called by UIP when a new UDP datagram has been received
// (UIP_UDP_APPCALL has been configured accordingly in uip-conf.h)
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_AppCall(void)
{
  if( uip_poll() ) {
    // called each second or on request (-> uip_udp_periodic_conn)

    if( osc_send_packet != NULL ) {
      // send datagram
      uip_send(osc_send_packet, osc_send_len);
      osc_send_packet = NULL;
    }
  }

  if( uip_newdata() ) {
    // new UDP package has been received
#if DEBUG_VERBOSE_LEVEL >= 2
    MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] Received Datagram from %d.%d.%d.%d:%d (%d bytes)\n", 
				 (uip_udp_conn->ripaddr[0] >> 0) & 0xff,
				 (uip_udp_conn->ripaddr[0] >> 8) & 0xff,
				 (uip_udp_conn->ripaddr[1] >> 0) & 0xff,
				 (uip_udp_conn->ripaddr[1] >> 8) & 0xff,
				 HTONS(uip_udp_conn->rport),
				 uip_len);
    MIOS32_MIDI_SendDebugHexDump((u8 *)uip_appdata, uip_len);
#endif
    s32 status = MIOS32_OSC_ParsePacket((u8 *)uip_appdata, uip_len, parse_root);
    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] invalid OSC packet, status %d\n", status);
#endif
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Called by OSC client to send an UDP datagram
// To ensure proper mutex handling, functions inside the server have to
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_SendPacket(u8 *packet, u32 len)
{
  // exit immediately if length == 0
  if( len == 0 )
    return 0;

  // exit immediately if connection not ready
  if( osc_conn == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] dropped SendPacket due to missing connection\n");
#endif
    return -1; // connection not established
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] Send Datagram to %d.%d.%d.%d:%d (%d bytes)\n", 
			       (osc_conn->ripaddr[0] >> 0) & 0xff,
			       (osc_conn->ripaddr[0] >> 8) & 0xff,
			       (osc_conn->ripaddr[1] >> 0) & 0xff,
			       (osc_conn->ripaddr[1] >> 8) & 0xff,
			       HTONS(osc_conn->rport),
			       len);
  MIOS32_MIDI_SendDebugHexDump(packet, len);
#endif

  // take over exclusive access to UIP functions
  MUTEX_UIP_TAKE;

  // store pointer and len in global variable, so that OSC_SERVER_AppCall() can take over
  osc_send_packet = packet;
  osc_send_len = len;

  // force processing for a connection
  // this will call OSC_SERVER_AppCall() with uip_poll() set
  // note: the packet cannot be send directly from here, we have to use the uIP framework!
  uip_udp_periodic_conn(osc_conn);

  // send packet immediately
  if(uip_len > 0) {
    uip_arp_out();
    network_device_send();
    uip_len = 0;
  }

  // release exclusive access to UIP functions
  MUTEX_UIP_GIVE;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Method to send a MIDI message
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_SendMIDI(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // check for MIDI event
  if( osc_args->arg_type[0] != 'm' )
    return -2; // wrong argument type for first parameter

  mios32_midi_package_t p = MIOS32_OSC_GetMIDI(osc_args->arg_ptr[0]);

  // extra treatment for SysEx messages
  if( p.evnt0 >= 0xf0 || p.evnt0 < 0x80 ) {
    if( p.evnt0 >= 0xf8 )
      p.cin = 0xf; // realtime
    if( p.evnt0 == 0xf7 ) {
      p.cin = 5; // SysEx ends with single byte
      running_sysex = 0;
    } else if( p.evnt1 == 0xf7 ) {
      p.cin = 6; // SysEx ends with two bytes
      running_sysex = 0;
    } else if( p.evnt2 == 0xf7 ) {
      p.cin = 7; // SysEx ends with three bytes
      running_sysex = 0;
    } else if( p.evnt0 == 0xf0 ) {
      p.cin = 4; // SysEx starts or continues
      running_sysex = 1;
    } else {
      running_sysex = 0;
      if( p.evnt0 == 0xf1 || p.evnt0 == 0xf3 )
	p.cin = 2; // two byte system common message
      else
	p.cin = 3; // three byte system common message
    }
  } else {
    p.cin = p.evnt0 >> 4;
    running_sysex = 0;
  }

  // port is located in method argument
  MIOS32_MIDI_SendPackage(method_arg, p);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////



static mios32_osc_search_tree_t parse_root[] = {
  { "midi", NULL, &OSC_SERVER_Method_SendMIDI, 0x00000000 },

  { "midi_usb0", NULL, &OSC_SERVER_Method_SendMIDI, USB0 },
  { "midi_usb1", NULL, &OSC_SERVER_Method_SendMIDI, USB1 },

  { "midi_uart0", NULL, &OSC_SERVER_Method_SendMIDI, UART0 },
  { "midi_uart1", NULL, &OSC_SERVER_Method_SendMIDI, UART1 },

  { NULL, NULL, NULL, 0 } // terminator
};
