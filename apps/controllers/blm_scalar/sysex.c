// $Id$
/*
 * SysEx Parser for BLM_SCALAR
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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
#include "sysex.h"


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SYSEX_CmdFinished(void);
static s32 SYSEX_SendFooter(u8 force);
static s32 SYSEX_Cmd(u8 cmd_state, u8 midi_in);

static s32 SYSEX_Cmd_InfoRequest(u8 cmd_state, u8 midi_in);
static s32 SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_cmd;

static mios32_midi_port_t sysex_port = DEFAULT;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

static const unsigned char sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4e };


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // install SysEx parser
  MIOS32_MIDI_SysExCallback_Init(SYSEX_Parser);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump with layout informations
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_SendLayoutInfo(mios32_midi_port_t port)
{
  u8 sysex_buffer[20];
  int sysex_buffer_ix = 0;

  // send header
  int i;
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // send device ID
  sysex_buffer[sysex_buffer_ix++] = MIOS32_MIDI_DeviceIDGet();

  // Command #1 (Layout Info)
  sysex_buffer[sysex_buffer_ix++] = 0x01;

  // Number of columns
  sysex_buffer[sysex_buffer_ix++] = 16; // TODO: should be configurable

  // Number of rows
  sysex_buffer[sysex_buffer_ix++] = 16; // TODO: should be configurable

  // number of LED colours
  sysex_buffer[sysex_buffer_ix++] = 2; // TODO: should be configurable

  // number of extra rows
  sysex_buffer[sysex_buffer_ix++] = 1;

  // number of extra columns
  sysex_buffer[sysex_buffer_ix++] = 1;

  // number of extra buttons (e.g. shift)
  sysex_buffer[sysex_buffer_ix++] = 1;

  // footer
  sysex_buffer[sysex_buffer_ix++] = 0xf7;

  // finally send SysEx stream and return error status
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, sysex_buffer_ix);
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[20];
  int sysex_buffer_ix = 0;

  // send header
  int i;
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // send device ID
  sysex_buffer[sysex_buffer_ix++] = MIOS32_MIDI_DeviceIDGet();

  // send ack code and argument
  sysex_buffer[sysex_buffer_ix++] = ack_code;
  sysex_buffer[sysex_buffer_ix++] = ack_arg;

  // send footer
  sysex_buffer[sysex_buffer_ix++] = 0xf7;

  // finally send SysEx stream and return error status
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, sysex_buffer_ix);
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != sysex_port )
    return 1; // don't forward package to APP_MIDI_NotifyPackage()

  sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(sysex_header) && midi_in != sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(sysex_header) && midi_in != MIOS32_MIDI_DeviceIDGet()) ) {
      // incoming byte doesn't match
      SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(sysex_header) ) {
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
      	SYSEX_Cmd(SYSEX_CMD_STATE_END, midi_in);
      }
      SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	SYSEX_Cmd(SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	SYSEX_Cmd(SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_CmdFinished(void)
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
s32 SYSEX_SendFooter(u8 force)
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
s32 SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x00:
      SYSEX_Cmd_InfoRequest(cmd_state, midi_in);
      break;
    case 0x01: // Layout Info
    case 0x0e: // error
      // ignore to avoid feedback loops
      break;
    case 0x0f:
      SYSEX_Cmd_Ping(cmd_state, midi_in);
      break;
    default:
      // unknown command
      SYSEX_SendFooter(0);
      SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_INVALID_COMMAND);
      SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 00: Layout Info Request
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Cmd_InfoRequest(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // SYSEX_CMD_STATE_END
      SYSEX_SendFooter(0);

      // send layout info
      SYSEX_SendLayoutInfo(sysex_port);
      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // SYSEX_CMD_STATE_END
      SYSEX_SendFooter(0);

      // send acknowledge
      SYSEX_SendAck(sysex_port, SYSEX_ACK, 0x00);
      break;
  }

  return 0; // no error
}


