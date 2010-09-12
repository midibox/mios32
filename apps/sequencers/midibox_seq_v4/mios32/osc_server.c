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
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static struct uip_udp_conn *osc_conn[OSC_SERVER_NUM_CONNECTIONS];

static mios32_osc_search_tree_t parse_root[];

static u8 *osc_send_packet;
static u32 osc_send_len;

// TODO: variable initialisation contains hardcoded dependency to OSC_SERVER_NUM_CONNECTIONS!
static u32 osc_remote_ip[OSC_SERVER_NUM_CONNECTIONS] = { OSC_REMOTE_IP, OSC_REMOTE_IP, OSC_REMOTE_IP, OSC_REMOTE_IP };
static u16 osc_remote_port[OSC_SERVER_NUM_CONNECTIONS] = { OSC_REMOTE_PORT, OSC_REMOTE_PORT, OSC_REMOTE_PORT, OSC_REMOTE_PORT };
static u16 osc_local_port[OSC_SERVER_NUM_CONNECTIONS] = { OSC_LOCAL_PORT, OSC_LOCAL_PORT, OSC_LOCAL_PORT, OSC_LOCAL_PORT };

static u8 running_sysex;

/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_Init(u32 mode)
{
  int con;

  // disable sysex status
  running_sysex = 0;

  // disable send packet
  osc_send_packet = NULL;

  // remove open connections
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con)
    if( osc_conn[con] != NULL )
      uip_udp_remove(osc_conn[con]);

  // create new connections
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    uip_ipaddr_t ripaddr;
    uip_ipaddr(ripaddr,
	       ((osc_remote_ip[con])>>24) & 0xff,
	       ((osc_remote_ip[con])>>16) & 0xff,
	       ((osc_remote_ip[con])>> 8) & 0xff,
	       ((osc_remote_ip[con])>> 0) & 0xff);

    if( (osc_conn[con]=uip_udp_new(&ripaddr, HTONS(osc_remote_port[con]))) != NULL ) {
      uip_udp_bind(osc_conn[con], HTONS(osc_local_port[con]));
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] #%d sends to %d.%d.%d.%d:%d\n", 
				   con,
				   (osc_conn[con]->ripaddr[0] >> 0) & 0xff,
				   (osc_conn[con]->ripaddr[0] >> 8) & 0xff,
				   (osc_conn[con]->ripaddr[1] >> 0) & 0xff,
				   (osc_conn[con]->ripaddr[1] >> 8) & 0xff,
				   HTONS(osc_conn[con]->rport));
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] FAILED to create connection #%d (no free ports)\n", con);
#endif
      return -1;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Init function for presets (read before OSC_SERVER_Init()
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_InitFromPresets(u8 con, u32 _osc_remote_ip, u16 _osc_remote_port, u16 _osc_local_port)
{
  if( con >= OSC_SERVER_NUM_CONNECTIONS )
    return -1; // invalid connection

  osc_remote_ip[con] = _osc_remote_ip;
  osc_remote_port[con] = _osc_remote_port;
  osc_local_port[con] = _osc_local_port;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set functions
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_RemoteIP_Set(u8 con, u32 ip)
{
  if( con >= OSC_SERVER_NUM_CONNECTIONS )
    return -1; // invalid connection

  osc_remote_ip[con] = ip;
#if 0
  return OSC_SERVER_Init(0);
#else
  return 0; // OSC_SERVER_Init(0) has to be called after all settings have been done
#endif
}

u32 OSC_SERVER_RemoteIP_Get(u8 con)
{
  return osc_remote_ip[con];
}


s32 OSC_SERVER_RemotePortSet(u8 con, u16 port)
{
  if( con >= OSC_SERVER_NUM_CONNECTIONS )
    return -1; // invalid connection

  osc_remote_port[con] = port;
#if 0
  return OSC_SERVER_Init(0);
#else
  return 0; // OSC_SERVER_Init(0) has to be called after all settings have been done
#endif
}

u16 OSC_SERVER_RemotePortGet(u8 con)
{
  return osc_remote_port[con];
}

s32 OSC_SERVER_LocalPortSet(u8 con, u16 port)
{
  if( con >= OSC_SERVER_NUM_CONNECTIONS )
    return -1; // invalid connection

  osc_local_port[con] = port;
#if 0
  return OSC_SERVER_Init(0);
#else
  return 0; // OSC_SERVER_Init(0) has to be called after all settings have been done
#endif
}

u16 OSC_SERVER_LocalPortGet(u8 con)
{
  return osc_local_port[con];
}


/////////////////////////////////////////////////////////////////////////////
// Returns >= 0 if given port is a remote port of any connection
// Returns -1 if no connection asigned to the port
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_IsRemotePort(u16 port)
{
  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con)
    if( osc_remote_port[con] == port )
      return con;

  return -1; // port not assigned
}

/////////////////////////////////////////////////////////////////////////////
// Returns >= 0 if given port is a local port of any connection
// Returns -1 if no connection asigned to the port
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_IsLocalPort(u16 port)
{
  int con;
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con)
    if( osc_local_port[con] == port )
      return con;

  return -1; // port not assigned
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
#if DEBUG_VERBOSE_LEVEL >= 3
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
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] invalid OSC packet, status %d\n", status);
#endif
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Called by OSC client to send an UDP datagram
// To ensure proper mutex handling, functions inside the server have to
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_SendPacket(u8 con, u8 *packet, u32 len)
{
  // exit immediately if length == 0
  if( len == 0 )
    return 0;

  // exit immediately if connection not ready
  if( osc_conn[con] == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] dropped SendPacket due to invalid connection #%d\n", con);
#endif
    return -1; // connection not established
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  MIOS32_MIDI_SendDebugMessage("[OSC_SERVER] Send Datagram to %d.%d.%d.%d:%d (%d bytes) via #%d\n", 
			       (osc_conn[con]->ripaddr[0] >> 0) & 0xff,
			       (osc_conn[con]->ripaddr[0] >> 8) & 0xff,
			       (osc_conn[con]->ripaddr[1] >> 0) & 0xff,
			       (osc_conn[con]->ripaddr[1] >> 8) & 0xff,
			       HTONS(osc_conn[con]->rport),
			       len,
			       con);
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
  uip_udp_periodic_conn(osc_conn[con]);

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
#if DEBUG_VERBOSE_LEVEL >= 2
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

  // propagate to application
  // port is located in method argument
  MIOS32_MIDI_SendPackageToRxCallback(method_arg, p);
  APP_MIDI_NotifyPackage(method_arg, p);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////



static mios32_osc_search_tree_t parse_root[] = {
  //{ "midi", NULL, &OSC_SERVER_Method_SendMIDI, 0x00000000 },
  { "midi",  NULL, &OSC_SERVER_Method_SendMIDI, OSC0 },
  { "midi1", NULL, &OSC_SERVER_Method_SendMIDI, OSC0 },
  { "midi2", NULL, &OSC_SERVER_Method_SendMIDI, OSC1 },
  { "midi3", NULL, &OSC_SERVER_Method_SendMIDI, OSC2 },
  { "midi4", NULL, &OSC_SERVER_Method_SendMIDI, OSC3 },

  { NULL, NULL, NULL, 0 } // terminator
};
