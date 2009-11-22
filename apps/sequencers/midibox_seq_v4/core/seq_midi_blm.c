// $Id$
/*
 * MIDI Routines for MBHP_BLM_SCALAR access
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

#include "seq_midi_blm.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define SYSEX_DISACK_INVALID_COMMAND      0x0c


#define SYSEX_BLM_CMD_REQUEST      0x00
#define SYSEX_BLM_CMD_LAYOUT       0x01


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

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned COLUMNS_RECEIVED:1;
    unsigned ROWS_RECEIVED:1;
    unsigned COLOURS_RECEIVED:1;
  };

} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

mios32_midi_port_t seq_midi_blm_port;
u8 seq_midi_blm_num_steps;
u8 seq_midi_blm_num_tracks;
u8 seq_midi_blm_num_colours;
u8 seq_midi_blm_force_update;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_BLM_SYSEX_CmdFinished(void);
static s32 SEQ_MIDI_BLM_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_BLM_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_BLM_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_BLM_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const u8 seq_midi_blm_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4e }; // Header of MBHP_BLM_SCALAR

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_BLM_Init(u32 mode)
{
  seq_midi_blm_port = 0; // disabled
  seq_midi_blm_num_steps = 16;
  seq_midi_blm_num_tracks = 16;
  seq_midi_blm_num_colours = 2;
  seq_midi_blm_force_update = 0;

  sysex_device_id = 0; // only device 0 supported yet

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
// Called from 
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_BLM_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  if( !seq_midi_blm_port )
    return -1; // MIDI In not configured

  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // check for MIDI port
  if( sysex_state.MY_SYSEX && port != seq_midi_blm_port )
    return -1;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(seq_midi_blm_sysex_header) && midi_in != seq_midi_blm_sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(seq_midi_blm_sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      SEQ_MIDI_BLM_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(seq_midi_blm_sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	SEQ_MIDI_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_END, midi_in);
      }
      SEQ_MIDI_BLM_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	SEQ_MIDI_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	SEQ_MIDI_BLM_SYSEX_Cmd(port, SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_BLM_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_BLM_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case SYSEX_BLM_CMD_REQUEST: // ignore to avoid loopbacks
      break;

    case SYSEX_BLM_CMD_LAYOUT:
      SEQ_MIDI_BLM_SYSEX_Cmd_Layout(port, cmd_state, midi_in);
      break;

    case 0x0e: // ignore to avoid loopbacks
      break;

    case 0x0f:
      SEQ_MIDI_BLM_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      SEQ_MIDI_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND);
      SEQ_MIDI_BLM_SYSEX_CmdFinished();      
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Remote Command Handler
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_BLM_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.COLUMNS_RECEIVED ) {
	sysex_state.COLUMNS_RECEIVED = 1;
	seq_midi_blm_num_steps = midi_in;
	if( seq_midi_blm_num_steps == 0 ) // extra to provide 128 steps (steps & 0x7f)
	  seq_midi_blm_num_steps = 128;
      } else if( !sysex_state.ROWS_RECEIVED ) {
	sysex_state.ROWS_RECEIVED = 1;
	seq_midi_blm_num_tracks = midi_in;
      } else if( !sysex_state.COLOURS_RECEIVED ) {
	sysex_state.COLOURS_RECEIVED = 1;
	seq_midi_blm_num_colours = midi_in;
      }
      // ignore all other bytes
      // don't sent error message to allow future extensions
      break;

    default: // SYSEX_CMD_STATE_END
      // update BLM
      seq_midi_blm_force_update = 1;
      // send acknowledge
      SEQ_MIDI_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_BLM_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
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
	SEQ_MIDI_BLM_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_BLM_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(seq_midi_blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_midi_blm_sysex_header[i];

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
s32 SEQ_MIDI_BLM_SYSEX_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX )
    SEQ_MIDI_BLM_SYSEX_CmdFinished();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called to request the layout
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_BLM_SYSEX_SendRequest(u8 req)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  if( !seq_midi_blm_port )
    return -1; // MIDI Out not configured

  for(i=0; i<sizeof(seq_midi_blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_midi_blm_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send request
  *sysex_buffer_ptr++ = SYSEX_BLM_CMD_REQUEST;
  *sysex_buffer_ptr++ = req;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(seq_midi_blm_port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  MUTEX_MIDIOUT_GIVE;
  return status;
}
