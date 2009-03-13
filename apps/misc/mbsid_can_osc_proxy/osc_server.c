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

#include "mbnet_task.h"


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


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// return value of a function is negative on invalid arguments
// since some parameters are biased (+/- <range>), we define an error value outside this range
#define INVALID_ARGUMENT -1000000


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_Init(u32 mode)
{
  // disable send packet
  osc_send_packet = NULL;

  // remove open connection
  if(osc_conn != NULL) {
    uip_udp_remove(osc_conn);
  }

  // create new connection
  uip_ipaddr_t ripaddr;
  uip_ipaddr(ripaddr,
	     ((OSC_REMOTE_IP)>>24) & 0xff,
	     ((OSC_REMOTE_IP)>>16) & 0xff,
	     ((OSC_REMOTE_IP)>> 8) & 0xff,
	     ((OSC_REMOTE_IP)>> 0) & 0xff);

  if( (osc_conn=uip_udp_new(&ripaddr, HTONS(OSC_SERVER_PORT))) != NULL ) {
    uip_udp_bind(osc_conn, HTONS(OSC_SERVER_PORT));
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
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by UIP when a new UDP datagram has been received
// (UIP_UDP_APPCALL has been configured accordingly in uip-conf.h)
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_AppCall(void)
{
  if( uip_udp_conn->rport == HTONS(OSC_SERVER_PORT) ) {
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
}


/////////////////////////////////////////////////////////////////////////////
// Called by OSC client to send an UDP datagram
// To ensure proper mutex handling, functions inside the server have to
// use OSC_SERVER_SendPacket_Internal instead to avoid a hang-up while
// waiting for a mutex which the server already got
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_SendPacket_Internal(u8 *packet, u32 len)
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

  return 0; // no error
}

s32 OSC_SERVER_SendPacket(u8 *packet, u32 len)
{
  // take over exclusive access to UIP functions
  MUTEX_UIP_TAKE;

  // send packet
  s32 status = OSC_SERVER_SendPacket_Internal(packet, len);

  // release exclusive access to UIP functions
  MUTEX_UIP_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// OSC Debug Output
// Called by all methods if verbose level >= 1
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_DebugMessage(mios32_osc_args_t *osc_args, u32 method_arg)
{
  {
    // for debugging: merge path parts to complete path
    char path[128]; // should be enough?
    int i;
    char *path_ptr = path;
    for(i=0; i<osc_args->num_path_parts; ++i) {
      path_ptr = stpcpy(path_ptr, "/");
      path_ptr = stpcpy(path_ptr, osc_args->path_part[i]);
    }

    MIOS32_MIDI_SendDebugMessage("[%s] timetag %d.%d (%d args), Method Arg: 0x%08x\n", 
				 path,
				 osc_args->timetag.seconds,
				 osc_args->timetag.fraction,
				 osc_args->num_args,
				 method_arg);

    for(i=0; i < osc_args->num_args; ++i) {
      switch( osc_args->arg_type[i] ) {
        case 'i': // int32
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: %d (int32)\n", path, i, MIOS32_OSC_GetInt(osc_args->arg_ptr[i]));
	  break;

        case 'f': { // float32
	  float value = MIOS32_OSC_GetFloat(osc_args->arg_ptr[i]);
	  // note: the simple printf function doesn't support %f
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: %d.%03d (float32)\n", path, i, 
				       (int)value, (int)((value*1000))%1000);
	} break;

        case 's': // string
        case 'S': // alternate string
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: '%s'\n", path, i, MIOS32_OSC_GetString(osc_args->arg_ptr[i]));
	  break;

        case 'b': // blob
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: blob with length %u\n", path, i, MIOS32_OSC_GetWord(osc_args->arg_ptr[i]));
	  break;

        case 'h': { // int64
	  long long value = MIOS32_OSC_GetLongLong(osc_args->arg_ptr[i]);
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: 0x%08x%08x (int64)\n", path, i, 
				       (u32)(value >> 32), (u32)value);
	} break;

        case 't': { // timetag
	  mios32_osc_timetag_t timetag = MIOS32_OSC_GetTimetag(osc_args->arg_ptr[i]);
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: seconds %u fraction %u\n", path, i, 
				       timetag.seconds, timetag.fraction);
	} break;

        case 'd': { // float64 (double)
	  double value = MIOS32_OSC_GetDouble(osc_args->arg_ptr[i]);
	  // note: the simple printf function doesn't support %f
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: %d.%03d (float64)\n", path, i, 
				       (int)value, (int)((value*1000))%1000);
	} break;

        case 'c': // ASCII character
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: char %c\n", path, i, MIOS32_OSC_GetChar(osc_args->arg_ptr[i]));
	  break;

        case 'r': // 32 bit RGBA color
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: %08X (RGBA color)\n", path, i, MIOS32_OSC_GetWord(osc_args->arg_ptr[i]));
	  break;

        case 'm': // MIDI message
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: %08X (MIDI)\n", path, i, MIOS32_OSC_GetMIDI(osc_args->arg_ptr[i]).ALL);
	  break;

        case 'T': // TRUE
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: TRUE\n", path, i);
	  break;

        case 'F': // FALSE
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: FALSE\n", path, i);
	  break;

        case 'N': // NIL
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: NIL\n", path, i);
	  break;

        case 'I': // Infinitum
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: Infinitum\n", path, i);
	  break;

        case '[': // beginning of array
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: [ (beginning of array)\n", path, i);
	  break;

        case ']': // end of array
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: [ (end of array)\n", path, i);
	  break;

        default:
	  MIOS32_MIDI_SendDebugMessage("[%s] %d: unknown arg type '%c'\n", path, i, osc_args->arg_type[i]);
	  break;
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function which returns a positive float
// returns INVALID_ARGUMENT if argument isn't a float
/////////////////////////////////////////////////////////////////////////////
static float OSC_SERVER_GetFloat(u8 arg_type, u8 *arg_ptr)
{
  float value;

  switch( arg_type ) {
    case 'f': // float32
      value = MIOS32_OSC_GetFloat(arg_ptr);
      break;
    default:
      return INVALID_ARGUMENT;
  }

  return value;
}

/////////////////////////////////////////////////////////////////////////////
// Help function which converts a positive float or integer argument into integer
// returns INVALID_ARGUMENT if argument neither float nor integer
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_GetIntOrFloat(u8 arg_type, u8 *arg_ptr)
{
  s32 value;

  switch( arg_type ) {
    case 'i': // int32
      value = MIOS32_OSC_GetInt(arg_ptr);
      break;
    case 'f': // float32
      value = (s32)MIOS32_OSC_GetFloat(arg_ptr);
      break;
    default:
      return INVALID_ARGUMENT;
  }

  return value;
}

/////////////////////////////////////////////////////////////////////////////
// Help function which converts a positive float, integer, TRUE or FALSE
// into a binary value (0 or 1)
// returns INVALID_ARGUMENT if type doesn't match
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_GetBinary(u8 arg_type, u8 *arg_ptr)
{
  s32 value;

  switch( arg_type ) {
    case 'i': // int32
      value = MIOS32_OSC_GetInt(arg_ptr) ? 1 : 0;
      break;
    case 'f': // float32
      value = (MIOS32_OSC_GetFloat(arg_ptr) >= 1.0) ? 1 : 0;
      break;
    case 'T': // TRUE
      value = 1;
      break;
    case 'F': // FALSE
      value = 0;
      break;
    default:
      return INVALID_ARGUMENT;
  }

  return value;
}


/////////////////////////////////////////////////////////////////////////////
// Method to set a signed 7bit value in OSC range
// Path: /sid*/*/*/osc/<parameter> <value>
// <value> has to be a float32 or int32 in the range -64..+63
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_OscPM7D(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  s32 value;
  if( (value=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // shift to range 0..127 and saturate
  value += 64;
  if( value < 0 )
    value = 0;
  else if( value > 127 )
    value = 127;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 osc = ((method_arg >> 28) & 0x03) + 3*((method_arg >> 31) & 1);
  u16 addr = 0x060 + 0x10*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite8(sid, addr, value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to set a signed 8bit value in OSC range
// Path: /sid*/*/*/osc/<parameter> <value>
// <value> has to be a float32 in the range -1..1
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_OscPM8(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // shift to range 0..256 and saturate
  s32 int_value = (s32)((value * 128.0) + 128.0);
  if( int_value < 0 )
    int_value = 0;
  else if( int_value > 255 )
    int_value = 255;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 osc = ((method_arg >> 28) & 0x03) + 3*((method_arg >> 31) & 1);
  u16 addr = 0x060 + 0x10*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite8(sid, addr, int_value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to set a 12bit value in OSC range
// Path: /sid*/*/filter/<parameter> <value>
// <value> has to be a float32 in the range 0..+1
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_Osc12(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // stretch to range 0..4095 and saturate
  s32 int_value = (s32)(value * 4095.0);
  if( int_value < 0 )
    int_value = 0;
  else if( int_value > 4095 )
    int_value = 4095;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 osc = ((method_arg >> 28) & 0x03) + 3*((method_arg >> 31) & 1);
  u16 addr = 0x060 + 0x10*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite16(sid, addr, int_value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to set a 12bit value in Filter range
// Path: /sid*/*/filter/<parameter> <value>
// <value> has to be a float32 in the range 0..+1
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_Fil12(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // stretch to range 0..4095 and saturate
  s32 int_value = (s32)(value * 4095.0);
  if( int_value < 0 )
    int_value = 0;
  else if( int_value > 4095 )
    int_value = 4095;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 fil = (method_arg >> 31) & 1;
  u16 addr = 0x054 + 0x06*fil + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite16(sid, addr, int_value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to set a 8bit value in Filter range
// Path: /sid*/*/*/filter/<parameter> <value>
// <value> has to be a float32 in the range 0..+1
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_Fil8(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // stretch to range 0..255 and saturate
  s32 int_value = (s32)(value * 255.0);
  if( int_value < 0 )
    int_value = 0;
  else if( int_value > 255 )
    int_value = 255;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 fil = (method_arg >> 31) & 1;
  u16 addr = 0x054 + 0x06*fil + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite8(sid, addr, int_value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Method to access the SID frequency register directly
// Path: /sid*/*/*/direct/frq <value>
// <value> has to be a float32 or int32 in the range 0..3905 (Hz)
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_DirectFrq(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  s32 value;
  if( (value=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // ensure that frequency is positive
  if( value < 0 )
    value = 0;

  // determine SID frequency register value
  s32 frq_value = (s32)((float)value * 16.777216);
  if( frq_value > 65535 )
    frq_value = 65535;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 lr = (method_arg >> 31) & 1;
  u8 osc = ((method_arg >> 28) & 0x03);
  u16 addr = 0xfe00 + lr*0x20 + 7*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite16(sid, addr, (u16)frq_value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to access a 16bit SID register directly
// Path: /sid*/*/*/direct/* <value>
// <value> has to be a float32 or int32 in the range 0..65535
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_Direct16(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  s32 value;
  if( (value=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // ensure that value is in allowed range
  if( value < 0 )
    value = 0;
  else if( value > 65535 )
    value = 65535;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 lr = (method_arg >> 31) & 1;
  u8 osc = ((method_arg >> 28) & 0x03);
  u16 addr = 0xfe00 + lr*0x20 + 7*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite16(sid, addr, (u16)value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to access a 8bit SID register directly
// Path: /sid*/*/*/direct/* <value>
// <value> has to be a float32 or int32 in the range 0..255
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_Direct8(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get value
  s32 value;
  if( (value=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  // ensure that value is in allowed range
  if( value < 0 )
    value = 0;
  else if( value > 255 )
    value = 255;

  // forward to MBNet
  u8 sid = (method_arg >> 24) & 0xf;
  u8 lr = (method_arg >> 31) & 1;
  u8 osc = ((method_arg >> 28) & 0x03);
  u16 addr = 0xfe00 + lr*0x20 + 7*osc + (method_arg & 0xfff);
  return MBNET_TASK_PatchWrite8(sid, addr, (u8)value);
}


/////////////////////////////////////////////////////////////////////////////
// Method to set the gates of all SIDs
// Path: /gates <gates1> <gates2> <gates3> <gates4>
// <gates*> has to be a float32 or int32
// bit [2:0] (left channel) and [6:4] (right channel) are used
// Each bit controls the gate of an oscillator
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_DirectGates(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 4 arguments
  if( osc_args->num_args < 4 )
    return -1; // wrong number of arguments

  // get gate values
  s32 g1;
  if( (g1=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) <= INVALID_ARGUMENT )
    return -2; // wrong argument type for first parameter

  s32 g2;
  if( (g2=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[1], osc_args->arg_ptr[1])) <= INVALID_ARGUMENT )
    return -3; // wrong argument type for second parameter

  s32 g3;
  if( (g3=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[2], osc_args->arg_ptr[2])) <= INVALID_ARGUMENT )
    return -4; // wrong argument type for third parameter

  s32 g4;
  if( (g4=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[3], osc_args->arg_ptr[3])) <= INVALID_ARGUMENT )
    return -5; // wrong argument type for fourth parameter

  // forward to MBNet
  u16 addr = 0xfe80;
  s32 status = 0;
  status |= MBNET_TASK_PatchWrite8(0x00, addr, (u8)g1);
  status |= MBNET_TASK_PatchWrite8(0x01, addr, (u8)g2);
  status |= MBNET_TASK_PatchWrite8(0x02, addr, (u8)g3);
  status |= MBNET_TASK_PatchWrite8(0x03, addr, (u8)g4);
  return status;
}



/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////


static const char str_sid1[] = "sid1";
static const char str_sid2[] = "sid2";
static const char str_sid3[] = "sid3";
static const char str_sid4[] = "sid4";

static const char str_l[] = "l";
static const char str_r[] = "r";

static const char str_1[] = "1";
static const char str_2[] = "2";
static const char str_3[] = "3";

static const char str_osc[] = "osc";
static const char str_filter[] = "filter";
static const char str_direct[] = "direct";

static const char str_transpose[] = "transpose";
static const char str_finetune[] = "finetune";
static const char str_pulsewidth[] = "pulsewidth";

static const char str_cutoff[] = "cutoff";
static const char str_resonance[] = "resonance";

static const char str_frq[] = "frq";
static const char str_ctrl[] = "ctrl";
static const char str_adsr[] = "adsr";

static const char str_gates[] = "gates";


static mios32_osc_search_tree_t parse_sid_osc[] = {
  { str_transpose,  NULL, &OSC_SERVER_Method_OscPM7D, 0x00000008 }, // bit [11:0] selects offset in OSC address range
  { str_finetune,   NULL, &OSC_SERVER_Method_OscPM8, 0x00000009 }, // bit [11:0] selects offset in OSC address range
  { str_pulsewidth, NULL, &OSC_SERVER_Method_Osc12,  0x00000004 }, // bit [11:0] selects offset in OSC address range
  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_sid_filter[] = {
  { str_cutoff,    NULL, &OSC_SERVER_Method_Fil12, 0x00000001 }, // bit [11:0] selects offset in Filter address range
  { str_resonance, NULL, &OSC_SERVER_Method_Fil8,  0x00000003 }, // bit [11:0] selects offset in Filter address range
  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_sid_direct[] = {
  { str_frq,        NULL, &OSC_SERVER_Method_DirectFrq,   0x00000000 }, // bit [11:0] selects offset in SID address range
  { str_pulsewidth, NULL, &OSC_SERVER_Method_Direct16,    0x00000002 }, // bit [11:0] selects offset in SID address range
  { str_ctrl,       NULL, &OSC_SERVER_Method_Direct8,     0x00000004 }, // bit [11:0] selects offset in SID address range
  { str_adsr,       NULL, &OSC_SERVER_Method_Direct16,    0x00000005 }, // bit [11:0] selects offset in SID address range
  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_sid_123[] = {
  { str_osc,    parse_sid_osc,    NULL, 0x00000000 },
  { str_direct, parse_sid_direct, NULL, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};


static mios32_osc_search_tree_t parse_sid_lr[] = {
  { str_1, parse_sid_123, NULL, 0x00000000 }, // bit [30:28] selects Voice1
  { str_2, parse_sid_123, NULL, 0x10000000 }, // bit [30:28] selects Voice2
  { str_3, parse_sid_123, NULL, 0x20000000 }, // bit [30:28] selects Voice3
  { str_filter, parse_sid_filter, NULL, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};

static mios32_osc_search_tree_t parse_sid[] = {
  { str_l, parse_sid_lr, NULL, 0x00000000 }, // bit [31] selects L
  { str_r, parse_sid_lr, NULL, 0x80000000 }, // bit [31] selects R
  { NULL, NULL, NULL, 0 } // terminator
};



static mios32_osc_search_tree_t parse_root[] = {
  { str_sid1,  parse_sid, NULL, 0x00000000 }, // bit [27:24] selects SID1
  { str_sid2,  parse_sid, NULL, 0x01000000 }, // bit [27:24] selects SID2
  { str_sid3,  parse_sid, NULL, 0x02000000 }, // bit [27:24] selects SID3
  { str_sid4,  parse_sid, NULL, 0x03000000 }, // bit [27:24] selects SID4
  { str_gates, NULL,      &OSC_SERVER_Method_DirectGates, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};
