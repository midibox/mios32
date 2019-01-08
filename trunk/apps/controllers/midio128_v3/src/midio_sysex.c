// $Id$
/*
 * SysEx Parser for MIDIO128 V3
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
#include "midio_patch.h"
#include "midio_sysex.h"

#include "midio_file.h"
#include "midio_file_p.h"

/////////////////////////////////////////////////////////////////////////////
// local definitions
/////////////////////////////////////////////////////////////////////////////


// help constant - don't change!
#define MIDIO_SYSEX_PATCH_DUMP_SIZE  (MIDIO_PATCH_SIZE)

// command states
#define MIDIO_SYSEX_CMD_STATE_BEGIN 0
#define MIDIO_SYSEX_CMD_STATE_CONT  1
#define MIDIO_SYSEX_CMD_STATE_END   2

// ack/disack code
#define MIDIO_SYSEX_DISACK   0x0e
#define MIDIO_SYSEX_ACK      0x0f

// disacknowledge arguments
#define MIDIO_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define MIDIO_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define MIDIO_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define MIDIO_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define MIDIO_SYSEX_DISACK_INVALID_COMMAND      0x0c


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
    unsigned BLOCK_RECEIVED:1;
    unsigned :1;
    unsigned :1;
    unsigned :1;
  };
} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIDIO_SYSEX_CmdFinished(void);
static s32 MIDIO_SYSEX_SendFooter(u8 force);
static s32 MIDIO_SYSEX_Cmd(u8 cmd_state, u8 midi_in);

static s32 MIDIO_SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in);
static s32 MIDIO_SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in);
static s32 MIDIO_SYSEX_Cmd_ReadPatchBlock(u8 cmd_state, u8 midi_in);
static s32 MIDIO_SYSEX_Cmd_WritePatchBlock(u8 cmd_state, u8 midi_in);
static s32 MIDIO_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_cmd;

static mios32_midi_port_t sysex_port = DEFAULT;
static u8 sysex_checksum;
static u8 sysex_block;
static u8 sysex_received_checksum;
static u16 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

// SysEx header of MIDIO128
static const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x44 };


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// TODO: use malloc function instead of a global array to save RAM
static u8 sysex_buffer[MIDIO_SYSEX_PATCH_DUMP_SIZE + 100];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // install SysEx parser
  //MIOS32_MIDI_SysExCallback_Init(MIDIO_SYSEX_Parser);
  // parser called from APP_SYSEX_Parser

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Send(mios32_midi_port_t port)
{
  int i;
  int sysex_buffer_ix = 0;
  u8 checksum;
  u8 c;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // "write patch" command (so that dump could be sent back to overwrite EEPROM w/o modifications)
  sysex_buffer[sysex_buffer_ix++] = 0x02;

  // send patch content
  for(checksum=0, i=0; i<MIDIO_PATCH_SIZE; ++i) {
    c = MIDIO_PATCH_ReadByte(i);

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
// This function sends a SysEx dump of a patch block
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_SendBlock(mios32_midi_port_t port, u8 block)
{
  int i;
  int sysex_buffer_ix = 0;
  u8 checksum;
  u8 c;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // "write block" command (so that dump could be sent back to overwrite EEPROM w/o modifications)
  sysex_buffer[sysex_buffer_ix++] = 0x04;

  // write block number
  sysex_buffer[sysex_buffer_ix++] = block;
  checksum = block;

  // send patch content
  u16 addr = block*256;
  for(i=0; i<256; ++i) {
    c = MIDIO_PATCH_ReadByte(addr++);

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
s32 MIDIO_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  int i;
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
s32 MIDIO_SYSEX_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX && port == sysex_port )
    MIDIO_SYSEX_CmdFinished();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != sysex_port )
    return 1; // don't forward package to APP_MIDI_NotifyPackage()

  sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( midi_in != sysex_header[sysex_state.CTR] ) {
      // incoming byte doesn't match
      MIDIO_SYSEX_CmdFinished();
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
      	MIDIO_SYSEX_Cmd(MIDIO_SYSEX_CMD_STATE_END, midi_in);
      }
      MIDIO_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	MIDIO_SYSEX_Cmd(MIDIO_SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	MIDIO_SYSEX_Cmd(MIDIO_SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_CmdFinished(void)
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
s32 MIDIO_SYSEX_SendFooter(u8 force)
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
s32 MIDIO_SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x01:
      MIDIO_SYSEX_Cmd_ReadPatch(cmd_state, midi_in);
      break;
    case 0x02:
      MIDIO_SYSEX_Cmd_WritePatch(cmd_state, midi_in);
      break;
    case 0x03:
      MIDIO_SYSEX_Cmd_ReadPatchBlock(cmd_state, midi_in);
      break;
    case 0x04:
      MIDIO_SYSEX_Cmd_WritePatchBlock(cmd_state, midi_in);
      break;
    case 0x0f:
      MIDIO_SYSEX_Cmd_Ping(cmd_state, midi_in);
      break;
    default:
      // unknown command
      MIDIO_SYSEX_SendFooter(0);
      MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_INVALID_COMMAND);
      MIDIO_SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 01: Read Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIDIO_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MIDIO_SYSEX_CMD_STATE_CONT:
      // wait for 0xf7
      break;

    default: // MIDIO_SYSEX_CMD_STATE_END
      MIDIO_SYSEX_SendFooter(0);

#if 0
      // load patch
      if( MIDIO_PATCH_Load(midio_file_p_patch_name) ) {
	// read failed (bankstick not available)
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_BS_NOT_AVAILABLE);
      } else {
	// send dump
	MIDIO_SYSEX_Send(sysex_port);
      }
#else
      // send dump
      MIDIO_SYSEX_Send(sysex_port);
#endif

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIDIO_SYSEX_CMD_STATE_BEGIN:
      sysex_checksum = 0; // clear checksum
      sysex_receive_ctr = 0; // clear byte counter
      sysex_received_checksum = 0;
      break;

    case MIDIO_SYSEX_CMD_STATE_CONT:
      if( sysex_receive_ctr < MIDIO_SYSEX_PATCH_DUMP_SIZE ) {

	// new byte has been received - put it into patch structure

	// 7bit format - 8th bit discarded
	MIDIO_PATCH_WriteByte(sysex_receive_ctr, midi_in);

	// add to checksum
	sysex_checksum += midi_in;

      } else if( sysex_receive_ctr == MIDIO_SYSEX_PATCH_DUMP_SIZE ) {
	// store received checksum
	sysex_received_checksum = midi_in;
      } else {
	// wait for F7
      }

      // increment counter
      ++sysex_receive_ctr;
      break;

    default: // MIDIO_SYSEX_CMD_STATE_END
      MIDIO_SYSEX_SendFooter(0);

      if( sysex_receive_ctr < (MIDIO_SYSEX_PATCH_DUMP_SIZE+1) ) {
	// not enough bytes received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_receive_ctr > (MIDIO_SYSEX_PATCH_DUMP_SIZE+1) ) {
	// too many bytes received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// write patch
	s32 error;
	if( (error = MIDIO_PATCH_Store(midio_file_p_patch_name)) ) {
	  // write failed (bankstick not available)
	  MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_BS_NOT_AVAILABLE);
	} else {
	  // notify that bytes have been received
	  MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_ACK, 0x00);

	  // print message
	  //print_msg = PRINT_MSG_DUMP_RECEIVED;
	}
      }
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 03: Read Patch block handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Cmd_ReadPatchBlock(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIDIO_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MIDIO_SYSEX_CMD_STATE_CONT:
      if( !sysex_state.BLOCK_RECEIVED ) {
	sysex_block = midi_in; // store block number
	sysex_state.BLOCK_RECEIVED = 1;
      } else {
	// wait for 0xf7
      }
      break;

    default: // MIDIO_SYSEX_CMD_STATE_END
      MIDIO_SYSEX_SendFooter(0);

      // block received?
      if( !sysex_state.BLOCK_RECEIVED ) {
	// not enough bytes received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
	// send partial dump
	MIDIO_SYSEX_SendBlock(sysex_port, sysex_block);
      }

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 04: Write Patch Block handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Cmd_WritePatchBlock(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIDIO_SYSEX_CMD_STATE_BEGIN:
      sysex_checksum = 0; // clear checksum
      sysex_receive_ctr = 0; // clear byte counter
      sysex_received_checksum = 0;
      break;

    case MIDIO_SYSEX_CMD_STATE_CONT:
      if( !sysex_state.BLOCK_RECEIVED ) {
	sysex_block = midi_in; // store block number
	sysex_state.BLOCK_RECEIVED = 1;

	// add to checksum
	sysex_checksum += midi_in;
      } else {
	if( sysex_receive_ctr < 256 ) {
	  // 7bit format - 8th bit discarded
	  u16 addr = 256*sysex_block + sysex_receive_ctr;
	  MIDIO_PATCH_WriteByte(addr, midi_in);

	  // add to checksum
	  sysex_checksum += midi_in;

	} else if( sysex_receive_ctr == 256 ) {
	  // store received checksum
	  sysex_received_checksum = midi_in;
	} else {
	  // wait for F7
	}

	// increment counter
	++sysex_receive_ctr;
      }

      break;

    default: // MIDIO_SYSEX_CMD_STATE_END
      MIDIO_SYSEX_SendFooter(0);
      if( sysex_receive_ctr < (256+1) ) {
	// not enough bytes received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_receive_ctr > MIDIO_SYSEX_PATCH_DUMP_SIZE ) {
	// too many bytes received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// write patch when last block has been written
	if( sysex_block == 5 ) {
	  s32 error;
	  if( (error = MIDIO_PATCH_Store(midio_file_p_patch_name)) ) {
	    // write failed (bankstick not available)
	    MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_DISACK, MIDIO_SYSEX_DISACK_BS_NOT_AVAILABLE);
	  } else {
	    // notify that bytes have been received
	    MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_ACK, 0x00);
	  }
	} else {
	  // notify that bytes have been received
	  MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_ACK, 0x00);
	}
      }
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIDIO_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MIDIO_SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // MIDIO_SYSEX_CMD_STATE_END
      MIDIO_SYSEX_SendFooter(0);

      // send acknowledge
      MIDIO_SYSEX_SendAck(sysex_port, MIDIO_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}


