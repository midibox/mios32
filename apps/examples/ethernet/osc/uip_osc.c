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
#include <string.h>

#include "uip_osc.h"
#include "uip.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static struct uip_udp_conn *osc_conn = NULL;


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC daemon
/////////////////////////////////////////////////////////////////////////////
s32 UIP_OSC_Init(u32 mode)
{
  // remove open connection
  if(osc_conn != NULL) {
    uip_udp_remove(osc_conn);
  }

  // create new connection
  uip_ipaddr_t ripaddr;
  uip_ipaddr(ripaddr, 10,0,0,2);

  if( (osc_conn=uip_udp_new(&ripaddr, HTONS(UIP_OSC_PORT))) != NULL ) {
    uip_udp_bind(osc_conn, HTONS(UIP_OSC_PORT));
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[UIP_OSC] listen to %d.%d.%d.%d:%d\n", 
				 (osc_conn->ripaddr[0] >> 0) & 0xff,
				 (osc_conn->ripaddr[0] >> 8) & 0xff,
				 (osc_conn->ripaddr[1] >> 0) & 0xff,
				 (osc_conn->ripaddr[1] >> 8) & 0xff,
				 HTONS(osc_conn->rport));
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

const char str_sid1[] = "sid1";
const char str_sid2[] = "sid2";
const char str_sid3[] = "sid3";
const char str_sid4[] = "sid4";

const char str_osc[] = "osc";
const char str_filter[] = "filter";

const char str_transpose[] = "transpose";
const char str_finetune[] = "finetune";
const char str_pulsewidth[] = "pulsewidth";

const char str_cutoff[] = "cutoff";
const char str_resonance[] = "resonance";

void osc_method(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
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
#endif  
}

mios32_osc_search_tree_t parse_osc[] = {
  { str_transpose, NULL, &osc_method, 0x00000001 },
  { str_finetune, NULL, &osc_method, 0x00000002 },
  { str_pulsewidth, NULL, &osc_method, 0x00000003 },
  { NULL, NULL, NULL, 0 } // terminator
};

mios32_osc_search_tree_t parse_filter[] = {
  { str_cutoff, NULL, &osc_method, 0x00000004 },
  { str_resonance, NULL, &osc_method, 0x00000005 },
  { NULL, NULL, NULL, 0 } // terminator
};

mios32_osc_search_tree_t parse_sid[] = {
  { str_osc, parse_osc, NULL, 0 },
  { str_filter, parse_filter, NULL, 0 },
  { NULL, NULL, NULL, 0 } // terminator
};

mios32_osc_search_tree_t parse_root[] = {
  { str_sid1, parse_sid, NULL, 0 },
  { str_sid2, parse_sid, NULL, 0 },
  { str_sid3, parse_sid, NULL, 0 },
  { str_sid4, parse_sid, NULL, 0 },
  { NULL, NULL, NULL, 0 } // terminator
};


s32 UIP_OSC_AppCall(void)
{
  if( uip_udp_conn->rport == HTONS(UIP_OSC_PORT) ) {
    if( uip_poll() ) {
      // called each second
    }

    if( uip_newdata() ) {
      // new UDP package has been received
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugMessage("[UIP_OSC] Received Datagram from %d.%d.%d.%d:%d (%d bytes)\n", 
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
	MIOS32_MIDI_SendDebugMessage("[UIP_OSC] invalid OSC packet, status %d\n", status);
#endif
      }
    }
  }
}
