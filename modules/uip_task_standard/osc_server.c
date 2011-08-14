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
#include "uip_arp.h"
#include "network-device.h"
#include "uip_task.h"

#include "osc_server.h"
#include "osc_client.h"
#include <app.h>

/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


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


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_Init(u32 mode)
{
  int con;

  // disable send packet
  osc_send_packet = NULL;

  // remove open connections
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con)
    if( osc_conn[con] != NULL )
      uip_udp_remove(osc_conn[con]);

  // create new connections
  // note: for faster execution we accept all UDP packets from the given IP which are sent to the given local port!
  // ports are checked inside OSC_SERVER_AppCall!
  for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con) {
    uip_ipaddr_t ripaddr;
    uip_ipaddr(ripaddr,
               ((osc_remote_ip[con])>>24) & 0xff,
               ((osc_remote_ip[con])>>16) & 0xff,
               ((osc_remote_ip[con])>> 8) & 0xff,
               ((osc_remote_ip[con])>> 0) & 0xff);

#if 0
    u16 remote_port = HTONS(osc_remote_port[con]);
#else
    u16 remote_port = 0;
#endif
    if( (osc_conn[con]=uip_udp_new(&ripaddr, remote_port)) != NULL ) {
      uip_udp_bind(osc_conn[con], HTONS(osc_local_port[con]));
#if DEBUG_VERBOSE_LEVEL >= 2
      UIP_TASK_MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("[OSC_SERVER] #%d sends to %d.%d.%d.%d:%d\n", 
		con,
		(osc_conn[con]->ripaddr[0] >> 0) & 0xff,
		(osc_conn[con]->ripaddr[0] >> 8) & 0xff,
		(osc_conn[con]->ripaddr[1] >> 0) & 0xff,
		(osc_conn[con]->ripaddr[1] >> 8) & 0xff,
		HTONS(osc_conn[con]->lport));
      UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif
    } else {
#if DEBUG_VERBOSE_LEVEL >= 1
      UIP_TASK_MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("[OSC_SERVER] FAILED to create connection #%d (no free ports)\n", con);
      UIP_TASK_MUTEX_MIDIOUT_GIVE;
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
// Called by UIP when a new UDP datagram has been received
// (UIP_UDP_APPCALL has been configured accordingly in uip-conf.h)
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_AppCall(void)
{
  u8 udp_monitor_level = UIP_TASK_UDP_MonitorLevelGet();

  if( uip_poll() ) {
    // called each second or on request (-> uip_udp_periodic_conn)

    if( osc_send_packet != NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      UIP_TASK_MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("[OSC_SERVER] Sending Datagram to %d.%d.%d.%d:%d (%d bytes)\n", 
		(uip_udp_conn->ripaddr[0] >> 0) & 0xff,
		(uip_udp_conn->ripaddr[0] >> 8) & 0xff,
		(uip_udp_conn->ripaddr[1] >> 0) & 0xff,
		(uip_udp_conn->ripaddr[1] >> 8) & 0xff,
		HTONS(uip_udp_conn->rport),
		osc_send_len);
      MIOS32_MIDI_SendDebugHexDump((u8 *)osc_send_packet, osc_send_len);
      UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

      // send datagram
      uip_send(osc_send_packet, osc_send_len);
      osc_send_packet = NULL;

      if( udp_monitor_level >= UDP_MONITOR_LEVEL_2_OSC_REC_AND_SEND )
	UIP_TASK_UDP_MonitorPacket(UDP_MONITOR_SEND, "OSC_SEND");
    }
  }

  if( uip_newdata() ) {    
    // check for matching port
    int con;
    u16 search_port = HTONS(uip_udp_conn->lport); // (no error: use lport instead of rport, since UIP inserts it there)

    u8 port_ok = 0;
    for(con=0; con<OSC_SERVER_NUM_CONNECTIONS; ++con)
      if( osc_local_port[con] == search_port ) {
	port_ok = 1;
	break;
      }

    if( !port_ok ) {
      // forward to monitor
      if( udp_monitor_level >= UDP_MONITOR_LEVEL_4_ALL ||
	  (udp_monitor_level >= UDP_MONITOR_LEVEL_3_ALL_GEQ_1024 && search_port >= 1024) )
	UIP_TASK_UDP_MonitorPacket(UDP_MONITOR_RECEIVED, "UNMATCHED_PORT");
    } else {
      // forward to monitor
      if( udp_monitor_level >= UDP_MONITOR_LEVEL_1_OSC_REC )
	UIP_TASK_UDP_MonitorPacket(UDP_MONITOR_RECEIVED, "OSC_RECEIVED");

      // new UDP package has been received
#if DEBUG_VERBOSE_LEVEL >= 3
      UIP_TASK_MUTEX_MIDIOUT_TAKE;
      DEBUG_MSG("[OSC_SERVER] Received Datagram from %d.%d.%d.%d:%d (%d bytes)\n", 
		(uip_udp_conn->ripaddr[0] >> 0) & 0xff,
		(uip_udp_conn->ripaddr[0] >> 8) & 0xff,
		(uip_udp_conn->ripaddr[1] >> 0) & 0xff,
		(uip_udp_conn->ripaddr[1] >> 8) & 0xff,
		search_port,
		uip_len);
      MIOS32_MIDI_SendDebugHexDump((u8 *)uip_appdata, uip_len);
      UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif
      s32 status = MIOS32_OSC_ParsePacket((u8 *)uip_appdata, uip_len, parse_root);
      if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	UIP_TASK_MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("[OSC_SERVER] invalid OSC packet, status %d\n", status);
	UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif
      }
    }
  }

  return 0; // no error
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
    UIP_TASK_MUTEX_MIDIOUT_TAKE;
    DEBUG_MSG("[OSC_SERVER] dropped SendPacket due to invalid connection #%d\n", con);
    UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif
    return -1; // connection not established
  }

  // take over exclusive access to UIP functions
  MUTEX_UIP_TAKE;

  // set remote port now!
  osc_conn[con]->rport = HTONS(osc_remote_port[con]);

#if DEBUG_VERBOSE_LEVEL >= 2
  UIP_TASK_MUTEX_MIDIOUT_TAKE;
  DEBUG_MSG("[OSC_SERVER] Send Datagram to %d.%d.%d.%d:%d (%d bytes) via #%d\n", 
	    (osc_conn[con]->ripaddr[0] >> 0) & 0xff,
	    (osc_conn[con]->ripaddr[0] >> 8) & 0xff,
	    (osc_conn[con]->ripaddr[1] >> 0) & 0xff,
	    (osc_conn[con]->ripaddr[1] >> 8) & 0xff,
	    HTONS(osc_conn[con]->rport),
	    len,
	    con);
  MIOS32_MIDI_SendDebugHexDump(packet, len);
  UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

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

  // clear remote port again so that we accept packets sent from any port
  osc_conn[con]->rport = 0;

  // release exclusive access to UIP functions
  MUTEX_UIP_GIVE;

  return 0; // no error
}




/////////////////////////////////////////////////////////////////////////////
// Method to send a MIDI message
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_MIDI(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  UIP_TASK_MUTEX_MIDIOUT_TAKE;
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
  UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

  // check osc port
  u8 con = method_arg & 0xf;
  if( con > OSC_SERVER_NUM_CONNECTIONS )
    return -1; // wrong port

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -2; // wrong number of arguments

  // check for MIDI event
  if( osc_args->arg_type[0] == 'm' ) {
    mios32_midi_package_t p = MIOS32_OSC_GetMIDI(osc_args->arg_ptr[0]);

    // extra treatment for SysEx messages
    // Note: SysEx streams will be sent as blobs and therefore don't need to be considered here
    if( p.evnt0 >= 0xf0 || p.evnt0 < 0x80 ) {
      if( p.evnt0 >= 0xf8 )
	p.cin = 0xf; // realtime
      if( p.evnt0 == 0xf1 || p.evnt0 == 0xf3 )
	p.cin = 2; // two byte system common message
      else
	p.cin = 3; // three byte system common message
    } else {
      p.cin = p.evnt0 >> 4;
    }

    // propagate to application
    // port is located in method argument
    UIP_TASK_MUTEX_MIDIIN_TAKE;
    MIOS32_MIDI_SendPackageToRxCallback(method_arg, p);
    APP_MIDI_NotifyPackage(method_arg, p);
    UIP_TASK_MUTEX_MIDIIN_GIVE;

  } else  if( osc_args->arg_type[0] == 'b' ) {
    // SysEx stream is embedded into blob
    u32 len = MIOS32_OSC_GetBlobLength(osc_args->arg_ptr[0]);
    u8 *blob = MIOS32_OSC_GetBlobData(osc_args->arg_ptr[0]);

    // propagate to application
    // port is located in method argument
    int i;
    for(i=0; i<len; ++i, blob++) {
      APP_SYSEX_Parser(method_arg, *blob);
      if( *blob == 0xf7 )
	break;
    }
  } else
    return -2; // wrong argument type for first parameter


  return 0; // no error
}


static s32 OSC_SERVER_Method_MCMPP(mios32_osc_args_t *osc_args, u32 method_arg)
{
  int i;

#if DEBUG_VERBOSE_LEVEL >= 2
  UIP_TASK_MUTEX_MIDIOUT_TAKE;
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
  UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // extract value and channel from original path
  // format: /mcmpp/name/<value>/<channel>
  // /mcmpp and /name have already been parsed, we only need the values
  char *path_values = (char *)osc_args->original_path;
  for(i=0; i<2; ++i)
    if( (path_values = strchr(path_values+1, '/')) == NULL )
      return -2; // invalid format
  ++path_values;

  int note = 0;
  if( method_arg < 0xc0 ) {
      // (key) value not transmitted for 0xc0 (program change), 0xd0 (aftertouch), 0xe0 (pitch)

    // get value
    note = atoi(path_values);

    // next slash
    if( (path_values = strchr(path_values+1, '/')) == NULL )
      return -2; // invalid format
    ++path_values;
  }

  // get channel
  int chn = atoi(path_values) - 1;
  if( chn < 0 || chn >= 15 )
    return -3; // invalid channel

  // get status nibble and merge with channel
  int evnt0 = (method_arg & 0xf0) | chn;

  // build MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = evnt0 >> 4;
  p.evnt0 = evnt0;

  // pitch bender?
  if( (evnt0 & 0xf0) == 0xe0 ) {
    // get velocity resp. CC value
    int pitch = 8192;
    if( osc_args->arg_type[0] == 'i' )
      pitch = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
    else if( osc_args->arg_type[0] == 'f' )
      pitch = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 8191.0);
    pitch += 8192;
    if( pitch < 0 ) pitch = 0; else if( pitch > 16383 ) pitch = 16383;
    p.evnt1 = pitch & 0x7f;
    p.evnt2 = (pitch >> 7) & 0x7f;
  } else {
    // get velocity resp. CC value
    int velocity = 127;
    if( osc_args->arg_type[0] == 'i' )
      velocity = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
    else if( osc_args->arg_type[0] == 'f' )
      velocity = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 127.0);
    if( velocity < 0 ) velocity = 0; else if( velocity > 127 ) velocity = 127;

    p.evnt1 = note;
    p.evnt2 = velocity;
  }

  // propagate to application
  // port is located in method argument

  // search for ports which are assigned to the MCMPP protocol
  int osc_port;
  for(osc_port=0; osc_port<OSC_CLIENT_NUM_PORTS; ++osc_port) {
    u8 transfer_mode = OSC_CLIENT_TransferModeGet(osc_port);
    if( OSC_IGNORE_TRANSFER_MODE || transfer_mode == OSC_CLIENT_TRANSFER_MODE_MCMPP ) {
      UIP_TASK_MUTEX_MIDIIN_TAKE;
      APP_MIDI_NotifyPackage(OSC0 + osc_port, p);
      UIP_TASK_MUTEX_MIDIIN_GIVE;
    }
  }

  return 0; // no error
}


static s32 OSC_SERVER_Method_Event(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  UIP_TASK_MUTEX_MIDIOUT_TAKE;
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
  UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get channel and status nibble
  int evnt0 = method_arg & 0xff;

  // check for TouchOSC format (note/CC number coded in path)
  // extract value and channel from original path
  // format: /<chn>/name/<value>
  // /<chn> and /name have already been parsed, we only need the last value
  char *path_values = (char *)osc_args->original_path;
  int i;
  for(i=0; i<2; ++i) {
    if( (path_values = strchr(path_values+1, '/')) == NULL )
      break;
    ++path_values;
  }

  int note = 60;
  int velocity = 127;
  if( i == 2 ) { // we expect touch osc format
    if( method_arg < 0xc0 ) {
      // (key) value not transmitted for 0xc0 (program change), 0xd0 (aftertouch), 0xe0 (pitch)

      // get value
      note = atoi(path_values);
    }

    // get velocity
    if( osc_args->num_args >= 1 ) {
      if( osc_args->arg_type[0] == 'i' )
	velocity = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
      else if( osc_args->arg_type[0] == 'f' )
	velocity = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 127.0);
    }
  } else {
    // get note
    int note = 60;
    if( osc_args->arg_type[0] == 'i' )
      note = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
    else if( osc_args->arg_type[0] == 'f' )
      note = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 127.0);

    // get velocity
    if( osc_args->num_args >= 2 ) {
      if( osc_args->arg_type[1] == 'i' )
	velocity = MIOS32_OSC_GetInt(osc_args->arg_ptr[1]);
      else if( osc_args->arg_type[1] == 'f' )
	velocity = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[1]) * 127.0);
    }
  }

  if( note < 0 ) note = 0; else if( note > 127 ) note = 127;
  if( velocity < 0 ) velocity = 0; else if( velocity > 127 ) velocity = 127;

  // build MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = evnt0 >> 4;
  p.evnt0 = evnt0;
  p.evnt1 = note;
  p.evnt2 = velocity;

  // propagate to application
  // search for ports which are assigned to the MIDI value protocol
  int osc_port;
  for(osc_port=0; osc_port<OSC_CLIENT_NUM_PORTS; ++osc_port) {
    u8 transfer_mode = OSC_CLIENT_TransferModeGet(osc_port);
    if( OSC_IGNORE_TRANSFER_MODE ||
	transfer_mode == OSC_CLIENT_TRANSFER_MODE_INT ||
	transfer_mode == OSC_CLIENT_TRANSFER_MODE_FLOAT ||
	transfer_mode == OSC_CLIENT_TRANSFER_MODE_TOSC ) {
      UIP_TASK_MUTEX_MIDIIN_TAKE;
      APP_MIDI_NotifyPackage(OSC0 + osc_port, p);
      UIP_TASK_MUTEX_MIDIIN_GIVE;
    }
  }

  return 0; // no error
}

static s32 OSC_SERVER_Method_EventPB(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  UIP_TASK_MUTEX_MIDIOUT_TAKE;
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
  UIP_TASK_MUTEX_MIDIOUT_GIVE;
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get channel and status nibble
  int evnt0 = method_arg & 0xff;

  // get pitchbender value
  int value = 8192;
  if( osc_args->arg_type[0] == 'i' )
    value = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
  else if( osc_args->arg_type[0] == 'f' )
    value = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 8191.0);
  value += 8192;
  if( value < 0 ) value = 0; else if( value > 16383 ) value = 16383;

  // build MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = evnt0 >> 4;
  p.evnt0 = evnt0;
  p.evnt1 = value & 0x7f;
  p.evnt2 = (value >> 7) & 0x7f;

  // propagate to application
  // search for ports which are assigned to the MIDI value protocol
  int osc_port;
  for(osc_port=0; osc_port<OSC_CLIENT_NUM_PORTS; ++osc_port) {
    u8 transfer_mode = OSC_CLIENT_TransferModeGet(osc_port);
    if( OSC_IGNORE_TRANSFER_MODE ||
	transfer_mode == OSC_CLIENT_TRANSFER_MODE_INT ||
	transfer_mode == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
      UIP_TASK_MUTEX_MIDIIN_TAKE;
      APP_MIDI_NotifyPackage(OSC0 + osc_port, p);
      UIP_TASK_MUTEX_MIDIIN_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////


static mios32_osc_search_tree_t parse_mcmpp_value[] = {
  { "*", NULL, &OSC_SERVER_Method_MCMPP, 0x00000000 },

  { NULL, NULL, NULL, 0 } // terminator
};

static mios32_osc_search_tree_t parse_mcmpp[] = {
  { "key",           parse_mcmpp_value, NULL, 0x00000090 }, // bit [7:4] contains status byte
  { "polypressure",  parse_mcmpp_value, NULL, 0x000000a0 }, // bit [7:4] contains status byte
  { "cc",            parse_mcmpp_value, NULL, 0x000000b0 }, // bit [7:4] contains status byte
  { "programchange", parse_mcmpp_value, NULL, 0x000000c0 }, // bit [7:4] contains status byte
  { "aftertouch",    parse_mcmpp_value, NULL, 0x000000d0 }, // bit [7:4] contains status byte
  { "pitch",         parse_mcmpp_value, NULL, 0x000000e0 }, // bit [7:4] contains status byte

  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_event[] = {
  { "note",          NULL, &OSC_SERVER_Method_Event,   0x00000090 }, // bit [7:4] contains status byte
  { "polypressure",  NULL, &OSC_SERVER_Method_Event,   0x000000a0 }, // bit [7:4] contains status byte
  { "cc",            NULL, &OSC_SERVER_Method_Event,   0x000000b0 }, // bit [7:4] contains status byte
  { "programchange", NULL, &OSC_SERVER_Method_Event,   0x000000c0 }, // bit [7:4] contains status byte
  { "aftertouch",    NULL, &OSC_SERVER_Method_Event,   0x000000b0 }, // bit [7:4] contains status byte
  { "pitchbend",     NULL, &OSC_SERVER_Method_EventPB, 0x000000e0 }, // bit [7:4] contains status byte

  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_root[] = {
  { "midi",  NULL, &OSC_SERVER_Method_MIDI, OSC0 },
  { "midi1", NULL, &OSC_SERVER_Method_MIDI, OSC0 },
  { "midi2", NULL, &OSC_SERVER_Method_MIDI, OSC1 },
  { "midi3", NULL, &OSC_SERVER_Method_MIDI, OSC2 },
  { "midi4", NULL, &OSC_SERVER_Method_MIDI, OSC3 },

  { "mcmpp", parse_mcmpp, NULL, 0x00000000}, // pianist pro format

  { "1",  parse_event, NULL, 0x00000000}, // bit [0:3] selects MIDI channel
  { "2",  parse_event, NULL, 0x00000001}, // bit [0:3] selects MIDI channel
  { "3",  parse_event, NULL, 0x00000002}, // bit [0:3] selects MIDI channel
  { "4",  parse_event, NULL, 0x00000003}, // bit [0:3] selects MIDI channel
  { "5",  parse_event, NULL, 0x00000004}, // bit [0:3] selects MIDI channel
  { "6",  parse_event, NULL, 0x00000005}, // bit [0:3] selects MIDI channel
  { "7",  parse_event, NULL, 0x00000006}, // bit [0:3] selects MIDI channel
  { "8",  parse_event, NULL, 0x00000007}, // bit [0:3] selects MIDI channel
  { "9",  parse_event, NULL, 0x00000008}, // bit [0:3] selects MIDI channel
  { "10", parse_event, NULL, 0x00000009}, // bit [0:3] selects MIDI channel
  { "11", parse_event, NULL, 0x0000000a}, // bit [0:3] selects MIDI channel
  { "12", parse_event, NULL, 0x0000000b}, // bit [0:3] selects MIDI channel
  { "13", parse_event, NULL, 0x0000000c}, // bit [0:3] selects MIDI channel
  { "14", parse_event, NULL, 0x0000000d}, // bit [0:3] selects MIDI channel
  { "15", parse_event, NULL, 0x0000000e}, // bit [0:3] selects MIDI channel
  { "16", parse_event, NULL, 0x0000000f}, // bit [0:3] selects MIDI channel

  { NULL, NULL, NULL, 0 } // terminator
};
