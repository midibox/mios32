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

#include <aout.h>

#include "seq_hwcfg.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"


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
  { USB0,    "USB1" },
  { USB1,    "USB2" },
  { USB2,    "USB3" },
  { USB3,    "USB4" },
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
  { USB0,    "USB1" },
  { USB1,    "USB2" },
  { USB2,    "USB3" },
  { USB3,    "USB4" },
  { UART0,   "OUT1" },
  { UART1,   "OUT2" },
  { UART2,   "OUT3" },
  { UART3,   "OUT4" },
  { IIC0,    "IIC1" },
  { IIC1,    "IIC2" },
  { IIC2,    "IIC3" },
  { IIC3,    "IIC4" },
  { 0x80,    "AOUT" },
  { 0xf0,    "Bus1" },
  { 0xf1,    "Bus2" },
  { 0xf2,    "Bus3" },
  { 0xf3,    "Bus4" }
  // MEMO: SEQ_MIDI_PORT_OutMuteGet() has to be changed whenever ports are added/removed!
};


static u32 muted_out;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_Init(u32 mode)
{
  // unmute all Out ports
  muted_out = 0;

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
      if( port >= 0xf0 )
	return 1; // Bus is always available
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
      if( port == 0x80 ) {
	// check if AOUT configured
	aout_config_t config;
	config = AOUT_ConfigGet();
	return config.if_type ? 1 : 0;
      } else if ( port >= 0xf0 ) {
	return 1; // Bus is always available
      } else
	return MIOS32_MIDI_CheckAvailable(port);
  }
  return 0; // port not available
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if MIOS32 MIDI Out Port is muted
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_OutMuteGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;

#if 0
  if( port != DEFAULT )
    port_ix = SEQ_MIDI_PORT_OutIxGet(port);

    if( !port_ix )
      return 0; // port not supported... and not muted (therefore 0 instead of -1)
  }
#else
  // faster version - execution time does matter, as this function is frequently called
  // from SEQ_CORE_Tick()
  switch( port & 0xf0 ) {
    // has to be kept in sync with out_ports[]!
    case DEFAULT: port_ix = 0; break;
    case USB0:  port_ix = (port & 0x0f) + 1; break;
    case UART0: port_ix = (port & 0x0f) + 5; break;
    case IIC0:  port_ix = (port & 0x0f) + 9; break;
    case 0x80:  port_ix = (port & 0x0f) + 13; break; // AOUT
    case 0xf0:  port_ix = (port & 0x0f) + 14; break; // Bus
  }
#endif

  return (muted_out & (1 << port_ix)) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// Mutes/Unmutes a MIDI port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_OutMuteSet(mios32_midi_port_t port, u8 mute)
{
  u8 port_ix;

  if( port == DEFAULT )
    port_ix = 0;
  else {
    port_ix = SEQ_MIDI_PORT_OutIxGet(port);

    if( !port_ix )
      return -1; // port not supported
  }

  if( mute )
    muted_out |= (1 << port_ix);
  else
    muted_out &= ~(1 << port_ix);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Tx Notificaton hook if a MIDI event should be sent
// Allows to provide additional MIDI ports
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // TODO: Add also Bus handlers here

  if( port == 0x80 ) { // AOUT port
    // Note Off -> Note On with velocity 0
    if( package.event == NoteOff ) {
      package.event = NoteOn;
      package.velocity = 0;
    }

    if( package.event == NoteOn ) {
      // if channel 1..8: set only note value on CV OUT #1..8, triggers Gate #1..8
      // if channel 9..12: set note/velocity on each pin pair (CV OUT #1/2, #3/4, #5/6, #7/8), triggers Gate #1+2, #3+4, #5+6, #7+8
      // if channel 13..15: set velocity/note on each pin pair (CV OUT #1/2, #3/4, #5/6), triggers Gate #1+2, #3+4, #5+6
      // if channel 16: trigger the extension pins (DOUT)

      if( package.chn == Chn16 ) {
	int gate_pin = package.note - 0x24; // C-1 is the base note
	if( gate_pin >= 0 ) {
	  u8 dout_sr = gate_pin / 8;
	  u8 dout_pin = gate_pin % 8;

	  if( dout_sr < SEQ_HWCFG_NUM_SR_DOUT_GATES && seq_hwcfg_dout_gate_sr[dout_sr] )
	    MIOS32_DOUT_PinSet((seq_hwcfg_dout_gate_sr[dout_sr]-1)*8 + dout_pin, package.velocity ? 1 : 0);
	}
      } else {
	int aout_chn_note, aout_chn_vel, gate_pin;

	if( package.chn <= Chn8 ) {
	  aout_chn_note = package.chn;
	  aout_chn_vel = -1;
	  gate_pin = package.chn;
	} else if( package.chn <= Chn12 ) {
	  aout_chn_note = ((package.chn & 3) << 1);
	  aout_chn_vel = aout_chn_note + 1;
	  gate_pin = package.chn & 3;
	} else { // Chn <= 15
	  aout_chn_vel = ((package.chn & 3) << 1);
	  aout_chn_note = aout_chn_vel + 1;
	  gate_pin = package.chn & 3;
	}

	// update Note/Velocity CV if velocity > 0
	if( package.velocity ) {
	  if( aout_chn_note >= 0 )
	    AOUT_PinSet(aout_chn_note, package.note << 9);

	  if( aout_chn_vel >= 0 )
	    AOUT_PinSet(aout_chn_vel, package.velocity << 9);
	}

	// set gate pin
	if( gate_pin >= 0 ) {
	  AOUT_DigitalPinSet(gate_pin, package.velocity ? 1 : 0);
#if 0
	  // not here - all pins are updated at once after AOUT_Update() (see SEQ_TASK_MIDI() in app.c)
	  MIOS32_BOARD_J5_PinSet(gate_pin, package.velocity ? 1 : 0);
#endif
	}
      }
    } else if( package.event == CC ) {
      // if channel 1..8: sets CV Out #1..8 depending on CC number (16 = #1, 17 = #2, ...) - channel has no effect
      // if channel 9..16: sets CV Out #1..8 depending on channel, always sets Gate #1..8

      int aout_chn, gate_pin;
      if( package.chn <= Chn8 ) {
	aout_chn = package.cc_number - 16;
	gate_pin = aout_chn;
      } else {
	aout_chn = package.chn & 0x7;
	gate_pin = aout_chn;
      }

      // aout_chn could be >= 8, but this doesn't matter... AOUT_PinSet() checks for number of available channels
      // this could be useful for future extensions (e.g. higher number of AOUT Channels)
      if( aout_chn >= 0 )
	AOUT_PinSet(aout_chn, package.value << 9);

      // Gate is always set (useful for controlling pitch of an analog synth where gate is connected as well)
      AOUT_DigitalPinSet(gate_pin, 1);
#if 0
      // not here - all pins are updated at once after AOUT_Update() (see SEQ_TASK_MIDI() in app.c)
      MIOS32_BOARD_J5_PinSet(gate_pin, 1);
#endif
    }
  }

  return 0; // no error
}
