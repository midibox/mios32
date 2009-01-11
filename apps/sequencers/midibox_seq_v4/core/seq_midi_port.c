// $Id$
/*
 * MIDI port functions of MIDIbox SEQ
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "seq_midi_port.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_port_t port;
  char name[5];
} seq_midi_port_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const seq_midi_port_entry_t in_ports[] = {
  // port ID  Name
  { DEFAULT, "Def." },
  { USB0,    "USB " },
  { UART0,   "IN1 " },
  { UART1,   "IN2 " },
  { IIC0,    "IIC1" },
  { 0xf0,    "Bus1" },
  { 0xf1,    "Bus2" },
  { 0xf2,    "Bus3" },
  { 0xf3,    "Bus4" }
};

static const seq_midi_port_entry_t out_ports[] = {
  // port ID  Name
  { DEFAULT, "Def." },
  { USB0,    "USB " },
  { UART0,   "OUT1" },
  { UART1,   "OUT2" },
  { UART2,   "OUT3" },
  { UART3,   "OUT4" },
  { IIC0,    "IIC1" },
  { IIC1,    "IIC2" },
  { IIC2,    "IIC3" },
  { IIC3,    "IIC4" },
  { 0x80,    "AOU1" },
  { 0x81,    "AOU2" },
  { 0x82,    "AOU3" },
  { 0x83,    "AOU4" },
  { 0xf0,    "Bus1" },
  { 0xf1,    "Bus2" },
  { 0xf2,    "Bus3" },
  { 0xf3,    "Bus4" }
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of available MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_InNumGet(void)
{
  return sizeof(in_ports)/sizeof(seq_midi_port_entry_t);
}

s32 SEQ_MIDI_PORT_OutNumGet(void)
{
  return sizeof(out_ports)/sizeof(seq_midi_port_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of MIDI IN/OUT port in (4 characters + zero terminator)
// port_ix is within 0..SEQ_MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
char *SEQ_MIDI_PORT_InNameGet(u8 port_ix)
{
  if( port_ix >= sizeof(in_ports)/sizeof(seq_midi_port_entry_t) )
    return "----";
  else
    return in_ports[port_ix].name;
}

char *SEQ_MIDI_PORT_OutNameGet(u8 port_ix)
{
  if( port_ix >= sizeof(out_ports)/sizeof(seq_midi_port_entry_t) )
    return "----";
  else
    return out_ports[port_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MIOS32 MIDI Port ID of a MIDI IN/OUT port
// port_ix is within 0..SEQ_MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t SEQ_MIDI_PORT_InPortGet(u8 port_ix)
{
  if( port_ix >= sizeof(in_ports)/sizeof(seq_midi_port_entry_t) )
    return 0xff; // dummy interface
  else
    return in_ports[port_ix].port;
}

mios32_midi_port_t SEQ_MIDI_PORT_OutPortGet(u8 port_ix)
{
  if( port_ix >= sizeof(out_ports)/sizeof(seq_midi_port_entry_t) )
    return 0xff; // dummy interface
  else
    return out_ports[port_ix].port;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MBSEQ MIDI Port Index of a MIOS32 MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_MIDI_PORT_InIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<sizeof(in_ports)/sizeof(seq_midi_port_entry_t); ++ix) {
    if( in_ports[ix].port == port )
      return ix;
  }
  return 0; // return first ix if not found
}

u8 SEQ_MIDI_PORT_OutIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<sizeof(out_ports)/sizeof(seq_midi_port_entry_t); ++ix) {
    if( out_ports[ix].port == port )
      return ix;
  }
  return 0; // return first ix if not found
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if MIOS32 MIDI In/Out Port is available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_InCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<sizeof(in_ports)/sizeof(seq_midi_port_entry_t); ++ix) {
    if( in_ports[ix].port == port )
      if( port >= 0x80 )
	return 1; // TODO: check for AOUT port
      else
	return MIOS32_MIDI_CheckAvailable(port);
  }
  return 0; // port not available
}

s32 SEQ_MIDI_PORT_OutCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<sizeof(out_ports)/sizeof(seq_midi_port_entry_t); ++ix) {
    if( out_ports[ix].port == port )
      if( port >= 0x80 )
	return 1; // TODO: check for AOUT port
      else
	return MIOS32_MIDI_CheckAvailable(port);
  }
  return 0; // port not available
}
