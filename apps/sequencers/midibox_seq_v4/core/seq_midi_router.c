// $Id$
/*
 * MIDI router functions of MIDIbox SEQ
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
#include <seq_midi_out.h>

#include "tasks.h"
#include "seq_midi_port.h"
#include "seq_midi_router.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_midi_router_node_t seq_midi_router_node[SEQ_MIDI_ROUTER_NUM_NODES];

u32 seq_midi_router_mclk_in;
u32 seq_midi_router_mclk_out;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_Init(u32 mode)
{
  u8 node;

  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    n->src_port = DEFAULT;
    n->src_chn  = 0; // disabled
    n->dst_port = UART0;
    n->dst_chn  = 0; // disabled
  }

  //                         USB0 only     UART0..3       IIC0..3     reserved for OSC
  seq_midi_router_mclk_in = (0x01 << 0) | (0x0f << 8) | (0x0f << 16) | (0x00 << 24);
  //                        all ports
  seq_midi_router_mclk_out = 0xffffffff;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  u8 node;

  mios32_midi_port_t def_port = MIOS32_MIDI_DefaultPortGet();

  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    if( n->src_chn && n->dst_chn &&
	(n->src_port == port || (n->src_port == DEFAULT && port == def_port)) ) {

      if( midi_package.event >= NoteOff && midi_package.event <= PitchBend ) {
	if( n->src_chn == 17 || midi_package.chn == (n->src_chn-1) ) {
	  mios32_midi_package_t fwd_package = midi_package;
	  if( n->dst_chn <= 16 )
	    fwd_package.chn = (n->dst_chn-1);
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(n->dst_port, fwd_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      } else {
	if( n->dst_chn >= 17 ) { // SysEx, MIDI Clock, etc... only forwarded if destination channel set to "All"
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
// Returns 1 if given port receives MIDI Clock
// Returns 0 if MIDI Clock In disabled
// Returns -1 if port not supported
// Returns -2 if MIDI In function disabled
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_MIDIClockInGet(mios32_midi_port_t port)
{
  // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
  if( !(port & 0x08) && port >= 0x10 && port < 0x50 ) {
    // extra: MIDI IN Clock function not supported for IIC0..7 (yet)
    if( port >= IIC0 && port <= (IIC0+15) )
      return -2; // MIDI In function disabled

    int port_flag = (((port&0xf0)-0x10) >> 1) | (port & 0x7);    
    return (seq_midi_router_mclk_in & (1 << port_flag)) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI In Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_MIDIClockInSet(mios32_midi_port_t port, u8 enable)
{
  // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
  if( !(port & 0x08) && port >= 0x10 && port < 0x50 ) {
    int port_flag = (((port&0xf0)-0x10) >> 1) | (port & 0x7);    
    if( enable )
      seq_midi_router_mclk_in |= (1 << port_flag);
    else
      seq_midi_router_mclk_in &= ~(1 << port_flag);

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
s32 SEQ_MIDI_ROUTER_MIDIClockOutGet(mios32_midi_port_t port)
{
  // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
  if( !(port & 0x08) && port >= 0x10 && port < 0x50 ) {
    int port_flag = (((port&0xf0)-0x10) >> 1) | (port & 0x7);    
    return (seq_midi_router_mclk_out & (1 << port_flag)) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI Out Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_MIDIClockOutSet(mios32_midi_port_t port, u8 enable)
{
  // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
  if( !(port & 0x08) && port >= 0x10 && port < 0x50 ) {
    int port_flag = (((port&0xf0)-0x10) >> 1) | (port & 0x7);    
    if( enable )
      seq_midi_router_mclk_out |= (1 << port_flag);
    else
      seq_midi_router_mclk_out &= ~(1 << port_flag);

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
s32 SEQ_MIDI_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick)
{
  int i;

  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = 0x5; // Single-byte system common message
  p.evnt0 = evnt0;

  u32 port_mask = 0x00000001;
  for(i=0; i<24; ++i, port_mask<<=1) { // OSC not used yet
    if( seq_midi_router_mclk_out & port_mask ) {
      // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
      mios32_midi_port_t port = (USB0 + ((i&0x18) << 1)) | (i&7);

      if( MIOS32_MIDI_CheckAvailable(port) ) {
	if( bpm_tick )
	  SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_ClkEvent, bpm_tick, 0);
	else {
	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(port, p);
	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }
  }

  return 0; // no error;
}
