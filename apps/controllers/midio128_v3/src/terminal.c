// $Id$
/*
 * The command/configuration Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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

#include "app.h"
#include "midio_patch.h"
#include "midio_port.h"
#include "midio_router.h"
#include "terminal.h"
#include "uip_terminal.h"
#include "midimon.h"
#include "tasks.h"
#include "midio_file.h"
#include "midio_file_p.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_PrintSystem(void *_output_function);
static s32 TERMINAL_PrintRouterInfo(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( UIP_TERMINAL_ParseLine(input, _output_function) > 0 )
    return 0; // command parsed by UIP Terminal

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      UIP_TERMINAL_Help(_output_function);
      out("  set midimon <on|off>:             enables/disables the MIDI monitor");
      out("  set midimon_filter <on|off>:      enables/disables MIDI monitor filters");
      out("  set midimon_tempo <on|off>:       enables/disables the tempo display");
      out("  set dout <pin> <0|1>:             directly sets DOUT (all or 0..%d) to given level (1 or 0)", MIOS32_SRIO_NUM_SR*8 - 1);
      out("  save <name>:                      stores current config on SD Card");
      out("  load <name>:                      restores config from SD Card");
      out("  show:                             shows the current configuration file");
      out("  msd <on|off>:                     enables Mass Storage Device driver");
      out("  router:                           print MIDI router info\n");
      out("  set router <in-port> <off|channel|all> <out-port> <off|channel|all>: change router setting");
      out("  set mclk_in  <in-port>  <on|off>: change MIDI IN Clock setting");
      out("  set mclk_out <out-port> <on|off>: change MIDI OUT Clock setting");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "msd") == 0 ) {
      char *arg = NULL;
      if( (arg = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(arg, "on") == 0 ) {
	  if( TASK_MSD_EnableGet() ) {
	    out("Mass Storage Device Mode already activated!\n");
	  } else {
	    out("Mass Storage Device Mode activated - USB MIDI will be disabled!!!\n");
	    // wait a second to ensure that this message is print in MIOS Terminal
	    int d;
	    for(d=0; d<1000; ++d)
	      MIOS32_DELAY_Wait_uS(1000);
	    // activate MSD mode
	    TASK_MSD_EnableSet(1);
	  }
	} else if( strcmp(arg, "off") == 0 ) {
	  if( !TASK_MSD_EnableGet() ) {
	    out("Mass Storage Device Mode already deactivated!\n");
	  } else {
	    out("Mass Storage Device Mode deactivated - USB MIDI will be available again.n");
	    TASK_MSD_EnableSet(0);
	  }
	} else
	  arg = NULL;
      }
      if( arg == NULL ) {
	out("Please enter 'msd on' or 'msd off'\n");
      }      
    } else if( strcmp(parameter, "save") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("ERROR: please specify filename for patch (up to 8 characters)!");
      } else {
	if( strlen(parameter) > 8 ) {
	  out("ERROR: 8 characters maximum!");
	} else {
	  s32 status = MIDIO_PATCH_Store(parameter);
	  if( status >= 0 ) {
	    out("Patch '%s' stored on SD Card!", parameter);
	  } else {
	    out("ERROR: failed to store patch '%s' on SD Card (status %d)!", parameter, status);
	  }
	}
      }
    } else if( strcmp(parameter, "load") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("ERROR: please specify filename for patch (up to 8 characters)!");
      } else {
	if( strlen(parameter) > 8 ) {
	  out("ERROR: 8 characters maximum!");
	} else {
	  s32 status = MIDIO_PATCH_Load(parameter);
	  if( status >= 0 ) {
	    out("Patch '%s' loaded from SD Card!", parameter);
	  } else {
	    out("ERROR: failed to load patch '%s' on SD Card (status %d)!", parameter, status);
	  }
	}
      }
    } else if( strcmp(parameter, "router") == 0 ) {
      TERMINAL_PrintRouterInfo(out);
    } else if( strcmp(parameter, "show") == 0 ) {
      MIDIO_FILE_P_Debug();
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "midimon") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_ActiveSet(on_off);
	    out("MIDI Monitor %s!", MIDIMON_ActiveGet() ? "enabled" : "disabled");
	  }
	} else if( strcmp(parameter, "midimon_filter") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_FilterActiveSet(on_off);
	    out("MIDI Monitor Filter %s!", MIDIMON_FilterActiveGet() ? "enabled" : "disabled");
	  }
	} else if( strcmp(parameter, "midimon_tempo") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_TempoActiveSet(on_off);
	    out("MIDI Monitor Tempo Display %s!", MIDIMON_TempoActiveGet() ? "enabled" : "disabled");
	  }
	} else if( strcmp(parameter, "dout") == 0 ) {
	  s32 pin = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	    if( strcmp(parameter, "all") == 0 ) {
	      pin = -42;
	    } else {
	      pin = get_dec(parameter);
	    }
	  }

	  if( (pin < 0 && pin != -42) || pin >= (MIOS32_SRIO_NUM_SR*8) ) {
	    out("Pin number should be between 0..%d", MIOS32_SRIO_NUM_SR*8 - 1);
	  } else {
	    s32 value = -1;
	    if( (parameter = strtok_r(NULL, separators, &brkt)) )
	      value = get_dec(parameter);

	    if( value < 0 || value > 1 ) {
	      out("Expecting value 1 or 0 for DOUT pin %d", pin);
	    } else {
	      if( pin == -42 ) {
		for(pin=0; pin<(MIOS32_SRIO_NUM_SR*8); ++pin)
		  MIOS32_DOUT_PinSet(pin, value);
		out("All DOUT pins set to %d", value);
	      } else {
		MIOS32_DOUT_PinSet(pin, value);
		out("DOUT Pin %d (SR#%d.D%d) set to %d", pin, (pin/8)+1, 7-(pin%8), value);
	      }
	    }
	  }
	} else if( strcmp(parameter, "router") == 0 ) {
	  char *arg;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Missing node number!");
	  } else {
	    s32 node = get_dec(arg);
	    if( node < 1 || node > MIDIO_PATCH_NUM_ROUTER ) {
	      out("Expecting node number between 1..%d!", MIDIO_PATCH_NUM_ROUTER);
	    } else {
	      node-=1; // user counts from 1

	      if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
		out("Missing input port!");
	      } else {
		mios32_midi_port_t src_port = 0xff;
		int port_ix;
		for(port_ix=0; port_ix<MIDIO_PORT_InNumGet(); ++port_ix) {
		  // terminate port name at first space
		  char port_name[10];
		  strcpy(port_name, MIDIO_PORT_InNameGet(port_ix));
		  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

		  if( strcmp(arg, port_name) == 0 ) {
		    src_port = MIDIO_PORT_InPortGet(port_ix);
		    break;
		  }
		}

		if( src_port >= 0xf0 ) {
		  out("Unknown or invalid MIDI input port!");
		} else {

		  char *arg_src_chn;
		  if( !(arg_src_chn = strtok_r(NULL, separators, &brkt)) ) {
		    out("Missing source channel, expecting off, 1..16 or all!");
		  } else {
		    int src_chn = -1;

		    if( strcmp(arg_src_chn, "---") == 0 || strcmp(arg_src_chn, "off") == 0 )
		      src_chn = 0;
		    else if( strcmp(arg_src_chn, "All") == 0 || strcmp(arg_src_chn, "all") == 0 )
		      src_chn = 17;
		    else {
		      src_chn = get_dec(arg_src_chn);
		      if( src_chn > 16 )
			src_chn = -1;
		    }

		    if( src_chn < 0 ) {
		      out("Invalid source channel, expecting off, 1..16 or all!");
		    } else {

		      if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
			out("Missing output port!");
		      } else {
			mios32_midi_port_t dst_port = 0xff;
			int port_ix;
			for(port_ix=0; port_ix<MIDIO_PORT_OutNumGet(); ++port_ix) {
			  // terminate port name at first space
			  char port_name[10];
			  strcpy(port_name, MIDIO_PORT_OutNameGet(port_ix));
			  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

			  if( strcmp(arg, port_name) == 0 ) {
			    dst_port = MIDIO_PORT_OutPortGet(port_ix);
			    break;
			  }
			}

			if( dst_port >= 0xf0 ) {
			  out("Unknown or invalid MIDI output port!");
			} else {

			  char *arg_dst_chn;
			  if( !(arg_dst_chn = strtok_r(NULL, separators, &brkt)) ) {
			    out("Missing destination channel, expecting off, 1..16 or all!");
			  } else {
			    int dst_chn = -1;

			    if( strcmp(arg_dst_chn, "---") == 0 || strcmp(arg_dst_chn, "off") == 0 )
			      dst_chn = 0;
			    else if( strcmp(arg_dst_chn, "All") == 0 || strcmp(arg_dst_chn, "all") == 0 )
			      dst_chn = 17;
			    else {
			      dst_chn = get_dec(arg_dst_chn);
			      if( dst_chn > 16 )
				dst_chn = -1;
			    }

			    if( dst_chn < 0 ) {
			      out("Invalid destination channel, expecting off, 1..16 or all!");
			    } else {
			      //
			      // finally...
			      //
			      midio_patch_router_entry_t *n = &midio_patch_router[0];
			      n->src_port = src_port;
			      n->src_chn = src_chn;
			      n->dst_port = dst_port;
			      n->dst_chn = dst_chn;

			      out("Changed Node %d to SRC:%s %s  DST:%s %s",
				  node+1,
				  MIDIO_PORT_InNameGet(MIDIO_PORT_InIxGet(n->src_port)),
				  arg_src_chn,
				  MIDIO_PORT_OutNameGet(MIDIO_PORT_OutIxGet(n->dst_port)),
				  arg_dst_chn);
			    }
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	} else if( strcmp(parameter, "mclk_in") == 0 || strcmp(parameter, "mclk_out") == 0 ) {
	  int mclk_in = strcmp(parameter, "mclk_in") == 0;

	  char *arg;
	  if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
	    out("Missing MIDI clock port!");
	  } else {
	    mios32_midi_port_t mclk_port = 0xff;
	    int port_ix;
	    for(port_ix=0; port_ix<MIDIO_PORT_ClkNumGet(); ++port_ix) {
	      // terminate port name at first space
	      char port_name[10];
	      strcpy(port_name, MIDIO_PORT_ClkNameGet(port_ix));
	      int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

	      if( strcmp(arg, port_name) == 0 ) {
		mclk_port = MIDIO_PORT_ClkPortGet(port_ix);
		break;
	      }
	    }

	    if( mclk_in && mclk_port >= 0xf0 ) {
	      // extra: allow 'INx' as well
	      if( strncmp(arg, "IN", 2) == 0 && arg[2] >= '1' && arg[2] <= '4' )
		mclk_port = UART0 + (arg[2] - '1');
	    }

	    if( !mclk_in && mclk_port >= 0xf0 ) {
	      // extra: allow 'OUTx' as well
	      if( strncmp(arg, "OUT", 3) == 0 && arg[3] >= '1' && arg[3] <= '4' )
		mclk_port = UART0 + (arg[3] - '1');
	    }

	    if( mclk_port >= 0xf0 ) {
	      out("Unknown or invalid MIDI Clock port!");
	    } else {
	      int on_off = -1;
	      char *arg_on_off;
	      if( !(arg_on_off = strtok_r(NULL, separators, &brkt)) ||
		  (on_off = get_on_off(arg_on_off)) < 0 ) {
		out("Missing 'on' or 'off' after port name!");
	      } else {
		if( mclk_in ) {
		  if( MIDIO_ROUTER_MIDIClockInSet(mclk_port, on_off) < 0 )
		    out("Failed to set MIDI Clock port %s", arg);
		  else
		    out("Set MIDI Clock for IN port %s to %s\n", arg, arg_on_off);
		} else {
		  if( MIDIO_ROUTER_MIDIClockOutSet(mclk_port, on_off) < 0 )
		    out("Failed to set MIDI Clock port %s", arg);
		  else
		    out("Set MIDI Clock for OUT port %s to %s\n", arg, arg_on_off);
		}
	      }
	    }
	  }
	} else {
	  out("Unknown set parameter: '%s'!", parameter);
	}
      } else {
	out("Missing parameter after 'set'!");
      }
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// System Informations
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Application: " MIOS32_LCD_BOOT_MSG_LINE1);

  out("MIDI Monitor: %s", MIDIMON_ActiveGet() ? "enabled" : "disabled");
  out("MIDI Monitor Filters: %s", MIDIMON_FilterActiveGet() ? "enabled" : "disabled");
  out("MIDI Monitor Tempo Display: %s", MIDIMON_TempoActiveGet() ? "enabled" : "disabled");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Router Informations
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_PrintRouterInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("MIDI Router Nodes (change with 'set router <in-port> <channel> <out-port> <channel>)");

  u8 node;
  midio_patch_router_entry_t *n = &midio_patch_router[0];
  for(node=0; node<MIDIO_PATCH_NUM_ROUTER; ++node, ++n) {

    char src_chn[10];
    if( !n->src_chn )
      sprintf(src_chn, "off");
    else if( n->src_chn > 16 )
      sprintf(src_chn, "all");
    else
      sprintf(src_chn, "#%2d", n->src_chn);

    char dst_chn[10];
    if( !n->dst_chn )
      sprintf(dst_chn, "off");
    else if( n->dst_chn > 16 )
      sprintf(dst_chn, "all");
    else
      sprintf(dst_chn, "#%2d", n->dst_chn);

    out("  %2d  SRC:%s %s  DST:%s %s",
	node+1,
	MIDIO_PORT_InNameGet(MIDIO_PORT_InIxGet(n->src_port)),
	src_chn,
	MIDIO_PORT_OutNameGet(MIDIO_PORT_OutIxGet(n->dst_port)),
	dst_chn);
  }

  out("");
  out("MIDI Clock (change with 'set mclk_in <in-port> <on|off>' resp. 'set mclk_out <out-port> <on|off>')");

  int num_mclk_ports = MIDIO_PORT_ClkNumGet();
  int port_ix;
  for(port_ix=0; port_ix<num_mclk_ports; ++port_ix) {
    mios32_midi_port_t mclk_port = MIDIO_PORT_ClkPortGet(port_ix);

    s32 enab_rx = MIDIO_ROUTER_MIDIClockInGet(mclk_port);
    if( !MIDIO_PORT_ClkCheckAvailable(mclk_port) )
      enab_rx = -1; // MIDI In port not available

    s32 enab_tx = MIDIO_ROUTER_MIDIClockOutGet(mclk_port);
    if( !MIDIO_PORT_ClkCheckAvailable(mclk_port) )
      enab_tx = -1; // MIDI In port not available

    out("  %s  IN:%s  OUT:%s\n",
	MIDIO_PORT_ClkNameGet(port_ix),
	(enab_rx == 0) ? "off" : ((enab_rx == 1) ? "on " : "---"),
	(enab_tx == 0) ? "off" : ((enab_tx == 1) ? "on " : "---"));
  }

  return 0; // no error
}

