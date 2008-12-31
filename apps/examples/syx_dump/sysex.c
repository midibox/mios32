// $Id$
/*
 * SysEx Parser Demo
 * see README.txt for details
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

#include "app.h"
#include "patch.h"
#include "sysex.h"


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SYSEX_CmdFinished(void);
static s32 SYSEX_SendFooter(u8 force);
static s32 SYSEX_Cmd(u8 cmd_state, u8 midi_in);

static s32 SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in);
static s32 SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in);
static s32 SYSEX_Cmd_Ping(u8 cmd_state, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_cmd;

static mios32_midi_port_t sysex_port = DEFAULT;
static u8 sysex_bank;
static u8 sysex_patch;
static u8 sysex_checksum;
static u8 sysex_received_checksum;
static u16 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

// should be changed for your own application
// Headers used by MIDIbox applications are documented here:
// http://svnmios.midibox.org/filedetails.php?repname=svn.mios&path=%2Ftrunk%2Fdoc%2FSYSEX_HEADERS
// if you decide to use "F0 00 00 7E" prefix, please ensure that your
// own ID (here: 0x7f) will be entered into this document.
// Otherwise please use a different header
const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x7f };


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

// TODO: use malloc function instead of a global array to save RAM
u8 sysex_buffer[1024];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Send(mios32_midi_port_t port, u8 bank, u8 patch)
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

  // send bank and patch number
  sysex_buffer[sysex_buffer_ix++] = patch;
  sysex_buffer[sysex_buffer_ix++] = bank;

  // send patch content
  for(checksum=0, i=0; i<SYSEX_PATCH_SIZE; ++i) {
    c = PATCH_ReadByte((u8)i);

#if   SYSEX_FORMAT == 0    // 7bit format - 8th bit discarded
    sysex_buffer[sysex_buffer_ix++] = c & 0x7f;
    checksum += c;
#elif SYSEX_FORMAT == 1    // two nibble format - low-nibble first
    sysex_buffer[sysex_buffer_ix++] = c & 0x0f;
    checksum += c & 0x0f;
    sysex_buffer[sysex_buffer_ix++] = c >> 4;
    checksum += c >> 4;
#else
# error "unsupported SYSEX_FORMAT"
#endif
  }

#if SYSEX_CHECKSUM_PROTECTION
  // send checksum
  sysex_buffer[sysex_buffer_ix++] = -checksum & 0x7f;
#endif

  // send footer
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
  int i;
  int sysex_buffer_ix = 0;
  u8 checksum;
  u8 c;

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
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != sysex_port )
    return -1;

  sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( midi_in != sysex_header[sysex_state.CTR] ) {
      // incoming byte doesn't match
      SYSEX_CmdFinished();
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

  return sysex_state.MY_SYSEX ? 1 : 0; // no error - return 1 if new command is received
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
    case 0x01:
      SYSEX_Cmd_ReadPatch(cmd_state, midi_in);
      break;
    case 0x02:
      SYSEX_Cmd_WritePatch(cmd_state, midi_in);
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
// Command 01: Read Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Cmd_ReadPatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.BANK_RECEIVED ) {
	sysex_bank = midi_in; // store bank number
	sysex_state.BANK_RECEIVED = 1;
      } else if( !sysex_state.PATCH_RECEIVED ) {
	sysex_patch = midi_in; // store patch number
	sysex_state.PATCH_RECEIVED = 1;
      } else {
	// wait for 0xf7
      }
      break;

    default: // SYSEX_CMD_STATE_END
      SYSEX_SendFooter(0);

      // bank and patch received? (PATCH_RECEIVED implies that BANK_RECEIVED already set)
      if( !sysex_state.PATCH_RECEIVED ) {
	// not enough bytes received
	SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
	// load patch
	if( PATCH_Load(sysex_bank, sysex_patch) ) {
	  // read failed (bankstick not available)
	  SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_BS_NOT_AVAILABLE);
	} else {
	  // send dump
	  SYSEX_Send(sysex_port, sysex_bank, sysex_patch);
	}
      }

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Patch handler
/////////////////////////////////////////////////////////////////////////////
s32 SYSEX_Cmd_WritePatch(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      sysex_checksum = 0; // clear checksum
      sysex_receive_ctr = 0; // clear byte counter
      sysex_received_checksum = 0;
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.BANK_RECEIVED ) {
	sysex_bank = midi_in; // store bank number
	sysex_state.BANK_RECEIVED = 1;
      } else if( !sysex_state.PATCH_RECEIVED ) {
	sysex_patch = midi_in; // store patch number
	sysex_state.PATCH_RECEIVED = 1;
      } else {
	if( sysex_receive_ctr < SYSEX_PATCH_DUMP_SIZE ) {

	  // new byte has been received - put it into patch structure

#if   SYSEX_FORMAT == 0    // 7bit format - 8th bit discarded
	  PATCH_WriteByte((u8)sysex_receive_ctr, midi_in);
#elif SYSEX_FORMAT == 1    // two nibble format - low-nibble first
	  if( (sysex_receive_ctr&1) == 0 ) {
	    // low-nibble has been received
	    PATCH_WriteByte((u8)(sysex_receive_ctr>>1), midi_in & 0x0f);
	  } else {
	    // high-nibble has been received, merge it with previously received low-nibble
	    PATCH_WriteByte((u8)(sysex_receive_ctr>>1), 
			    PATCH_ReadByte((u8)(sysex_receive_ctr>>1)) & 0x0f | ((midi_in&0x0f) << 4));
	  }
#else
   XXX unsupported SYSEX_FORMAT XXX
#endif

	  // add to checksum
	  sysex_checksum += midi_in;

#if SYSEX_CHECKSUM_PROTECTION
	} else if( sysex_receive_ctr == SYSEX_PATCH_DUMP_SIZE ) {
	  // store received checksum
	  sysex_received_checksum = midi_in;
#endif
	} else {
	  // wait for F7
	}

	// increment counter
	++sysex_receive_ctr;
      }
      break;

    default: // SYSEX_CMD_STATE_END
      SYSEX_SendFooter(0);

      if( sysex_receive_ctr < SYSEX_PATCH_DUMP_SIZE ) {
	// not enough bytes received
	SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_receive_ctr > SYSEX_PATCH_DUMP_SIZE ) {
	// too many bytes received
	SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_MORE_BYTES_THAN_EXP);
#if SYSEX_CHECKSUM_PROTECTION
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_WRONG_CHECKSUM);
#endif
      } else {
	// write patch
	s32 error;
	if( error = PATCH_Store(sysex_bank, sysex_patch) ) {
	  // write failed (bankstick not available)
	  SYSEX_SendAck(sysex_port, SYSEX_DISACK, SYSEX_DISACK_BS_NOT_AVAILABLE);
	} else {
	  // notify that bytes have been received
	  SYSEX_SendAck(sysex_port, SYSEX_ACK, 0x00);

	  // print message
	  print_msg = PRINT_MSG_DUMP_RECEIVED;
	}
      }
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


