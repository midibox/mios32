// $Id$
/*
 * SysEx Parser for MIDIbox CV V2
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

#include "app.h"
#include "mbcv_patch.h"
#include "mbcv_sysex.h"

#include "mbcv_file.h"
#include "mbcv_file_p.h"

/////////////////////////////////////////////////////////////////////////////
// local definitions
/////////////////////////////////////////////////////////////////////////////


// help constant - don't change!
#define MBCV_SYSEX_PATCH_DUMP_SIZE  (MBCV_PATCH_SIZE)

// command states
#define MBCV_SYSEX_CMD_STATE_BEGIN 0
#define MBCV_SYSEX_CMD_STATE_CONT  1
#define MBCV_SYSEX_CMD_STATE_END   2

// ack/disack code
#define MBCV_SYSEX_DISACK   0x0e
#define MBCV_SYSEX_ACK      0x0f

// disacknowledge arguments
#define MBCV_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define MBCV_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define MBCV_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define MBCV_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define MBCV_SYSEX_DISACK_INVALID_COMMAND      0x0c


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

  struct {
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
    unsigned PATCH_RECEIVED:1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
  };
} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MBCV_SYSEX_CmdFinished(void);
static s32 MBCV_SYSEX_SendFooter(u8 force);
static s32 MBCV_SYSEX_Cmd(u8 cmd_state, u8 midi_in);

static s32 MBCV_SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in);
static s32 MBCV_SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in);
static s32 MBCV_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_cmd;

static mios32_midi_port_t sysex_port = DEFAULT;
static u8 sysex_patch;
static u8 sysex_checksum;
static u8 sysex_block;
static u8 sysex_received_checksum;
static u16 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

// SysEx header of MIDIbox CV V2
static const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x48 };


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// TODO: use malloc function instead of a global array to save RAM
static u8 sysex_buffer[MBCV_SYSEX_PATCH_DUMP_SIZE + 100];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // install SysEx parser
  //MIOS32_MIDI_SysExCallback_Init(MBCV_SYSEX_Parser);
  // parser called from APP_SYSEX_Parser

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_Send(mios32_midi_port_t port, u8 patch)
{
  int i;
  int sysex_buffer_ix = 0;
  u8 checksum;
  u8 c;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // add device ID
  sysex_buffer[sysex_buffer_ix++] = MIOS32_MIDI_DeviceIDGet();

  // "write patch" command (so that dump could be sent back to overwrite EEPROM w/o modifications)
  sysex_buffer[sysex_buffer_ix++] = 0x02;

  // send patch number
  sysex_buffer[sysex_buffer_ix++] = patch;

  // send patch content
  for(checksum=0, i=0; i<MBCV_PATCH_SIZE; ++i) {
    c = MBCV_PATCH_ReadByte(i);

    // 7bit format - 8th bit discarded
    sysex_buffer[sysex_buffer_ix++] = c & 0x7f;
    checksum += c & 0x7f;
  }

  // send checksum
  sysex_buffer[sysex_buffer_ix++] = -checksum & 0x7f;

  // send footer
  sysex_buffer[sysex_buffer_ix++] = 0xf7;

  // finally send SysEx stream and return error status
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, sysex_buffer_ix);
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  int i;
  int sysex_buffer_ix = 0;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // add device ID
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
s32 MBCV_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
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
      MBCV_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR == (sizeof(sysex_header)+1) ) {
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
      	MBCV_SYSEX_Cmd(MBCV_SYSEX_CMD_STATE_END, midi_in);
      }
      MBCV_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	MBCV_SYSEX_Cmd(MBCV_SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	MBCV_SYSEX_Cmd(MBCV_SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_CmdFinished(void)
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
s32 MBCV_SYSEX_SendFooter(u8 force)
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
s32 MBCV_SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x01:
      MBCV_SYSEX_Cmd_ReadPatch(cmd_state, midi_in);
      break;
    case 0x02:
      MBCV_SYSEX_Cmd_WritePatch(cmd_state, midi_in);
      break;
    case 0x0f:
      MBCV_SYSEX_Cmd_Ping(cmd_state, midi_in);
      break;
    default:
      // unknown command
      MBCV_SYSEX_SendFooter(0);
      MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_INVALID_COMMAND);
      MBCV_SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 01: Read Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MBCV_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MBCV_SYSEX_CMD_STATE_CONT:
      if( !sysex_state.PATCH_RECEIVED ) {
        sysex_patch = midi_in; // store patch number
        sysex_state.PATCH_RECEIVED = 1;
      } else {
        // wait for 0xf7
      }
      break;

    default: // MBCV_SYSEX_CMD_STATE_END
      MBCV_SYSEX_SendFooter(0);

#      // patch received?
      if( !sysex_state.PATCH_RECEIVED ) {
        // not enough bytes received
        MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
#if 0
	// load patch
	if( MBCV_PATCH_Load(mbcv_file_p_patch_name) ) {
	  // read failed (bankstick not available)
	  MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_BS_NOT_AVAILABLE);
	} else {
	  // send dump
	  MBCV_SYSEX_Send(sysex_port);
	}
#else
	// send dump
	MBCV_SYSEX_Send(sysex_port, sysex_patch);
#endif
      }

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MBCV_SYSEX_CMD_STATE_BEGIN:
      sysex_checksum = 0; // clear checksum
      sysex_receive_ctr = 0; // clear byte counter
      sysex_received_checksum = 0;
      break;

    case MBCV_SYSEX_CMD_STATE_CONT:
      if( !sysex_state.PATCH_RECEIVED ) {
        sysex_patch = midi_in; // store patch number
        sysex_state.PATCH_RECEIVED = 1;
      } else {
	if( sysex_receive_ctr < MBCV_SYSEX_PATCH_DUMP_SIZE ) {

	  // new byte has been received - put it into patch structure

	  // 7bit format - 8th bit discarded
	  MBCV_PATCH_WriteByte(sysex_receive_ctr, midi_in);

	  // add to checksum
	  sysex_checksum += midi_in;

	} else if( sysex_receive_ctr == MBCV_SYSEX_PATCH_DUMP_SIZE ) {
	  // store received checksum
	  sysex_received_checksum = midi_in;
	} else {
	  // wait for F7
	}

	// increment counter
	++sysex_receive_ctr;
      }
      break;

    default: // MBCV_SYSEX_CMD_STATE_END
      MBCV_SYSEX_SendFooter(0);

      if( sysex_receive_ctr < (MBCV_SYSEX_PATCH_DUMP_SIZE+1) ) {
	// not enough bytes received
	MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_receive_ctr > (MBCV_SYSEX_PATCH_DUMP_SIZE+1) ) {
	// too many bytes received
	MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// write patch
	s32 error;
	if( (error = MBCV_PATCH_Store(mbcv_file_p_patch_name)) ) {
	  // write failed (bankstick not available)
	  MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_DISACK, MBCV_SYSEX_DISACK_BS_NOT_AVAILABLE);
	} else {
	  // notify that bytes have been received
	  MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_ACK, 0x00);

	  // print message
	  //print_msg = PRINT_MSG_DUMP_RECEIVED;
	}
      }
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MBCV_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MBCV_SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // MBCV_SYSEX_CMD_STATE_END
      MBCV_SYSEX_SendFooter(0);

      // send acknowledge
      MBCV_SYSEX_SendAck(sysex_port, MBCV_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}


