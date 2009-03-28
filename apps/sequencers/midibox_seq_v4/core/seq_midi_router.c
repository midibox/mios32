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

#include "seq_midi_port.h"
#include "seq_midi_router.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_midi_router_node_t seq_midi_router_node[SEQ_MIDI_ROUTER_NUM_NODES];

u8 seq_midi_router_mclk_out;


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

  seq_midi_router_mclk_out = 0xff;

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
	  MIOS32_MIDI_SendPackage(n->dst_port, fwd_package);
	}
      } else {
	if( n->dst_chn >= 17 ) // SysEx, MIDI Clock, etc... only forwarded if destination channel set to "All"
	  MIOS32_MIDI_SendPackage(n->dst_port, midi_package);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a MIDI clock/Start/Stop/Continue event to all output
// ports which have been enabled for this function.
// if bpm_tick == 0, the event will be sent immediately, otherwise it will
// be queued
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick)
{
  // currently only a simple on/off switch provided
  // we send to a couple of MIDI ports which are defined here:
  const mios32_midi_port_t mclk_ports[] = { USB0, USB1, USB2, USB3, UART0, UART1, IIC0, IIC1, IIC2, IIC3 };

  // only send in master mode (in slave mode, the router can be used to forward MIDI clock events)
  if( seq_midi_router_mclk_out && SEQ_BPM_IsMaster() ) {
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = 0x5; // Single-byte system common message
    p.evnt0 = evnt0;

    u8 num_ports = sizeof(mclk_ports)/sizeof(mios32_midi_port_t);
    u8 i;
    for(i=0; i<num_ports; ++i) {
      mios32_midi_port_t port = mclk_ports[i];

      if( MIOS32_MIDI_CheckAvailable(port) ) {
	if( bpm_tick )
	  SEQ_MIDI_OUT_Send(port, p, SEQ_MIDI_OUT_ClkEvent, bpm_tick, 0);
	else
	  MIOS32_MIDI_SendPackage(port, p);
      }
    }
  }

  return 0; // no error;
}
