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
// returns -1 if argument isn't a float
/////////////////////////////////////////////////////////////////////////////
static float OSC_SERVER_GetFloat(u8 arg_type, u8 *arg_ptr)
{
  float value;

  switch( arg_type ) {
    case 'f': // float32
      value = MIOS32_OSC_GetFloat(arg_ptr);
      break;
    default:
      return -1.0; // wrong argument type
  }

  return value;
}

/////////////////////////////////////////////////////////////////////////////
// Help function which converts a positive float or integer argument into integer
// returns -1 if argument neither float nor integer
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
      return -1; // wrong argument type
  }

  return value;
}

/////////////////////////////////////////////////////////////////////////////
// Help function which converts a positive float, integer, TRUE or FALSE
// into a binary value (0 or 1)
// returns -1 if type doesn't match
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
      return -1; // wrong argument type
  }

  return value;
}



/////////////////////////////////////////////////////////////////////////////
// Method to control a single LED
// Path: /cs/led/set <led-number> <value>
// <led-number> can be float32 or int32
// <value> can be float32 (<1: clear, >=1: set), int32 (0 or != 0) or TRUE/FALSE
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_LED_Set(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 2 arguments
  if( osc_args->num_args < 2 )
    return -1; // wrong number of arguments

  // get LED number
  s32 pin;
  if( (pin=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) < 0 )
    return -2; // wrong argument type for first parameter

  // get LED value (binary: 0 or 1)
  s32 value;
  if( (value=OSC_SERVER_GetBinary(osc_args->arg_type[1], osc_args->arg_ptr[1])) < 0 )
    return -3; // wrong argument type for second parameter

  MIOS32_DOUT_PinSet(pin, value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Method to control all LEDs
// Path: /cs/led/set_all <value>
// <value> can be float32 (<1: clear, >=1: set), int32 (0 or != 0) or TRUE/FALSE
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_LED_SetAll(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // SR value can be float, integer, TRUE or FALSE
  // get SR value (binary: 0 or 1)
  s32 value;
  if( (value=OSC_SERVER_GetBinary(osc_args->arg_type[0], osc_args->arg_ptr[0])) < 0 )
    return -2; // wrong argument type for first parameter

  // convert to 0x00 or 0xff
  u8 sr_value = value ? 0xff : 0x00;

  // set all SRs
  int sr;
  for(sr=0; sr<MIOS32_SRIO_NUM_SR; ++sr)
    MIOS32_DOUT_SRSet(sr, sr_value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Method to control a single Motorfader
// Path: /cs/mf/set <motorfader-number> <value>
// <value> has to be a float32 (range 0.00000..1.00000)
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_MF_Set(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 2 arguments
  if( osc_args->num_args < 2 )
    return -1; // wrong number of arguments

  // get MF number
  s32 mf;
  if( (mf=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) < 0 )
    return -2; // wrong argument type for first parameter

  // get MF value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[1], osc_args->arg_ptr[1])) < 0 )
    return -3; // wrong argument type for second parameter

  // scale to AIN resolution
  u32 mf_pos = (u32)(value * 4095.0);
  if( mf_pos > 4095 )
    mf_pos = 4095;

  // move fader
  MIOS32_MF_TouchDetectionReset(mf); // force move regardless of touch detection state
  MIOS32_MF_FaderMove(mf, mf_pos);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Method to control all Motorfaders
// Path: /cs/mf/set_all <value>
// <value> has to be a float32 (range 0.00000..1.00000)
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_MF_SetAll(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get MF value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) < 0 )
    return -2; // wrong argument type for first parameter

  // scale to AIN resolution
  u32 mf_pos = (u32)(value * 4095.0);
  if( mf_pos > 4095 )
    mf_pos = 4095;

  // move all faders
  u8 mf;
  for(mf=0; mf<MIOS32_MF_NUM; ++mf) {
    MIOS32_MF_TouchDetectionReset(mf); // force move regardless of touch detection state
    MIOS32_MF_FaderMove(mf, mf_pos);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Method to configure the MF
// Pathes: /cfg/mf/deadband <motorfader-number> <value>              (method_arg == 0x00000000)
// Pathes: /cfg/mf/pwm_period <motorfader-number> <value>            (method_arg == 0x00000001)
// Pathes: /cfg/mf/pwm_duty_cycle_down <motorfader-number> <value>   (method_arg == 0x00000002)
// Pathes: /cfg/mf/pwm_duty_cycle_up <motorfader-number> <value>     (method_arg == 0x00000003)
// <value> can be float32 or int32
/////////////////////////////////////////////////////////////////////////////
static s32 OSC_SERVER_Method_MF_Config(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  OSC_SERVER_DebugMessage(osc_args, method_arg);
#endif

  // we expect at least 2 arguments
  if( osc_args->num_args < 2 )
    return -1; // wrong number of arguments

  // get MF number
  s32 mf;
  if( (mf=OSC_SERVER_GetIntOrFloat(osc_args->arg_type[0], osc_args->arg_ptr[0])) < 0 )
    return -2; // wrong argument type for first parameter

  // get configuration value
  float value;
  if( (value=OSC_SERVER_GetFloat(osc_args->arg_type[1], osc_args->arg_ptr[1])) < 0 )
    return -3; // wrong argument type for second parameter

  // get current configuration
  mios32_mf_config_t config = MIOS32_MF_ConfigGet(mf);

  // change value
  switch( method_arg ) {
    case 0x00000000:
      config.cfg.deadband = value;
      break;
    case 0x00000001:
      config.cfg.pwm_period = value;
      break;
    case 0x00000002:
      config.cfg.pwm_duty_cycle_down = value;
      break;
    case 0x00000003:
      config.cfg.pwm_duty_cycle_up = value;
      break;
    default:
      return -5; // invalid method_arg
  }

  // set config
  MIOS32_MF_ConfigSet(mf, config);

  // note: if motorfader doesn't exist, this will be silently ignored

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////


static const char str_cs[] = "cs";
static const char str_cfg[] = "cfg";

static const char str_led[] = "led";
static const char str_mf[] = "mf";

static const char str_set[] = "set";
static const char str_set_all[] = "set_all";

static const char str_deadband[] = "deadband";
static const char str_pwm_period[] = "pwm_period";
static const char str_pwm_duty_cycle_down[] = "pwm_duty_cycle_down";
static const char str_pwm_duty_cycle_up[] = "pwm_duty_cycle_up";


static mios32_osc_search_tree_t parse_cs_led[] = {
  { str_set, NULL, &OSC_SERVER_Method_LED_Set, 0x00000000 },
  { str_set_all, NULL, &OSC_SERVER_Method_LED_SetAll, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};

static mios32_osc_search_tree_t parse_cs_mf[] = {
  { str_set, NULL, &OSC_SERVER_Method_MF_Set, 0x00000000 },
  { str_set_all, NULL, &OSC_SERVER_Method_MF_SetAll, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};

static mios32_osc_search_tree_t parse_cfg_mf[] = { // using same method with different args to select the parameter
  { str_deadband, NULL, &OSC_SERVER_Method_MF_Config, 0x00000000 },
  { str_pwm_period, NULL, &OSC_SERVER_Method_MF_Config, 0x00000001 },
  { str_pwm_duty_cycle_down, NULL, &OSC_SERVER_Method_MF_Config, 0x00000002 },
  { str_pwm_duty_cycle_up, NULL, &OSC_SERVER_Method_MF_Config, 0x00000003 },
  { NULL, NULL, NULL, 0 } // terminator
};



static mios32_osc_search_tree_t parse_cs[] = {
  { str_led, parse_cs_led, NULL, 0 },
  { str_mf, parse_cs_mf, NULL, 0 },
  { NULL, NULL, NULL, 0 } // terminator
};

static mios32_osc_search_tree_t parse_cfg[] = {
  { str_mf, parse_cfg_mf, NULL, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};



static mios32_osc_search_tree_t parse_root[] = {
  { str_cs, parse_cs, NULL, 0x00000000 },
  { str_cfg, parse_cfg, NULL, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};
