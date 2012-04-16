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

#include <osc_client.h>

#include "seq_hwcfg.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_cv.h"
#include "seq_core.h"
#include "seq_blm.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// MIDI events are forwarded when sent to port 0xc0 to selectable ports
// selects USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3, AOUT
u32 seq_midi_port_multi_enable_flags;

// MIDI OUT/IN activity
u8 seq_midi_port_out_combined_ctr;
u8 seq_midi_port_in_combined_ctr;

// optional OSC packet filter
u8 filter_osc_packets;


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

// has to be kept in synch with SEQ_MIDI_PORT_InIxGet() !!!
#define NUM_IN_PORTS 17
static const seq_midi_port_entry_t in_ports[NUM_IN_PORTS] = {
  // port ID  Name
  { DEFAULT, "Def." },
  { USB0,    "USB1" },
  { USB1,    "USB2" },
  { USB2,    "USB3" },
  { USB3,    "USB4" },
  { UART0,   "IN1 " },
  { UART1,   "IN2 " },
  { UART2,   "IN3 " },
  { UART3,   "IN4 " },
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
  { 0xf0,    "Bus1" },
  { 0xf1,    "Bus2" },
  { 0xf2,    "Bus3" },
  { 0xf3,    "Bus4" }
};

// has to be kept in synch with SEQ_MIDI_PORT_OutIxGet() !!!
#define NUM_OUT_PORTS 22
static const seq_midi_port_entry_t out_ports[NUM_OUT_PORTS] = {
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
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
  { 0x80,    "AOUT" },
  { 0xf0,    "Bus1" },
  { 0xf1,    "Bus2" },
  { 0xf2,    "Bus3" },
  { 0xf3,    "Bus4" }
  // MEMO: SEQ_MIDI_PORT_OutMuteGet() has to be changed whenever ports are added/removed!
};

#define NUM_CLK_PORTS 16
static const seq_midi_port_entry_t clk_ports[NUM_CLK_PORTS] = {
  // port ID  Name
  { USB0,    "USB1" },
  { USB1,    "USB2" },
  { USB2,    "USB3" },
  { USB3,    "USB4" },
  { UART0,   "MID1" },
  { UART1,   "MID2" },
  { UART2,   "MID3" },
  { UART3,   "MID4" },
  { IIC0,    "IIC1" },
  { IIC1,    "IIC2" },
  { IIC2,    "IIC3" },
  { IIC3,    "IIC4" },
  { OSC0,    "OSC1" },
  { OSC1,    "OSC2" },
  { OSC2,    "OSC3" },
  { OSC3,    "OSC4" },
};


// MIDI Out mute function
static u32 muted_out;

// for MIDI In/Out monitor
static u8 midi_out_ctr[NUM_OUT_PORTS];
static mios32_midi_package_t midi_out_package[NUM_OUT_PORTS];

static u8 midi_in_ctr[NUM_IN_PORTS];
static mios32_midi_package_t midi_in_package[NUM_IN_PORTS];

static seq_midi_port_mon_filter_t seq_midi_port_mon_filter;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_Init(u32 mode)
{
  int i;

  // multi port (0xc0): default enables:
  //                                   USB1,      OUT1,       OSC1,      AOUT
  seq_midi_port_multi_enable_flags = (1 << 0) | (1 << 4) | (1 << 12) | (1 << 16);

  // unmute all Out ports
  muted_out = 0;

  // OSC packet filter only tmp. enabled
  filter_osc_packets = 0;

  // init MIDI In/Out monitor variables
  for(i=0; i<NUM_OUT_PORTS; ++i) {
    midi_out_ctr[i] = 0;
    midi_out_package[i].ALL = 0;
  }

  for(i=0; i<NUM_IN_PORTS; ++i) {
    midi_in_ctr[i] = 0;
    midi_in_package[i].ALL = 0;
  }

  seq_midi_port_out_combined_ctr = 0;
  seq_midi_port_in_combined_ctr = 0;

  seq_midi_port_mon_filter.ALL = 0;
  seq_midi_port_mon_filter.MIDI_CLOCK = 1;
  seq_midi_port_mon_filter.ACTIVE_SENSE = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of available MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_InNumGet(void)
{
  return NUM_IN_PORTS;
}

s32 SEQ_MIDI_PORT_OutNumGet(void)
{
  return NUM_OUT_PORTS;
}

s32 SEQ_MIDI_PORT_ClkNumGet(void)
{
  return NUM_CLK_PORTS;
}


/////////////////////////////////////////////////////////////////////////////
// Returns name of MIDI IN/OUT port in (4 characters + zero terminator)
// port_ix is within 0..SEQ_MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
char *SEQ_MIDI_PORT_InNameGet(u8 port_ix)
{
  if( port_ix >= NUM_IN_PORTS )
    return "----";
  else
    return (char *)in_ports[port_ix].name;
}

char *SEQ_MIDI_PORT_OutNameGet(u8 port_ix)
{
  if( port_ix >= NUM_OUT_PORTS )
    return "----";
  else
    return (char *)out_ports[port_ix].name;
}

char *SEQ_MIDI_PORT_ClkNameGet(u8 port_ix)
{
  if( port_ix >= NUM_CLK_PORTS )
    return "----";
  else
    return (char *)clk_ports[port_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MIOS32 MIDI Port ID of a MIDI IN/OUT port
// port_ix is within 0..SEQ_MIDI_PORT_In/OutNumGet()-1
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t SEQ_MIDI_PORT_InPortGet(u8 port_ix)
{
  if( port_ix >= NUM_IN_PORTS )
    return 0xff; // dummy interface
  else
    return in_ports[port_ix].port;
}

mios32_midi_port_t SEQ_MIDI_PORT_OutPortGet(u8 port_ix)
{
  if( port_ix >= NUM_OUT_PORTS )
    return 0xff; // dummy interface
  else
    return out_ports[port_ix].port;
}

mios32_midi_port_t SEQ_MIDI_PORT_ClkPortGet(u8 port_ix)
{
  if( port_ix >= NUM_CLK_PORTS )
    return 0xff; // dummy interface
  else
    return clk_ports[port_ix].port;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the MBSEQ MIDI Port Index of a MIOS32 MIDI IN/OUT port
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_MIDI_PORT_InIxGet(mios32_midi_port_t port)
{
#if 0
  u8 ix;
  for(ix=0; ix<NUM_IN_PORTS; ++ix) {
    if( in_ports[ix].port == port )
      return ix;
  }
#else
  // faster version - execution time does matter for some features!
  if( (port & 0x0f) >= 4 )
    return 0; // only 1..4, filter number 5..16

  switch( port & 0xf0 ) {
    // has to be kept in sync with in_ports[]!
    case DEFAULT: return 0;
    case USB0:  return (port & 0x0f) + 1;
    case UART0: return (port & 0x0f) + 5;
    case OSC0:  return (port & 0x0f) + 9;
    case 0xf0:  return (port & 0x0f) + 13; // Bus
  }
#endif

  return 0; // return first ix if not found
}

u8 SEQ_MIDI_PORT_OutIxGet(mios32_midi_port_t port)
{
#if 0
  u8 ix;
  for(ix=0; ix<NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port )
      return ix;
  }
#else
  // faster version - execution time does matter for some features!
  if( (port & 0x0f) >= 4 )
    return 0; // only 1..4, filter number 5..16

  switch( port & 0xf0 ) {
    // has to be kept in sync with out_ports[]!
    case DEFAULT: return 0;
    case USB0:  return (port & 0x0f) + 1;
    case UART0: return (port & 0x0f) + 5;
    case IIC0:  return (port & 0x0f) + 9;
    case OSC0:  return (port & 0x0f) + 13;
    case 0x80:  return (port & 0x0f) + 17; // AOUT
    case 0xf0:  return (port & 0x0f) + 18; // Bus
  }
#endif

  return 0; // return first ix if not found
}

u8 SEQ_MIDI_PORT_ClkIxGet(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port )
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
  for(ix=0; ix<NUM_IN_PORTS; ++ix) {
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

s32 SEQ_MIDI_PORT_OutCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<NUM_OUT_PORTS; ++ix) {
    if( out_ports[ix].port == port ) {
      if( port == 0x80 ) {
	return SEQ_CV_IfGet() ? 1 : 0;
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

s32 SEQ_MIDI_PORT_ClkCheckAvailable(mios32_midi_port_t port)
{
  u8 ix;
  for(ix=0; ix<NUM_CLK_PORTS; ++ix) {
    if( clk_ports[ix].port == port ) {
      // only up to 80 ports (0x00..0x4f = USB, UART, IIC, OSC) supported
      if( port < 0x40 )
	return MIOS32_MIDI_CheckAvailable(port);
      else if( (port & 0xf0) == OSC0 )
	return 1; // TODO: check for ethernet connection here
    }
  }
  return 0; // port not available
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if MIOS32 MIDI Out Port is muted
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_OutMuteGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;

  if( port != DEFAULT ) {
    port_ix = SEQ_MIDI_PORT_OutIxGet(port);

    if( !port_ix )
      return 0; // port not supported... and not muted (therefore 0 instead of -1)
  }

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
// Returns the last sent MIDI package of given output port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t SEQ_MIDI_PORT_OutPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = SEQ_MIDI_PORT_OutIxGet(port);

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
mios32_midi_package_t SEQ_MIDI_PORT_InPackageGet(mios32_midi_port_t port)
{
  u8 port_ix = 0;
  mios32_midi_package_t empty;
  empty.ALL = 0;

  if( port != DEFAULT ) {
    port_ix = SEQ_MIDI_PORT_InIxGet(port);

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
s32 SEQ_MIDI_PORT_MonFilterSet(seq_midi_port_mon_filter_t filter)
{
  seq_midi_port_mon_filter = filter;
  return 0; // no error
}

seq_midi_port_mon_filter_t SEQ_MIDI_PORT_MonFilterGet(void)
{
  return seq_midi_port_mon_filter;
}


/////////////////////////////////////////////////////////////////////////////
// Handles the MIDI In/Out counters
// Should be called periodically each mS
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_Period1mS(void)
{
  int i;
  static u8 predivider = 0;

  // counters are handled each 10 mS
  if( ++predivider >= 10 ) {
    predivider = 0;

    for(i=0; i<NUM_OUT_PORTS; ++i)
      if( midi_out_ctr[i] )
	--midi_out_ctr[i];

    for(i=0; i<NUM_IN_PORTS; ++i)
      if( midi_in_ctr[i] )
	--midi_in_ctr[i];

    if( seq_midi_port_out_combined_ctr )
      --seq_midi_port_out_combined_ctr;

    if( seq_midi_port_in_combined_ctr )
      --seq_midi_port_in_combined_ctr;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// OSC packets are tmp. not sent if this flag is set
// Can be used to avoid feedback loops or stack overflows (e.g. used from seq_live.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_FilterOscPacketsSet(u8 filter)
{
  filter_osc_packets = filter;
  return 0;
}

s32 SEQ_MIDI_PORT_FilterOscPacketsGet(void)
{
  return filter_osc_packets;
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Tx Notificaton hook if a MIDI event should be sent
// Allows to provide additional MIDI ports
// If 1 is returned, package will be filtered!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_NotifyMIDITx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI Out monitor function
  u8 mon_filtered = 0;
  if( seq_midi_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( seq_midi_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = SEQ_MIDI_PORT_OutIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_out_package[port_ix] = package;
      midi_out_ctr[port_ix] = 20; // 2 seconds lifetime
      seq_midi_port_out_combined_ctr = 5; // 500 mS lifetime
    }
  }


  // DIN Sync Event (0xf9 sent over port 0xff)
  if( port == 0xff && package.evnt0 == 0xf9 ) {
    seq_core_din_sync_pulse_ctr = 2 + SEQ_CV_ClkPulseWidthGet(); // to generate a pulse with configurable length (+1 for 1->0 transition, +1 to compensate jitter)
    return 1; // filter package
  }

  if( (port & 0xf0) == OSC0 ) { // OSC1..4 port
    // avoid OSC feedback in seq_live.c (can cause infinite loops or stack overflows)
    if( filter_osc_packets || OSC_CLIENT_SendMIDIEvent(port & 0xf, package) >= 0 )
      return 1; // filter package
  } else if( port == 0x80 ) { // AOUT port
    if( SEQ_CV_SendPackage(port & 0xf, package) )
      return 1; // filter package
  } else if( port == 0xc0 ) { // Multi OUT port
    int i;
    u32 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( seq_midi_port_multi_enable_flags & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
	if( port != seq_blm_port ) // ensure that no note will be sent to BLM port if enabled
	  MIOS32_MIDI_SendPackage(port, package);
      }
    }

    if( seq_midi_port_multi_enable_flags & (1 << 16) ) {
      MIOS32_MIDI_SendPackage(0x80, package); // AOUT
    }

    return 1; // filter forwarding
  }

  return 0; // don't filter package
}


/////////////////////////////////////////////////////////////////////////////
// Called by MIDI Rx Notificaton hook in app.c if a MIDI event is received
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_PORT_NotifyMIDIRx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // MIDI In monitor function
  u8 mon_filtered = 0;
  if( seq_midi_port_mon_filter.MIDI_CLOCK && package.evnt0 == 0xf8 )
    mon_filtered = 1;
  else if( seq_midi_port_mon_filter.ACTIVE_SENSE && package.evnt0 == 0xfe )
    mon_filtered = 1;

  if( !mon_filtered ) {
    int port_ix = -1;
    if( port != DEFAULT ) {
      port_ix = SEQ_MIDI_PORT_InIxGet(port);

      if( !port_ix )
	port_ix = -1; // port not mapped
    }

    if( port_ix >= 0 ) {
      midi_in_package[port_ix] = package;
      midi_in_ctr[port_ix] = 20; // 2 seconds lifetime
      seq_midi_port_in_combined_ctr = 5; // 500 mS lifetime
    }
  }

  return 0; // no error
}
