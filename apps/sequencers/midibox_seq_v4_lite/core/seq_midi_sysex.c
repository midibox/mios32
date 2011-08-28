// $Id$
/*
 * MIDI SysEx Routines
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
#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define SYSEX_DISACK_INVALID_COMMAND      0x0c


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  SYSEX_CMD_STATE_BEGIN,
  SYSEX_CMD_STATE_CONT,
  SYSEX_CMD_STATE_END
} sysex_cmd_state_t;

typedef union {
  struct {
    unsigned ALL:32;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned PING_BYTE_RECEIVED:1;
  };

} sysex_state_t;



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_SYSEX_CmdFinished(void);
static s32 SEQ_MIDI_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 seq_midi_sysex_header[5] =    { 0xf0, 0x00, 0x00, 0x7e, 0x4d };

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;
static mios32_midi_port_t last_sysex_port = DEFAULT;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_Init(u32 mode)
{
  last_sysex_port = DEFAULT;
  sysex_state.ALL = 0;
  sysex_device_id = MIOS32_MIDI_DeviceIDGet(); // taken from MIOS32

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  // TODO: support for independent streams of MBSEQ Remote and remaining stuff
  if( sysex_state.MY_SYSEX && port != last_sysex_port )
    return -1;

  last_sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(seq_midi_sysex_header) && midi_in != seq_midi_sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(seq_midi_sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      SEQ_MIDI_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(seq_midi_sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	SEQ_MIDI_SYSEX_Cmd(port, SYSEX_CMD_STATE_END, midi_in);
      }
      SEQ_MIDI_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	SEQ_MIDI_SYSEX_Cmd(port, SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	SEQ_MIDI_SYSEX_Cmd(port, SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case 0x0e: // ignore to avoid loopbacks
      break;

    case 0x0f:
      SEQ_MIDI_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      SEQ_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND);
      SEQ_MIDI_SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      sysex_state.PING_BYTE_RECEIVED = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      sysex_state.PING_BYTE_RECEIVED = 1;
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // send acknowledge if no additional byte has been received
      // to avoid feedback loop if two cores are directly connected
      if( !sysex_state.PING_BYTE_RECEIVED )
	SEQ_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(seq_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  MUTEX_MIDIOUT_GIVE;
  return status;
}

/////////////////////////////////////////////////////////////////////////////
// This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
// MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX && port == last_sysex_port )
    SEQ_MIDI_SYSEX_CmdFinished();

  return 0; // no error
}
