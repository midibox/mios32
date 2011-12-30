// $Id$
/*
 * MIDI Port functions for MIDIO128 V3
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

#include "midio_port.h"

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_port_t port;
  char name[5];
} midio_port_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// has to be kept in synch with MIDIO_PORT_InIxGet() !!!
static const midio_port_entry_t in_ports[MIDIO_PORT_NUM_IN_PORTS] = {
  // port ID  Name
  { DEFAULT, "Def." },
  { USB0,    "USB1" },
  { UART0,   "IN1 " },
  { UART1,   "IN2 " },
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
};

// has to be kept in synch with MIDIO_PORT_OutIxGet() !!!
static const midio_port_entry_t out_ports[MIDIO_PORT_NUM_OUT_PORTS] = {
  // port ID  Name
  { DEFAULT, "Def." },
  { USB0,    "USB1" },
  { UART0,   "OUT1" },
  { UART1,   "OUT2" },
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
};

// has to be kept in synch with MIDIO_PORT_ClkIxGet() !!!
static const midio_port_entry_t clk_ports[MIDIO_PORT_NUM_CLK_PORTS] = {
  // port ID  Name
  { USB0,    "USB1" },
  { UART0,   "MID1" },
  { UART1,   "MID2" },
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
};

// for MIDI In/Out monitor
static u8 midi_out_ctr[MIDIO_PORT_NUM_OUT_PORTS];
static mios32_midi_package_t midi_out_package[MIDIO_PORT_NUM_OUT_PORTS];

static u8 midi_in_ctr[MIDIO_PORT_NUM_IN_PORTS];
static mios32_midi_package_t midi_in_package[MIDIO_PORT_NUM_IN_PORTS];

static midio_port_mon_filter_t midio_port_mon_filter;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_Init(u32 mode)
{
  int i;

  // init MIDI In/Out monitor variables
  for(i=0; i<MIDIO_PORT_NUM_OUT_PORTS; ++i) {
    midi_out_ctr[i] = 0;
    midi_out_package[i].ALL = 0;
  }

  for(i=0; i<MIDIO_PORT_NUM_IN_PORTS; ++i) {
    midi_in_ctr[i] = 0;
    midi_in_package[i].ALL = 0;
  }

  midio_port_mon_filter.ALL = 0;
  midio_port_mon_filter.MIDI_CLOCK = 1;
  midio_port_mon_filter.ACTIVE_SENSE = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of available MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_InNumGet(void)
{
  return MIDIO_PORT_NUM_IN_PORTS;
}

s32 MIDIO_PORT_OutNumGet(void)
{
  return MIDIO_PORT_NUM_OUT_PORTS;
}

s32 MIDIO_PORT_ClkNumGet(void)
{
  return MIDIO_PORT_NUM_CLK_PORTS;
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of MIDI IN/OUT port in (4 characters + zero terminator)
// port_ix is within 0..MIDIO_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
char *MIDIO_PORT_InNameGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_IN_PORTS )
    return "----";
  else
    return (char *)in_ports[port_ix].name;
}

char *MIDIO_PORT_OutNameGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_OUT_PORTS )
    return "----";
  else
    return (char *)out_ports[port_ix].name;
}

char *MIDIO_PORT_ClkNameGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_CLK_PORTS )
    return "----";
  else
    return (char *)clk_ports[port_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MIOS32 MIDI Port ID of a MIDI IN/OUT port
// port_ix is within 0..MIDIO_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t MIDIO_PORT_InPortGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_IN_PORTS )
    return 0xff; // dummy interface
  else
    return in_ports[port_ix].port;
}

mios32_midi_port_t MIDIO_PORT_OutPortGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_OUT_PORTS )
    return 0xff; // dummy interface
  else
    return out_ports[port_ix].port;
}

mios32_midi_port_t MIDIO_PORT_ClkPortGet(u8 port_ix)
{
  if( port_ix >= MIDIO_PORT_NUM_CLK_PORTS )
    return 0xff; // dummy interface
  else
    return clk_ports[port_ix].port;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MBSEQ MIDI Port Index of a MIOS32 MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
u8 MIDIO_PORT_InIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_IN_PORTS; ++ix) {
    if( in_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}

u8 MIDIO_PORT_OutIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}

u8 MIDIO_PORT_ClkIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port )
      return ix;
  }

  return 0; // return first ix if not found
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if MIOS32 MIDI In/Out Port is available
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_InCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_IN_PORTS; ++ix) {
    if( in_ports[ix].port == port ) {
      if( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

s32 MIDIO_PORT_OutCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port ) {
      if( port == 0x80 ) {
	//return SEQ_CV_IfGet() ? 1 : 0;
	return 0;
      } else if ( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

s32 MIDIO_PORT_ClkCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<MIDIO_PORT_NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port ) {
      if( port == 0x80 ) {
	//return SEQ_CV_IfGet() ? 1 : 0;
	return 0;
      } else if ( port >= 0xf0 )
	return 1; // Bus is always available
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
      else
	return MIOS32_MIDI_CheckAvailable(port);
    }
  }
  return 0; // port not available
}

/////////////////////////////////////////////////////////////////////////////
// Returns the last sent MIDI package of given output port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MIDIO_PORT_OutPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = MIDIO_PORT_OutIxGet(port);

    if( !port_ix )
      return empty; // port not supported...
  }

  if( !midi_out_ctr[port_ix] )
    return empty; // package expired

  return midi_out_package[port_ix];
}


/////////////////////////////////////////////////////////////////////////////
// Returns the last received MIDI package of given input port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MIDIO_PORT_InPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = MIDIO_PORT_InIxGet(port);

    if( !port_ix )
      return empty; // port not supported...
  }

  if( !midi_in_ctr[port_ix] )
    return empty; // package expired

  return midi_in_package[port_ix];
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set MIDI monitor filter option
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_MonFilterSet(midio_port_mon_filter_t filter)
{
  midio_port_mon_filter = filter;
  return 0; // no error
}

midio_port_mon_filter_t MIDIO_PORT_MonFilterGet(void)
{
  return midio_port_mon_filter;
}


/////////////////////////////////////////////////////////////////////////////
// Handles the MIDI In/Out counters
// Should be called periodically each mS
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_Period1mS(void)
{
  int i;
  static u8 predivider = 0;

  // counters are handled each 10 mS
  if( ++predivider >= 10 ) {
    predivider = 0;

    for(i=0; i<MIDIO_PORT_NUM_OUT_PORTS; ++i)
      if( midi_out_ctr[i] )
	--midi_out_ctr[i];

    for(i=0; i<MIDIO_PORT_NUM_IN_PORTS; ++i)
      if( midi_in_ctr[i] )
	--midi_in_ctr[i];
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Tx Notificaton hook if a MIDI event should be sent
// Allows to provide additional MIDI ports
// If 1 is returned, package will be filtered!
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI Out monitor function
  u8 mon_filtered = 0;
  if( midio_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( midio_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = MIDIO_PORT_OutIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_out_package[port_ix] = package;
      midi_out_ctr[port_ix] = 20; // 2 seconds lifetime
    }
  }


  // DIN Sync Event (0xf9 sent over port 0xff)
  if( port == 0xff && package.evnt0 == 0xf9 ) {
    //seq_core_din_sync_pulse_ctr = 2 + SEQ_CV_ClkPulseWidthGet(); // to generate a pulse with configurable length (+1 for 1->0 transition, +1 to compensate jitter)
    return 1; // filter package
  }

  if( (port & 0xf0) == OSC0 ) { // OSC1..4 port
    if( OSC_CLIENT_SendMIDIEvent(port & 0x0f, package) >= 0 )
      return 1; // filter package
  }

  return 0; // don't filter package
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Rx Notificaton hook in app.c if a MIDI event is received
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI In monitor function
  u8 mon_filtered = 0;
  if( midio_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( midio_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = MIDIO_PORT_InIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_in_package[port_ix] = package;
      midi_in_ctr[port_ix] = 20; // 2 seconds lifetime
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of an event in a string
// num_chars: currently only 5 supported, long event names could be added later
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_EventNameGet(mios32_midi_package_t package, char *label, u8 num_chars)
{
  // currently only 5 chars supported...
  if( package.type == 0xf || package.evnt0 >= 0xf8 ) {
    switch( package.evnt0 ) {
    case 0xf8: strcpy(label, " CLK "); break;
    case 0xfa: strcpy(label, "START"); break;
    case 0xfb: strcpy(label, "CONT."); break;
    case 0xfc: strcpy(label, "STOP "); break;
    default:
      sprintf(label, " %02X  ", package.evnt0);
    }
  } else if( package.type < 8 ) {
    strcpy(label, "SysEx");
  } else {
    switch( package.event ) {
      case NoteOff:
      case NoteOn:
      case PolyPressure:
      case CC:
      case ProgramChange:
      case Aftertouch:
      case PitchBend:
	// could be enhanced later
        sprintf(label, "%02X%02X ", package.evnt0, package.evnt1);
	break;

      default:
	// print first two bytes for unsupported events
        sprintf(label, "%02X%02X ", package.evnt0, package.evnt1);
    }
  }

#if 0
  // TODO: enhanced messages
  if( num_chars > 5 ) {
  }
#endif

  return 0; // no error
}
