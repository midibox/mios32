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


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_PORT_Init(u32 mode)
{
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
