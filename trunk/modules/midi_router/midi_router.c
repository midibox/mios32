// $Id$
/*
 * Generic MIDI Router functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
#include <osc_client.h>
#include "app.h"
#include "tasks.h"

#include "midi_router.h"
#include "midi_port.h"

#if MIDI_ROUTER_COMBINED_WITH_SEQ
#include <seq_midi_out.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// local defines
/////////////////////////////////////////////////////////////////////////////

// SysEx buffer for each input (exclusive Default)
#define NUM_SYSEX_BUFFERS     (MIDI_PORT_NUM_IN_PORTS-1)


/////////////////////////////////////////////////////////////////////////////
// global variables
/////////////////////////////////////////////////////////////////////////////
midi_router_node_entry_t midi_router_node[MIDI_ROUTER_NUM_NODES] = {
  // src chn   dst  chn
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
  { USB0,  0, UART0, 17 },
};

u32 midi_router_mclk_in;
u32 midi_router_mclk_out;

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 sysex_buffer[NUM_SYSEX_BUFFERS][MIDI_ROUTER_SYSEX_BUFFER_SIZE];
static u32 sysex_buffer_len[NUM_SYSEX_BUFFERS];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the MIDI router
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  //                     USB0 only     UART0..3       IIC0..3      OSC0..3
  midi_router_mclk_in = (0x01 << 0) | (0x0f << 8) | (0x0f << 16) | (0x00 << 24);
  //                      all ports
  midi_router_mclk_out = 0x00ffffff;

  // clear SysEx buffers and assign ports
  int i;
  for(i=0; i<NUM_SYSEX_BUFFERS; ++i)
    sysex_buffer_len[i] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns 32bit selection mask for USB0..7, UART0..7, IIC0..7, OSC0..7
/////////////////////////////////////////////////////////////////////////////
static inline u32 MIDI_ROUTER_PortMaskGet(mios32_midi_port_t port)
{
  u8 port_ix = port & 0xf;
  if( port >= USB0 && port <= OSC7 && port_ix <= 7 ) {
    return 1 << ((((port-USB0) & 0x30) >> 1) | port_ix);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // filter SysEx which is handled by separate parser
  if( midi_package.evnt0 < 0xf8 &&
      (midi_package.cin == 0xf ||
      (midi_package.cin >= 0x4 && midi_package.cin <= 0x7)) )
    return 0; // no error

  u32 sysex_dst_fwd_done = 0;
  int node;
  midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
  for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    if( n->src_chn && n->dst_chn && (n->src_port == port) ) {

      // forwarding OSC to OSC will very likely result into a stack overflow (or feedback loop) -> avoid this!
      if( ((port & 0xf0) == OSC0) && ((n->dst_port & 0xf0) == OSC0) )
	continue;

      if( midi_package.event >= NoteOff && midi_package.event <= PitchBend ) {
	if( n->src_chn == 17 || midi_package.chn == (n->src_chn-1) ) {
	  mios32_midi_package_t fwd_package = midi_package;
	  if( n->dst_chn <= 16 )
	    fwd_package.chn = (n->dst_chn-1);
	  mios32_midi_port_t port = n->dst_port;
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(port, fwd_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      } else {
	// Realtime events: ensure that they are only forwarded once
	u32 mask = MIDI_ROUTER_PortMaskGet(n->dst_port);
	if( !mask || !(sysex_dst_fwd_done & mask) ) {
	  sysex_dst_fwd_done |= mask;
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(n->dst_port, midi_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Receives a SysEx byte from APP_SYSEX_Parser (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  // determine SysEx buffer
  int sysex_in = MIDI_PORT_InIxGet(port);

  if( sysex_in == 0 )
    return -1; // not assigned

  // because DEFAULT is not buffered
  sysex_in -= 1;

  // just to ensure...
  if( sysex_in >= NUM_SYSEX_BUFFERS )
    return -2; // error in sysex assignments

  // store value into buffer, send when:
  //   o 0xf7 (end of stream) has been received
  //   o 0xf0 (start of stream) has been received although buffer isn't empty
  //   o buffer size has been exceeded (in this case, we switch to single byte send mode)
  // we check for (MIDI_ROUTER_SYSEX_BUFFER_SIZE-1), so that we always have a free byte for F7
  u32 buffer_len = sysex_buffer_len[sysex_in];
  if( midi_in == 0xf7 || (midi_in == 0xf0 && buffer_len != 0) || buffer_len >= (MIDI_ROUTER_SYSEX_BUFFER_SIZE-1) ) {

    if( midi_in == 0xf7 && buffer_len < MIDI_ROUTER_SYSEX_BUFFER_SIZE ) // note: we always have a free byte for F7
      sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;

    u32 sysex_dst_fwd_done = 0;
    int node;
    midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
    for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {
      if( n->src_chn && n->dst_chn && (n->src_port == port) ) {
	// SysEx, only forwarded once per destination port
	u32 mask = MIDI_ROUTER_PortMaskGet(n->dst_port);
	if( !mask || !(sysex_dst_fwd_done & mask) ) {
	  sysex_dst_fwd_done |= mask;

	  mios32_midi_port_t port = n->dst_port;
	  MUTEX_MIDIOUT_TAKE;
	  if( (port & 0xf0) == OSC0 )
	    OSC_CLIENT_SendSysEx(port & 0x0f, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
	  else
	    MIOS32_MIDI_SendSysEx(port, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }

    // empty buffer
    sysex_buffer_len[sysex_in] = 0;

    // fill with next byte if buffer size hasn't been exceeded
    if( midi_in != 0xf7 )
      sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;

  } else {
    // add to buffer
    sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns 1 if given port receives MIDI Clock
// Returns 0 if MIDI Clock In disabled
// Returns -1 if port not supported
// Returns -2 if MIDI In function disabled
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_MIDIClockInGet(mios32_midi_port_t port)
{
  // extra: MIDI IN Clock function not supported for IIC (yet)
  if( (port & 0xf0) == IIC0 )
    return -2; // MIDI In function disabled

  u32 mask = MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    return (midi_router_mclk_in & mask) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI In Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_MIDIClockInSet(mios32_midi_port_t port, u8 enable)
{
  u32 mask = MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    if( enable )
      midi_router_mclk_in |= mask;
    else
      midi_router_mclk_in &= ~mask;

    return 0; // no error
  }

  return -1; // port not supported
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if given port sends MIDI Clock
// Returns 0 if MIDI Clock Out disabled
// Returns -1 if port not supported
// Returns -2 if MIDI In function disabled
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_MIDIClockOutGet(mios32_midi_port_t port)
{
  u32 mask = MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    return (midi_router_mclk_out & mask) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI Out Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_MIDIClockOutSet(mios32_midi_port_t port, u8 enable)
{
  u32 mask = MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    if( enable )
      midi_router_mclk_out |= mask;
    else
      midi_router_mclk_out &= ~mask;

    return 0; // no error
  }

  return -1; // port not supported
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a MIDI clock/Start/Stop/Continue event to all output
// ports which have been enabled for this function.
// if bpm_tick == 0, the event will be sent immediately, otherwise it will
// be queued
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick)
{
  int i;

  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = 0x5; // Single-byte system common message
  p.evnt0 = evnt0;

  u32 port_mask = 0x00000001;
  for(i=0; i<32; ++i, port_mask<<=1) {
    if( midi_router_mclk_out & port_mask & 0xffffff0f ) { // filter USB5..USB8 to avoid unwanted clock events to non-existent ports
      // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
      mios32_midi_port_t port = (USB0 + ((i&0x18) << 1)) | (i&7);

      // TODO: special check for OSC, since MIOS32_MIDI_CheckAvailable() won't work here
      if( MIOS32_MIDI_CheckAvailable(port) ) {
	if( bpm_tick ) {
#if MIDI_ROUTER_COMBINED_WITH_SEQ
	  SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_ClkEvent, bpm_tick, 0);
#else
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(port, p);
	  MUTEX_MIDIOUT_GIVE;
#endif
	} else {
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(port, p);
	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }
  }

  return 0; // no error;
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
  if( strcmp(word, "on") == 0 || strcmp(word, "1") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 || strcmp(word, "0") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// print Syntax of "set router" command
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_ROUTER_TerminalSetRouterSyntax(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  set router <node> <in-port> <off|channel|all> <out-port> <off|channel|all>: change router setting");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("  router:                           print MIDI router info\n");
  MIDI_ROUTER_TerminalSetRouterSyntax(out);
  out("  set mclk_in  <in-port>  <on|off>: change MIDI IN Clock setting");
  out("  set mclk_out <out-port> <on|off>: change MIDI OUT Clock setting");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  int input_len = strlen(input);

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "router") == 0 ) {
      MIDI_ROUTER_TerminalPrintConfig(_output_function);
      return 1; // command taken
    } else if( strcmp(parameter, "set") == 0 ) {
      if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	out("Missing parameter after 'set'!");
	return 1; // command taken
      }

      if( strcmp(parameter, "router") == 0 ) {
	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing node number!");
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	s32 node = get_dec(parameter);
	if( node < 1 || node > MIDI_ROUTER_NUM_NODES ) {
	  out("Expecting node number between 1..%d!", MIDI_ROUTER_NUM_NODES);
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	node-=1; // the user counts from 1

	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing input port!");
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	mios32_midi_port_t src_port = 0xff;
	int port_ix;
	for(port_ix=0; port_ix<MIDI_PORT_InNumGet(); ++port_ix) {
	  // terminate port name at first space
	  char port_name[10];
	  strcpy(port_name, MIDI_PORT_InNameGet(port_ix));
	  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

	  if( strcasecmp(parameter, port_name) == 0 ) {
	    src_port = MIDI_PORT_InPortGet(port_ix);
	    break;
	  }
	}

	if( src_port >= 0xf0 ) {
	  out("Unknown or invalid MIDI input port!");
	  return 1; // command taken
	}

	char *arg_src_chn;
	if( !(arg_src_chn = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing source channel, expecting off, 1..16 or all!");
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	int src_chn = -1;
	if( strcmp(arg_src_chn, "---") == 0 || strcasecmp(arg_src_chn, "off") == 0 )
	  src_chn = 0;
	else if( strcasecmp(arg_src_chn, "All") == 0 )
	  src_chn = 17;
	else {
	  src_chn = get_dec(arg_src_chn);
	  if( src_chn > 16 )
	    src_chn = -1;
	}

	if( src_chn < 0 ) {
	  out("Invalid source channel, expecting off, 1..16 or all!");
	  return 1; // command taken
	}

	if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing output port!");
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	mios32_midi_port_t dst_port = 0xff;
	for(port_ix=0; port_ix<MIDI_PORT_OutNumGet(); ++port_ix) {
	  // terminate port name at first space
	  char port_name[10];
	  strcpy(port_name, MIDI_PORT_OutNameGet(port_ix));
	  int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

	  if( strcasecmp(parameter, port_name) == 0 ) {
	    dst_port = MIDI_PORT_OutPortGet(port_ix);
	    break;
	  }
	}

	if( dst_port >= 0xf0 ) {
	  out("Unknown or invalid MIDI output port!");
	  return 1; // command taken
	}

	char *arg_dst_chn;
	if( !(arg_dst_chn = strtok_r(NULL, separators, &brkt)) ) {
	  out("Missing destination channel, expecting off, 1..16 or all!");
	  MIDI_ROUTER_TerminalSetRouterSyntax(out);
	  return 1; // command taken
	}

	int dst_chn = -1;
	if( strcmp(arg_dst_chn, "---") == 0 || strcasecmp(arg_dst_chn, "off") == 0 )
	  dst_chn = 0;
	else if( strcasecmp(arg_dst_chn, "All") == 0 )
	  dst_chn = 17;
	else {
	  dst_chn = get_dec(arg_dst_chn);
	  if( dst_chn > 16 )
	    dst_chn = -1;
	}

	if( dst_chn < 0 ) {
	  out("Invalid destination channel, expecting off, 1..16 or all!");
	  return 1; // command taken
	}

	//
	// finally...
	//
	midi_router_node_entry_t *n = &midi_router_node[node];
	n->src_port = src_port;
	n->src_chn = src_chn;
	n->dst_port = dst_port;
	n->dst_chn = dst_chn;

	out("Changed Node %d to SRC:%s %s  DST:%s %s",
	    node+1,
	    MIDI_PORT_InNameGet(MIDI_PORT_InIxGet(n->src_port)),
	    arg_src_chn,
	    MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(n->dst_port)),
	    arg_dst_chn);
	return 1; // command taken

      } else if( strcmp(parameter, "mclk_in") == 0 || strcmp(parameter, "mclk_out") == 0 ) {
          int mclk_in = strcmp(parameter, "mclk_in") == 0;

          if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
            out("Missing MIDI clock port!");
	    return 1; // command taken
          } else {
            mios32_midi_port_t mclk_port = 0xff;
            int port_ix;
            for(port_ix=0; port_ix<MIDI_PORT_ClkNumGet(); ++port_ix) {
              // terminate port name at first space
              char port_name[10];
              strcpy(port_name, MIDI_PORT_ClkNameGet(port_ix));
              int i; for(i=0; i<strlen(port_name); ++i) if( port_name[i] == ' ' ) port_name[i] = 0;

              if( strcasecmp(parameter, port_name) == 0 ) {
                mclk_port = MIDI_PORT_ClkPortGet(port_ix);
                break;
              }
            }

            if( mclk_in && mclk_port >= 0xf0 ) {
              // extra: allow 'INx' as well
              if( strncmp(parameter, "IN", 2) == 0 && parameter[2] >= '1' && parameter[2] <= '4' )
                mclk_port = UART0 + (parameter[2] - '1');
            }

            if( !mclk_in && mclk_port >= 0xf0 ) {
              // extra: allow 'OUTx' as well
              if( strncmp(parameter, "OUT", 3) == 0 && parameter[3] >= '1' && parameter[3] <= '4' )
                mclk_port = UART0 + (parameter[3] - '1');
            }

            if( mclk_port >= 0xf0 ) {
              out("Unknown or invalid MIDI Clock port!");
	      return 1; // command taken
	    }
	    
	    int on_off = -1;
	    char *arg_on_off;
	    if( !(arg_on_off = strtok_r(NULL, separators, &brkt)) ||
		(on_off = get_on_off(arg_on_off)) < 0 ) {
	      out("Missing 'on' or 'off' after port name!");
	      return 1; // command taken
	    }

	    if( mclk_in ) {
	      if( MIDI_ROUTER_MIDIClockInSet(mclk_port, on_off) < 0 )
		out("Failed to set MIDI Clock port %s", parameter);
	      else
		out("Set MIDI Clock for IN port %s to %s\n", parameter, arg_on_off);
	    } else {
	      if( MIDI_ROUTER_MIDIClockOutSet(mclk_port, on_off) < 0 )
		out("Failed to set MIDI Clock port %s", parameter);
	      else
		out("Set MIDI Clock for OUT port %s to %s\n", parameter, arg_on_off);
	    }
	    return 1; // command taken
	  }
      } else {
	// out("Unknown command - type 'help' to list available commands!");
      }
    }
  }

  // restore input line (replace NUL characters by spaces)
  int i;
  char *input_ptr = input;
  for(i=0; i<input_len; ++i, ++input_ptr)
    if( !*input_ptr )
      *input_ptr = ' ';

  return 0; // command not taken
}


/////////////////////////////////////////////////////////////////////////////
// Keyboard Configuration (can also be called from external)
/////////////////////////////////////////////////////////////////////////////
s32 MIDI_ROUTER_TerminalPrintConfig(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("MIDI Router Nodes (change with 'set router <node> <in-port> <channel> <out-port> <channel>)");
  out("Example: set router 1 IN1 all USB1 all");

  u8 node;
  midi_router_node_entry_t *n = &midi_router_node[0];
  for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n) {

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
        MIDI_PORT_InNameGet(MIDI_PORT_InIxGet(n->src_port)),
        src_chn,
        MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(n->dst_port)),
        dst_chn);
  }

  out("");
  out("MIDI Clock (change with 'set mclk_in <in-port> <on|off>' resp. 'set mclk_out <out-port> <on|off>')");

  int num_mclk_ports = MIDI_PORT_ClkNumGet();
  int port_ix;
  for(port_ix=0; port_ix<num_mclk_ports; ++port_ix) {
    mios32_midi_port_t mclk_port = MIDI_PORT_ClkPortGet(port_ix);

    s32 enab_rx = MIDI_ROUTER_MIDIClockInGet(mclk_port);
    if( !MIDI_PORT_ClkCheckAvailable(mclk_port) )
      enab_rx = -1; // MIDI In port not available

    s32 enab_tx = MIDI_ROUTER_MIDIClockOutGet(mclk_port);
    if( !MIDI_PORT_ClkCheckAvailable(mclk_port) )
      enab_tx = -1; // MIDI In port not available

    out("  %s  IN:%s  OUT:%s\n",
        MIDI_PORT_ClkNameGet(port_ix),
        (enab_rx == 0) ? "off" : ((enab_rx == 1) ? "on " : "---"),
        (enab_tx == 0) ? "off" : ((enab_tx == 1) ? "on " : "---"));
  }

  return 0; // no error
}
