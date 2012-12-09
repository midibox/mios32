// $Id$
/*
 * SysEx Parser for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "mbng_patch.h"
#include "mbng_sysex.h"

/////////////////////////////////////////////////////////////////////////////
// local definitions
/////////////////////////////////////////////////////////////////////////////


// command states
#define MBNG_SYSEX_CMD_STATE_BEGIN 0
#define MBNG_SYSEX_CMD_STATE_CONT  1
#define MBNG_SYSEX_CMD_STATE_END   2

// ack/disack code
#define MBNG_SYSEX_DISACK   0x0e
#define MBNG_SYSEX_ACK      0x0f

// disacknowledge arguments
#define MBNG_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define MBNG_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define MBNG_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define MBNG_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define MBNG_SYSEX_DISACK_INVALID_COMMAND      0x0c


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };

  struct {
    unsigned CTR:3;
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned CMD:1;
    unsigned MY_SYSEX:1;
  };

} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MBNG_SYSEX_CmdFinished(void);
static s32 MBNG_SYSEX_SendFooter(u8 force);
static s32 MBNG_SYSEX_Cmd(u8 cmd_state, u8 midi_in);

static s32 MBNG_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_cmd;

static mios32_midi_port_t sysex_port = DEFAULT;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

// SysEx header of MBNG
static const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x50 };


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // install SysEx parser
  //MIOS32_MIDI_SysExCallback_Init(MBNG_SYSEX_Parser);
  // parser called from APP_SYSEX_Parser

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  int i;
  u8 sysex_buffer[20];
  int sysex_buffer_ix = 0;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // send ack code and argument
  sysex_buffer[sysex_buffer_ix++] = ack_code;
  sysex_buffer[sysex_buffer_ix++] = ack_arg;

  // send footer
  sysex_buffer[sysex_buffer_ix++] = 0xf7;

  // finally send SysEx stream and return error status
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, sysex_buffer_ix);
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
// MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX && port == sysex_port )
    MBNG_SYSEX_CmdFinished();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != sysex_port )
    return 1; // don't forward package to APP_MIDI_NotifyPackage()

  sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( midi_in != sysex_header[sysex_state.CTR] ) {
      // incoming byte doesn't match
      MBNG_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR == sizeof(sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;

	// disable merger forwarding until end of sysex message
	// TODO
	//	MIOS_MPROC_MergerDisable();
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	MBNG_SYSEX_Cmd(MBNG_SYSEX_CMD_STATE_END, midi_in);
      }
      MBNG_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	MBNG_SYSEX_Cmd(MBNG_SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	MBNG_SYSEX_Cmd(MBNG_SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  // enable MIDI forwarding again
  // TODO
  //  MIOS_MPROC_MergerEnable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends the SysEx footer if merger enabled
// if force == 1, send the footer regardless of merger state
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_SendFooter(u8 force)
{
#if 0
  // TODO ("force" not used yet, merger not available yet)
  if( force || (MIOS_MIDI_MergerGet() & 0x01) )
    MIOS_MIDI_TxBufferPut(0xf7);
#endif

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x0f:
      MBNG_SYSEX_Cmd_Ping(cmd_state, midi_in);
      break;
    default:
      // unknown command
      MBNG_SYSEX_SendFooter(0);
      MBNG_SYSEX_SendAck(sysex_port, MBNG_SYSEX_DISACK, MBNG_SYSEX_DISACK_INVALID_COMMAND);
      MBNG_SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MBNG_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MBNG_SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // MBNG_SYSEX_CMD_STATE_END
      MBNG_SYSEX_SendFooter(0);

      // send acknowledge
      MBNG_SYSEX_SendAck(sysex_port, MBNG_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}


