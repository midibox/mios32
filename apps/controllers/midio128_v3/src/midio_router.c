// $Id$
/*
 * MIDI Router functions for MIDIO128 V3
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
#include <osc_client.h>
#include "app.h"
#include "tasks.h"

#include "midio_router.h"
#include "midio_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local defines
/////////////////////////////////////////////////////////////////////////////

// SysEx buffer for each input
#define NUM_SYSEX_BUFFERS     8
#define SYSEX_BUFFER_IN_USB0  0
#define SYSEX_BUFFER_IN_USB1  1
#define SYSEX_BUFFER_IN_UART0 2
#define SYSEX_BUFFER_IN_UART1 3
#define SYSEX_BUFFER_IN_OSC0  4
#define SYSEX_BUFFER_IN_OSC1  5
#define SYSEX_BUFFER_IN_OSC2  6
#define SYSEX_BUFFER_IN_OSC3  7

#define SYSEX_BUFFER_SIZE 1024

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 sysex_buffer[NUM_SYSEX_BUFFERS][SYSEX_BUFFER_SIZE];
static u32 sysex_buffer_len[NUM_SYSEX_BUFFERS];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the MIDI router
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_ROUTER_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // clear SysEx buffers
  int i;
  for(i=0; i<NUM_SYSEX_BUFFERS; ++i)
    sysex_buffer_len[i] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // SysEx handled by APP_SYSEX_Parser()
  if( midi_package.type >= 4 && midi_package.type <= 7 )
    return 0; // no error

  int node;
  midio_patch_router_entry_t *n = (midio_patch_router_entry_t *)&midio_patch_router[0];
  for(node=0; node<MIDIO_PATCH_NUM_ROUTER; ++node, ++n) {
    if( n->src_chn && n->dst_chn && (n->src_port == port) ) {
      if( midi_package.event >= NoteOff && midi_package.event <= PitchBend ) {
	if( n->src_chn == 17 || midi_package.chn == (n->src_chn-1) ) {
	  mios32_midi_package_t fwd_package = midi_package;
	  if( n->dst_chn <= 16 )
	    fwd_package.chn = (n->dst_chn-1);
	  mios32_midi_port_t port = n->dst_port;
	  MUTEX_MIDIOUT_TAKE;
	  if( (port & 0xf0) == OSC0 )
	    OSC_CLIENT_SendMIDIEvent(port & 0x0f, fwd_package);
	  else
	    MIOS32_MIDI_SendPackage(port, fwd_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      } else {
	if( n->dst_chn >= 17 ) { // SysEx, MIDI Clock, etc... only forwarded if destination channel set to "All"
	  mios32_midi_port_t port = n->dst_port;
	  MUTEX_MIDIOUT_TAKE;
	  if( (port & 0xf0) == OSC0 )
	    OSC_CLIENT_SendMIDIEvent(port & 0x0f, midi_package);
	  else
	    MIOS32_MIDI_SendPackage(port, midi_package);
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
s32 MIDIO_ROUTER_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  // determine SysEx buffer
  int sysex_in = 0;

  switch( port ) {
  case USB0: sysex_in = SYSEX_BUFFER_IN_USB0; break;
  case USB1: sysex_in = SYSEX_BUFFER_IN_USB1; break;
  case UART0: sysex_in = SYSEX_BUFFER_IN_UART0; break;
  case UART1: sysex_in = SYSEX_BUFFER_IN_UART1; break;
  case OSC0: sysex_in = SYSEX_BUFFER_IN_OSC0; break;
  case OSC1: sysex_in = SYSEX_BUFFER_IN_OSC1; break;
  case OSC2: sysex_in = SYSEX_BUFFER_IN_OSC2; break;
  case OSC3: sysex_in = SYSEX_BUFFER_IN_OSC3; break;
  default:
    return -1; // not assigned
  }

  // store value into buffer, send when:
  //   o 0xf7 (end of stream) has been received
  //   o 0xf0 (start of stream) has been received although buffer isn't empty
  //   o buffer size has been exceeded
  // we check for (SYSEX_BUFFER_SIZE-1), so that we always have a free byte for F7
  u32 buffer_len = sysex_buffer_len[sysex_in];
  if( midi_in == 0xf7 || (midi_in == 0xf0 && buffer_len != 0) || buffer_len >= (SYSEX_BUFFER_SIZE-1) ) {

    if( midi_in == 0xf7 && buffer_len < SYSEX_BUFFER_SIZE ) // note: we always have a free byte for F7
      sysex_buffer[sysex_in][sysex_buffer_len[sysex_in]++] = midi_in;

    int node;
    midio_patch_router_entry_t *n = (midio_patch_router_entry_t *)&midio_patch_router[0];
    for(node=0; node<MIDIO_PATCH_NUM_ROUTER; ++node, ++n) {
      // SysEx, only forwarded if source and destination channel set to "All"
      if( n->src_chn >= 17 && n->dst_chn >= 17 && (n->src_port == port) ) {
	mios32_midi_port_t port = n->dst_port;
	MUTEX_MIDIOUT_TAKE;
	if( (port & 0xf0) == OSC0 )
	  OSC_CLIENT_SendSysEx(port & 0x0f, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
	else
	  MIOS32_MIDI_SendSysEx(port, sysex_buffer[sysex_in], sysex_buffer_len[sysex_in]);
	MUTEX_MIDIOUT_GIVE;
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
