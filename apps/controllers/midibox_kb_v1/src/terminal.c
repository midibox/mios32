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
#include "presets.h"
#include "mbkb_port.h"
#include "mbkb_router.h"
#include "terminal.h"
#include "keyboard.h"
#include "uip_terminal.h"
#include "midimon.h"
#include "tasks.h"

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

  if( KEYBOARD_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed by Keyboard Terminal

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      out("  set midimon <on|off>:             enables/disables the MIDI monitor");
      out("  set midimon_filter <on|off>:      enables/disables MIDI monitor filters");
      out("  set midimon_tempo <on|off>:       enables/disables the tempo display");
      KEYBOARD_TerminalHelp(_output_function);
      out("  set srio_num <1..16>:             max. number of scanned DIN/DOUT registers (currently: %d)", MIOS32_SRIO_ScanNumGet());
      UIP_TERMINAL_Help(_output_function);
      out("  router:                           print MIDI router info\n");
      out("  set router <node> <in-port> <off|channel|all> <out-port> <off|channel|all>: change router setting");
      out("  set mclk_in  <in-port>  <on|off>: change MIDI IN Clock setting");
      out("  set mclk_out <out-port> <on|off>: change MIDI OUT Clock setting");
      out("  store:                            stores current config as preset");
      out("  restore:                          restores config from preset");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "store") == 0 ) {
      s32 status = PRESETS_StoreAll();
      if( status >= 0 ) {
	out("Presets stored in internal EEPROM!");
      } else {
	out("ERROR: failed to store presets in internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "restore") == 0 ) {
      s32 status = PRESETS_Init(0);
      if( status >= 0 ) {
	out("Presets restored from internal EEPROM!");
      } else {
	out("ERROR: failed to restore presets from internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "router") == 0 ) {
      TERMINAL_PrintRouterInfo(out);
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
	} else if( strcmp(parameter, "srio_num") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the number of DIN/DOUTs which should be scanned (1..16)!");
	  } else {
	    int srs = get_dec(parameter);

	    if( srs < 1 || srs > 16 ) {
	      out("Number of DIN/DOUTs should be in the range between 1..16!");
	    } else {
	      MIOS32_SRIO_ScanNumSet(srs);
	      out("%d DINs and DOUTs will be scanned!", MIOS32_SRIO_ScanNumGet());
	    }
	  }
        } else if( strcmp(parameter, "router") == 0 ) {
          char *arg;
          if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
            out("Missing node number!");
          } else {
            s32 node = get_dec(arg);
            if( node < 1 || node > MBKB_ROUTER_NUM_NODES ) {
              out("Expecting node number between 1..%d!", MBKB_ROUTER_NUM_NODES);
            } else {
              node-=1; // user counts from 1

              if( !(arg = strtok_r(NULL, separators, &brkt)) ) {
                out("Missing input port!");
              } else {
                mios32_midi_port_t src_port = 0xff;
                int port_ix;
                for(port_ix=0; port_ix<MBKB_PORT_InNumGet(); ++port_ix) {
                  // terminate port name at first space
                  char port_name[10];
                  strcpy(port_name, MBKB_PORT_InNameGet(port_ix));
                  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

                  if( strcmp(arg, port_name) == 0 ) {
                    src_port = MBKB_PORT_InPortGet(port_ix);
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
                        for(port_ix=0; port_ix<MBKB_PORT_OutNumGet(); ++port_ix) {
                          // terminate port name at first space
                          char port_name[10];
                          strcpy(port_name, MBKB_PORT_OutNameGet(port_ix));
                          int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

                          if( strcmp(arg, port_name) == 0 ) {
                            dst_port = MBKB_PORT_OutPortGet(port_ix);
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
                              mbkb_router_node_entry_t *n = &mbkb_router_node[node];
                              n->src_port = src_port;
                              n->src_chn = src_chn;
                              n->dst_port = dst_port;
                              n->dst_chn = dst_chn;

                              out("Changed Node %d to SRC:%s %s  DST:%s %s",
                                  node+1,
                                  MBKB_PORT_InNameGet(MBKB_PORT_InIxGet(n->src_port)),
                                  arg_src_chn,
                                  MBKB_PORT_OutNameGet(MBKB_PORT_OutIxGet(n->dst_port)),
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
            for(port_ix=0; port_ix<MBKB_PORT_ClkNumGet(); ++port_ix) {
              // terminate port name at first space
              char port_name[10];
              strcpy(port_name, MBKB_PORT_ClkNameGet(port_ix));
              int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

              if( strcmp(arg, port_name) == 0 ) {
                mclk_port = MBKB_PORT_ClkPortGet(port_ix);
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
                  if( MBKB_ROUTER_MIDIClockInSet(mclk_port, on_off) < 0 )
                    out("Failed to set MIDI Clock port %s", arg);
                  else
                    out("Set MIDI Clock for IN port %s to %s\n", arg, arg_on_off);
                } else {
                  if( MBKB_ROUTER_MIDIClockOutSet(mclk_port, on_off) < 0 )
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
static s32 TERMINAL_PrintRouterInfo(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("MIDI Router Nodes (change with 'set router <in-port> <channel> <out-port> <channel>)");

  u8 node;
  mbkb_router_node_entry_t *n = &mbkb_router_node[0];
  for(node=0; node<MBKB_ROUTER_NUM_NODES; ++node, ++n) {

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
        MBKB_PORT_InNameGet(MBKB_PORT_InIxGet(n->src_port)),
        src_chn,
        MBKB_PORT_OutNameGet(MBKB_PORT_OutIxGet(n->dst_port)),
        dst_chn);
  }

  out("");
  out("MIDI Clock (change with 'set mclk_in <in-port> <on|off>' resp. 'set mclk_out <out-port> <on|off>')");

  int num_mclk_ports = MBKB_PORT_ClkNumGet();
  int port_ix;
  for(port_ix=0; port_ix<num_mclk_ports; ++port_ix) {
    mios32_midi_port_t mclk_port = MBKB_PORT_ClkPortGet(port_ix);

    s32 enab_rx = MBKB_ROUTER_MIDIClockInGet(mclk_port);
    if( !MBKB_PORT_ClkCheckAvailable(mclk_port) )
      enab_rx = -1; // MIDI In port not available

    s32 enab_tx = MBKB_ROUTER_MIDIClockOutGet(mclk_port);
    if( !MBKB_PORT_ClkCheckAvailable(mclk_port) )
      enab_tx = -1; // MIDI In port not available

    out("  %s  IN:%s  OUT:%s\n",
        MBKB_PORT_ClkNameGet(port_ix),
        (enab_rx == 0) ? "off" : ((enab_rx == 1) ? "on " : "---"),
        (enab_tx == 0) ? "off" : ((enab_tx == 1) ? "on " : "---"));
  }

  return 0; // no error
}
