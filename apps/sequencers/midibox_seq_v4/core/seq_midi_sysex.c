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

#include "seq_ui.h"
#include "seq_lcd.h"
#include "seq_midi_sysex.h"


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


#define SYSEX_REMOTE_CMD_OFF       0x00
#define SYSEX_REMOTE_CMD_ON        0x01
#define SYSEX_REMOTE_CMD_LCD       0x02
#define SYSEX_REMOTE_CMD_CHARSET   0x03

#define SYSEX_REMOTE_CMD_ALLOCATED  0x7c
#define SYSEX_REMOTE_CMD_INCOMPLETE 0x7d
#define SYSEX_REMOTE_CMD_DISABLED   0x7e
#define SYSEX_REMOTE_CMD_ERROR      0x7f


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
    unsigned PING_BYTE_RECEIVED;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned REMOTE_CMD_RECEIVED:1;
    unsigned REMOTE_CMD_VALID:1;
    unsigned REMOTE_CMD_COMPLETE:1;
    unsigned REMOTE_NO_ACK:1;
    unsigned REMOTE_CMD:8;
    s8       REMOTE_LCD_X;
    s8       REMOTE_LCD_Y;
  };
} sysex_state_t;



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_SYSEX_CmdFinished(void);
static s32 SEQ_MIDI_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 SEQ_MIDI_SYSEX_Cmd_Remote(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
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

static mios32_midi_port_t remote_sysex_port;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_Init(u32 mode)
{
  last_sysex_port = DEFAULT;
  sysex_state.ALL = 0;
  sysex_device_id = MIOS32_MIDI_DeviceIDGet(); // taken from MIOS32
  remote_sysex_port = DEFAULT;

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

  return 0; // no error
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
    case 0x09:
      SEQ_MIDI_SYSEX_Cmd_Remote(port, cmd_state, midi_in);
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
// Remote Command Handler
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_SYSEX_Cmd_Remote(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      sysex_state.REMOTE_CMD_RECEIVED = 0;
      sysex_state.REMOTE_CMD_VALID = 0;
      sysex_state.REMOTE_CMD_COMPLETE = 0;
      sysex_state.REMOTE_NO_ACK = 0;
      sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_ERROR;
      sysex_state.REMOTE_LCD_X = -1;
      sysex_state.REMOTE_LCD_Y = -1;
      break;

    case SYSEX_CMD_STATE_CONT:
      if( (seq_ui_remote_port != DEFAULT && port != seq_ui_remote_port) ||
	  (remote_sysex_port != DEFAULT && remote_sysex_port != port) ) {
	sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_ALLOCATED;
      } else if( !sysex_state.REMOTE_CMD_RECEIVED ) {
	sysex_state.REMOTE_CMD = midi_in;
	sysex_state.REMOTE_CMD_RECEIVED = 1;

	switch( sysex_state.REMOTE_CMD ) {
	  case SYSEX_REMOTE_CMD_OFF:
	    if( seq_ui_remote_mode == SEQ_UI_REMOTE_MODE_AUTO || seq_ui_remote_mode == SEQ_UI_REMOTE_MODE_CLIENT ) {
	      sysex_state.REMOTE_CMD_VALID = 1;
	      sysex_state.REMOTE_CMD_COMPLETE = 1;
	      seq_ui_remote_client_active = 0;
	      remote_sysex_port = DEFAULT;	      
	    } else {
	      sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_DISABLED;
	    }
	    break;

	  case SYSEX_REMOTE_CMD_ON:
	    if( seq_ui_remote_mode == SEQ_UI_REMOTE_MODE_AUTO || seq_ui_remote_mode == SEQ_UI_REMOTE_MODE_CLIENT ) {
	      sysex_state.REMOTE_CMD_VALID = 1;
	      sysex_state.REMOTE_CMD_COMPLETE = 1;
	      seq_ui_remote_client_active = 1;
	      remote_sysex_port = port;
	      SEQ_LCD_Clear();
	      SEQ_LCD_CursorSet(0, 0);
	      SEQ_LCD_PrintString("MBSEQ Remote Client activated.");
	    } else
	      sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_DISABLED;
	    break;

	  case SYSEX_REMOTE_CMD_LCD:
	    if( !seq_ui_remote_client_active )
	      sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_DISABLED;
	    else {
	      sysex_state.REMOTE_CMD_VALID = 1;
	    }
	    break;

	  case SYSEX_REMOTE_CMD_CHARSET:
	    if( !seq_ui_remote_client_active )
	      sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_DISABLED;
	    else {
	      sysex_state.REMOTE_CMD_VALID = 1;
	    }
	    break;

	  default:
	    sysex_state.REMOTE_CMD = SYSEX_REMOTE_CMD_ERROR;
	}


      } else {
	switch( sysex_state.REMOTE_CMD ) {
	  case SYSEX_REMOTE_CMD_LCD:
	    if( sysex_state.REMOTE_LCD_X < 0 )
	      sysex_state.REMOTE_LCD_X = midi_in;
	    else if( sysex_state.REMOTE_LCD_Y < 0 ) {
	      sysex_state.REMOTE_LCD_Y = midi_in;
	    } else {
	      sysex_state.REMOTE_CMD_COMPLETE = 1;
	      sysex_state.REMOTE_NO_ACK = 1; // no acknowledge to save bandwidth!
	      if( sysex_state.REMOTE_LCD_X < 80 && sysex_state.REMOTE_LCD_Y < 2 ) {
		SEQ_LCD_CursorSet(sysex_state.REMOTE_LCD_X, sysex_state.REMOTE_LCD_Y);
		SEQ_LCD_PrintChar(midi_in);
		++sysex_state.REMOTE_LCD_X;
	      }
	    }
	    break;

	  case SYSEX_REMOTE_CMD_CHARSET:
	    sysex_state.REMOTE_CMD_COMPLETE = 1;
	    SEQ_LCD_InitSpecialChars(midi_in);
	    break;
	}
      } break;

    default: // SYSEX_CMD_STATE_END
      if( !sysex_state.REMOTE_CMD_VALID )
	SEQ_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, sysex_state.REMOTE_CMD); // e.g. ERROR or DISABLED code
      else if( !sysex_state.REMOTE_CMD_COMPLETE ) {
	SEQ_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, SYSEX_REMOTE_CMD_INCOMPLETE);
      } else if( !sysex_state.REMOTE_NO_ACK )
	SEQ_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, sysex_state.REMOTE_CMD);
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
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
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



/////////////////////////////////////////////////////////////////////////////
// This function is called to send an on/off request to the server
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendOnOffRequest(u8 on)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(seq_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = seq_midi_sysex_header[i];

  // device ID of remote server
  *sysex_buffer_ptr++ = seq_ui_remote_id;

  // send remote request
  *sysex_buffer_ptr++ = 0x09;
  *sysex_buffer_ptr++ = on ? 0x01 : 0x00;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(seq_ui_remote_port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function is called to send a button event in remote client mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendButton(u8 pin, u8 depressed)
{
  // using a common MIDI event instead of SysEx for faster transfers
  mios32_midi_package_t p;

  p.type = NoteOn;
  p.event = NoteOn;
  p.chn = (pin > 128) ? 1 : 0;
  p.note = pin & 0x7f;
  p.velocity = depressed ? 0x00 : 0x7f;
  return MIOS32_MIDI_SendPackage(remote_sysex_port, p);
}

/////////////////////////////////////////////////////////////////////////////
// This function is called to send a BLM button event in remote client mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_REMOTE_Client_Send_BLM_Button(u8 row, u8 pin, u8 depressed)
{
  // using a common MIDI event instead of SysEx for faster transfers
  mios32_midi_package_t p;

  p.type = NoteOn;
  p.event = NoteOn;
  p.chn = 2 + (row >> 2); // prepared for 16x32 matrix!
  p.note = ((row&0x3) << 5) | (pin & 0x1f);
  p.velocity = depressed ? 0x00 : 0x7f;
  return MIOS32_MIDI_SendPackage(remote_sysex_port, p);
}

/////////////////////////////////////////////////////////////////////////////
// This function is called to send an encoder event in remote client mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendEncoder(u8 encoder, s8 incrementer)
{
  // using a common MIDI event instead of SysEx for faster transfers
  mios32_midi_package_t p;

  p.type = CC;
  p.event = CC;
  p.chn = 0;
  p.cc_number = 15 + encoder; // datawheel = 15, GP encoders = 16..
  p.value = (64 + incrementer) & 0x7f;
  return MIOS32_MIDI_SendPackage(remote_sysex_port, p);
}
