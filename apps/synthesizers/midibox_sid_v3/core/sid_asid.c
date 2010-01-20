// $Id$
/*
 * ASID Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <sid.h>

#include "sid_asid.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  ASID_STATE_SYX0,
  ASID_STATE_SYX1,
  ASID_STATE_CMD,
  ASID_STATE_DATA
} asid_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SID_ASID_Data(u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static sid_asid_mode_t asid_mode;
static asid_state_t asid_state;
static mios32_midi_port_t asid_last_sysex_port;
static u8 asid_cmd;
static u8 asid_stream_ix;
static u8 asid_reg_ix;
static u8 asid_masks[4];
static u8 asid_msbs[4];

static const u8 asid_reg_map[28] = {
  0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x04, 0x0b, 0x12, 0x04, 0x0b, 0x12
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_ASID_Init(u32 mode)
{
  asid_state = ASID_STATE_SYX0;
  asid_last_sysex_port = DEFAULT;
  asid_cmd = 0;
  asid_stream_ix = 0;
  asid_reg_ix = 0;

  SID_ASID_ModeSet(SID_ASID_MODE_OFF);

  // install SysEx callback
  // MIOS32_MIDI_SysExCallback_Init(SID_ASID_Parser);
  // called from SID_SYSEX_Parser

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream
/////////////////////////////////////////////////////////////////////////////
s32 SID_ASID_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( asid_mode != SID_ASID_MODE_OFF && port != asid_last_sysex_port )
    return -1;

  asid_last_sysex_port = port;

  // clear state if status byte (like 0xf0 or 0xf7...) has been received
  if( midi_in & (1 << 7) )
    asid_state = ASID_STATE_SYX0;

  // check SysEx sequence
  switch( asid_state ) {
    case ASID_STATE_SYX0:
      if( midi_in == 0xf0 )
	asid_state = ASID_STATE_SYX1;
      break;

    case ASID_STATE_SYX1:
      if( midi_in == 0x2d )
	asid_state = ASID_STATE_CMD;
      else
	asid_state = ASID_STATE_SYX0;
      break;

    case ASID_STATE_CMD:
      asid_cmd = midi_in;

      switch( asid_cmd ) {
        case 0x4c: // Start
	  SID_ASID_ModeSet(SID_ASID_MODE_ON);
	  break;

        case 0x4d: // Stop
	  SID_ASID_ModeSet(SID_ASID_MODE_OFF);
	  break;

        case 0x4e: // Sequence
	  // ensure that SID player is properly initialized (some players don't send start command!)
	  if( asid_mode == SID_ASID_MODE_OFF )
	    SID_ASID_ModeSet(SID_ASID_MODE_ON);
	  asid_stream_ix = 0;
	  asid_reg_ix = 0;
	  break;

        case 0x4f: // LCD
	  // ensure that SID player is properly initialized (some players don't send start command!)
	  if( asid_mode == SID_ASID_MODE_OFF )
	    SID_ASID_ModeSet(SID_ASID_MODE_ON);
	  break;
      }

      asid_state = ASID_STATE_DATA;
      break;

    case ASID_STATE_DATA:
      switch( asid_cmd ) {
        case 0x4c: // Start
	  break;

        case 0x4d: // Stop
	  break;

        case 0x4e: // Sequence
	  SID_ASID_Data(midi_in);
	  break;

        case 0x4f: // LCD
	  break;
      }
      break;

    default:
      // unexpected state
      SID_ASID_TimeOut(port);
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}



/////////////////////////////////////////////////////////////////////////////
// ASID Mode Control
/////////////////////////////////////////////////////////////////////////////
s32 SID_ASID_TimeOut(mios32_midi_port_t port)
{
  // called on SysEx timeout
  if( asid_state != ASID_STATE_SYX0 && port == asid_last_sysex_port ) {
    asid_state = ASID_STATE_SYX0;
    asid_last_sysex_port = DEFAULT;
    asid_cmd = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// ASID Mode Control
/////////////////////////////////////////////////////////////////////////////
sid_asid_mode_t SID_ASID_ModeGet(void)
{
  return asid_mode;
}

s32 SID_ASID_ModeSet(sid_asid_mode_t mode)
{
  asid_mode = mode;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parses ASID Data Stream
/////////////////////////////////////////////////////////////////////////////
static s32 SID_ASID_Data(u8 midi_in)
{
  if( asid_stream_ix < 4 ) {
    asid_masks[asid_stream_ix] = midi_in;
  } else if( asid_stream_ix < 8 ) {
    asid_msbs[asid_stream_ix-4] = midi_in;
  } else {
    // scan for next mask flag
    u8 taken = 0;
    do {
      // exit if last mapped register already reached
      if( asid_reg_ix >= sizeof(asid_reg_map) )
	return -1; // all register values received

      // change register if mask flag set
      if( asid_masks[asid_reg_ix/7] & (1 << (asid_reg_ix % 7)) ) {
	u8 sid_value = midi_in;
	if( asid_msbs[asid_reg_ix/7] & (1 << (asid_reg_ix % 7)) )
	  sid_value |= (1 << 7);

	sid_regs_t *sid_l = (sid_regs_t *)&sid_regs[0];
	sid_regs_t *sid_r = (sid_regs_t *)&sid_regs[1];
	u8 sid_reg = asid_reg_map[asid_reg_ix];

	sid_l->ALL[sid_reg] = sid_value;
	sid_r->ALL[sid_reg] = sid_value;
	SID_Update(0);

	taken = 1;
      }

      // try next register
      ++asid_reg_ix;
    } while( !taken );
  }

  ++asid_stream_ix;

  return 0;
}
