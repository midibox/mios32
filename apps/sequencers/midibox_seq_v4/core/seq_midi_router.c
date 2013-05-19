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
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_ui.h"


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

  //                         USB0 only     UART0..3       IIC0..3      OSC0..3
  seq_midi_router_mclk_in = (0x01 << 0) | (0x0f << 8) | (0x0f << 16) | (0x01 << 24);
  //                        all ports
  seq_midi_router_mclk_out = 0xffffffff;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns 32bit selection mask for USB0..7, UART0..7, IIC0..7, OSC0..7
/////////////////////////////////////////////////////////////////////////////
static inline u32 SEQ_MIDI_ROUTER_PortMaskGet(mios32_midi_port_t port)
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
s32 SEQ_MIDI_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // filter SysEx which is handled by separate parser
  if( midi_package.evnt0 < 0xf8 &&
      (midi_package.cin == 0xf ||
      (midi_package.cin >= 0x4 && midi_package.cin <= 0x7)) )
    return 0; // no error

  u32 sysex_dst_fwd_done = 0;
  u8 node;
  mios32_midi_port_t def_port = MIOS32_MIDI_DefaultPortGet();

  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    if( n->src_chn && n->dst_chn &&
	(n->src_port == port || (n->src_port == DEFAULT && port == def_port)) ) {

      // forwarding OSC to OSC will very likely result into a stack overflow (or feedback loop) -> avoid this!
      if( ((port ^ n->dst_port) & 0xf0) == OSC0 )
	continue;

      if( midi_package.event >= NoteOff && midi_package.event <= PitchBend ) {
	if( n->src_chn == 17 || midi_package.chn == (n->src_chn-1) ) {
	  mios32_midi_port_t fwd_port = n->dst_port;
	  mios32_midi_package_t fwd_package = midi_package;
	  if( n->dst_chn <= 16 ) {
	    fwd_package.chn = (n->dst_chn-1);
	  } else if( n->dst_chn == 18 ) { // Trk
	    fwd_port = SEQ_CC_Get(midi_package.chn, SEQ_CC_MIDI_PORT);
	    fwd_package.chn = SEQ_CC_Get(midi_package.chn, SEQ_CC_MIDI_CHANNEL);
	  } else if( n->dst_chn >= 19 ) { // SelTrk
	    u8 visible_track = SEQ_UI_VisibleTrackGet();
	    fwd_port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
	    fwd_package.chn = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL);
	  }

	  MUTEX_MIDIOUT_TAKE;
	  MIOS32_MIDI_SendPackage(fwd_port, fwd_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      } else {
	// Realtime events: ensure that they are only forwarded once
	u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(n->dst_port);
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
// Receives a SysEx byte from SEQ_MIDI_SYSEX_Parser (-> seq_midi_sysex.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  u8 node;

  mios32_midi_port_t def_port = MIOS32_MIDI_DefaultPortGet();

  u32 sysex_dst_fwd_done = 0;
  seq_midi_router_node_t *n = &seq_midi_router_node[0];
  for(node=0; node<SEQ_MIDI_ROUTER_NUM_NODES; ++node, ++n) {
    if( n->src_chn && n->dst_chn&&
	(n->src_port == port || (n->src_port == DEFAULT && port == def_port)) ) {

      // SysEx, only forwarded once per destination port
      u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(n->dst_port);
      if( !mask || !(sysex_dst_fwd_done & mask) ) {
	sysex_dst_fwd_done |= mask;

	// forward as single byte
	// TODO: optimize this by collecting up to 3 data words and put it into package
	MUTEX_MIDIOUT_TAKE;
	mios32_midi_package_t midi_package;
	midi_package.ALL = 0x00000000;
	midi_package.type = 0xf; // single byte
	midi_package.evnt0 = midi_in;
	MIOS32_MIDI_SendPackage(n->dst_port, midi_package);
	MUTEX_MIDIOUT_GIVE;
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
  // extra: MIDI IN Clock function not supported for IIC (yet)
  if( (port & 0xf0) == IIC0 )
    return -2; // MIDI In function disabled

  u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    return (seq_midi_router_mclk_in & mask) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI In Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_MIDIClockInSet(mios32_midi_port_t port, u8 enable)
{
  u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    if( enable )
      seq_midi_router_mclk_in |= mask;
    else
      seq_midi_router_mclk_in &= ~mask;

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
  u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    return (seq_midi_router_mclk_out & mask) ? 1 : 0;
  }

  return -1; // port not supported
}

/////////////////////////////////////////////////////////////////////////////
// Enables/Disables MIDI Out Clock function for given port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_ROUTER_MIDIClockOutSet(mios32_midi_port_t port, u8 enable)
{
  u32 mask = SEQ_MIDI_ROUTER_PortMaskGet(port);
  if( mask ) {
    if( enable )
      seq_midi_router_mclk_out |= mask;
    else
      seq_midi_router_mclk_out &= ~mask;

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
  for(i=0; i<32; ++i, port_mask<<=1) {
    if( seq_midi_router_mclk_out & port_mask & 0xffffff0f ) { // filter USB5..USB8 to avoid unwanted clock events to non-existent ports
      // coding: USB0..7, UART0..7, IIC0..7, OSC0..7
      mios32_midi_port_t port = (USB0 + ((i&0x18) << 1)) | (i&7);

      // TODO: special check for OSC, since MIOS32_MIDI_CheckAvailable() won't work here
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
