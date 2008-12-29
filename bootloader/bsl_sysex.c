// $Id: sysex.c 78 2008-10-12 22:09:23Z tk $
/*
 * BSL SysEx Parser
 *
 * MEMO: upload of ca. 50k code via UART MIDI: 24s, via USB: ca. 5s! :-)
 *
 * TODO: support for device id
 * TODO: command to request MIOS32 version, chip ID and flash size
 * TODO: command to halt BSL (so that the application won't be started before releasing this state)
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
#include <string.h>

#include "bsl_sysex.h"


/////////////////////////////////////////////////////////////////////////////
// Local Macros
/////////////////////////////////////////////////////////////////////////////

#define MEM32(addr) (*((volatile u32 *)(addr)))
#define MEM16(addr) (*((volatile u16 *)(addr)))
#define MEM8(addr)  (*((volatile u8  *)(addr)))

// STM32: base address of flash memory
#define FLASH_BASE_ADDR   0x08000000

// STM32: determine size of flash, it's stored in the "electronic signature"
#define FLASH_SIZE        (MEM16(0x1ffff7e0) * 0x400)

// STM32: determine page size (mid density devices: 1k, high density devices: 2k)
// TODO: find a proper way, as there could be high density devices with less than 256k?)
#define FLASH_PAGE_SIZE   (MEM16(0x1ffff7e0) >= 0x100 ? 0x800 : 0x400)


// STM32: base address of SRAM
#define SRAM_BASE_ADDR    0x20000000

// STM32: determine size of SRAM, it's stored in the "electronic signature"
#define SRAM_SIZE         (MEM16(0x1ffff7e2) * 0x400)



/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 BSL_SYSEX_CmdFinished(void);
static s32 BSL_SYSEX_SendFooter(u8 force);
static s32 BSL_SYSEX_Cmd(bsl_sysex_cmd_state_t cmd_state, u8 midi_in);

static s32 BSL_SYSEX_Cmd_ReadFlash(bsl_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BSL_SYSEX_Cmd_WriteFlash(bsl_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BSL_SYSEX_Cmd_Ping(bsl_sysex_cmd_state_t cmd_state, u8 midi_in);

static s32 BSL_SYSEX_RecAddrAndLen(u8 midi_in);

static s32 BSL_SYSEX_SendMem(mios32_midi_port_t port, u32 addr, u32 len);
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;

static bsl_sysex_rec_state_t sysex_rec_state;

static mios32_midi_port_t sysex_port = DEFAULT;
static u32 sysex_addr;
static u32 sysex_len;
static u8 sysex_checksum;
static u8 sysex_received_checksum;
static u32 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// constant definitions
/////////////////////////////////////////////////////////////////////////////

static const u8 sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x32 };


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 sysex_buffer[BSL_SYSEX_BUFFER_SIZE];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // TODO: allow to change device ID (read from flash, resp. config sector)
  sysex_device_id = 0x00;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  int i;

  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  for(i=0; i<sizeof(sysex_header); ++i)
    *sysex_buffer_ptr++ = sysex_header[i];

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
// This function parses an incoming sysex stream for SysEx messages
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != sysex_port )
    return -1;

  sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(sysex_header) && midi_in != sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      BSL_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	BSL_SYSEX_Cmd(BSL_SYSEX_CMD_STATE_END, midi_in);
      }
      BSL_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	BSL_SYSEX_Cmd(BSL_SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	BSL_SYSEX_Cmd(BSL_SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_rec_state = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends the SysEx footer if merger enabled
// if force == 1, send the footer regardless of merger state
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendFooter(u8 force)
{
  // function not relevant - merger not supported anyhow

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd(bsl_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x01:
      BSL_SYSEX_Cmd_ReadFlash(cmd_state, midi_in);
      break;
    case 0x02:
      BSL_SYSEX_Cmd_WriteFlash(cmd_state, midi_in);
      break;
    case 0x0f:
      BSL_SYSEX_Cmd_Ping(cmd_state, midi_in);
      break;
    default:
      // unknown command
      BSL_SYSEX_SendFooter(0);
      BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, BSL_SYSEX_DISACK_INVALID_COMMAND);
      BSL_SYSEX_CmdFinished();      
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 01: Read Flash handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_ReadFlash(bsl_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case BSL_SYSEX_CMD_STATE_BEGIN:
      // set initial receive state and address/len
      sysex_rec_state = BSL_SYSEX_REC_A3;
      sysex_addr = 0;
      sysex_len = 0;
      break;

    case BSL_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state < BSL_SYSEX_REC_PAYLOAD )
	BSL_SYSEX_RecAddrAndLen(midi_in);
      break;

    default: // BSL_SYSEX_CMD_STATE_END
      BSL_SYSEX_SendFooter(0);

      // did we reach payload state?
      if( sysex_rec_state != BSL_SYSEX_REC_PAYLOAD ) {
	// not enough bytes received
	BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, BSL_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
	// send dump
	BSL_SYSEX_SendMem(sysex_port, sysex_addr, sysex_len);
      }

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Flash handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_WriteFlash(bsl_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u32 bit_ctr8 = 0;
  static u32 value8 = 0;

  switch( cmd_state ) {

    case BSL_SYSEX_CMD_STATE_BEGIN:
      // set initial receive state and address/len
      sysex_rec_state = BSL_SYSEX_REC_A3;
      sysex_addr = 0;
      sysex_len = 0;
      // clear checksum and receive counters
      sysex_checksum = 0;
      sysex_received_checksum = 0;

      sysex_receive_ctr = 0;
      bit_ctr8 = 0;
      value8 = 0;
      break;

    case BSL_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state < BSL_SYSEX_REC_PAYLOAD ) {
	sysex_checksum += midi_in;
	BSL_SYSEX_RecAddrAndLen(midi_in);
      } else if( sysex_rec_state == BSL_SYSEX_REC_PAYLOAD ) {
	sysex_checksum += midi_in;
	// new byte has been received - descramble and buffer it
	if( sysex_receive_ctr < BSL_SYSEX_MAX_BYTES ) {
	  u8 value7 = midi_in;
	  int bit_ctr7;
	  for(bit_ctr7=0; bit_ctr7<7; ++bit_ctr7) {
	    value8 = (value8 << 1) | ((value7 & 0x40) ? 1 : 0);
	    value7 <<= 1;

	    if( ++bit_ctr8 >= 8 ) {
	      sysex_buffer[sysex_receive_ctr] = value8;
	      bit_ctr8 = 0;
	      if( ++sysex_receive_ctr >= sysex_len )
		sysex_rec_state = BSL_SYSEX_REC_CHECKSUM;
	    }
	  }
	}
      } else if( sysex_rec_state == BSL_SYSEX_REC_CHECKSUM ) {
	// store received checksum
	sysex_received_checksum = midi_in;
      } else {
	// too many bytes... wait for F7
	sysex_rec_state = BSL_SYSEX_REC_INVALID;
      }
      break;

    default: // BSL_SYSEX_CMD_STATE_END
      BSL_SYSEX_SendFooter(0);

      if( sysex_receive_ctr < sysex_len ) {
	// not enough bytes received
	BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, BSL_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_rec_state == BSL_SYSEX_REC_INVALID ) {
	// too many bytes received
	BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, BSL_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, BSL_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// write flash
	s32 error;
	if( error = BSL_SYSEX_WriteMem(sysex_addr, sysex_len, sysex_buffer) ) {
	  // write failed - return negated error status
	  BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_DISACK, -error);
	} else {
	  // notify that bytes have been received by returning checksum
	  BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_ACK, -sysex_checksum & 0x7f);
	}
      }
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_Ping(bsl_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case BSL_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case BSL_SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // BSL_SYSEX_CMD_STATE_END
      BSL_SYSEX_SendFooter(0);

      // send acknowledge
      BSL_SYSEX_SendAck(sysex_port, BSL_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to receive address and length
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_RecAddrAndLen(u8 midi_in)
{
  if( sysex_rec_state <= BSL_SYSEX_REC_A0 ) {
    sysex_addr = (sysex_addr << 7) | ((midi_in & 0x7f) << 3);
    if( sysex_rec_state == BSL_SYSEX_REC_A0 )
      sysex_rec_state = BSL_SYSEX_REC_L3;
    else
      ++sysex_rec_state;
  } else if( sysex_rec_state <= BSL_SYSEX_REC_L0 ) {
    sysex_len = (sysex_len << 7) | ((midi_in & 0x7f) << 3);
    if( sysex_rec_state == BSL_SYSEX_REC_L0 ) {
      sysex_rec_state = BSL_SYSEX_REC_PAYLOAD;
    } else {
      ++sysex_rec_state;
    }
  } else {
    return -1; // function shouldn't be called in this state
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx dump of the requested memory address range
// We expect that address and length are aligned to 16
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendMem(mios32_midi_port_t port, u32 addr, u32 len)
{
  int i;
  u8 checksum = 0;

  // send header
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  for(i=0; i<sizeof(sysex_header); ++i)
    *sysex_buffer_ptr++ = sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // "write flash" command (so that dump could be sent back to overwrite flash w/o modifications)
  *sysex_buffer_ptr++ = 0x02;

  // send 32bit address (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (addr >> 24) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 17) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 10) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >>  3) & 0x7f;

  // send 32bit range (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (len >> 24) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 17) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 10) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >>  3) & 0x7f;

  // send flash content
  u8 value7 = 0;
  u8 bit_ctr7 = 0;
  i=0;
  for(i=0; i<len; ++i) {
    u8 value8 = MEM8(addr+i);
    u8 bit_ctr8;
    for(bit_ctr8=0; bit_ctr8<8; ++bit_ctr8) {
      value7 = (value7 << 1) | ((value8 & 0x80) ? 1 : 0);
      value8 <<= 1;

      if( ++bit_ctr7 >= 7 ) {
	checksum += *sysex_buffer_ptr++ = value7;
	value7 = 0;
	bit_ctr7 = 0;
      }
    }
  }

  if( bit_ctr7 )
    checksum += *sysex_buffer_ptr++ = value7;

  // send checksum
  *sysex_buffer_ptr++ = -checksum & 0x7f;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function writes into a memory
// We expect that address and length are aligned to 16
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer)
{
  // check for alignment
  if( (addr % 4) || (len % 4) )
    return -BSL_SYSEX_DISACK_ADDR_NOT_ALIGNED;

  // check for flash memory range
  if( addr >= FLASH_BASE_ADDR && addr < (FLASH_BASE_ADDR + FLASH_SIZE) ) {
    // FLASH_* routines are part of the STM32 code library
    FLASH_Unlock();

    FLASH_Status status;
    int i;
    for(i=0; i<len; addr+=2, i+=2) {
      if( (addr % 0x800) == 0 ) {
	if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE )
	  return -BSL_SYSEX_DISACK_WRITE_FAILED;
      }
      
      if( (status=FLASH_ProgramHalfWord(addr, buffer[i+0] | ((u16)buffer[i+1] << 8))) != FLASH_COMPLETE )
	return -BSL_SYSEX_DISACK_WRITE_FAILED;
    }

    return 0; // no error
  }

  // check for SRAM memory range
  if( addr >= SRAM_BASE_ADDR && addr < (SRAM_BASE_ADDR + SRAM_SIZE) ) {

    // transfer buffer into SRAM
    memcpy((u8 *)addr, (u8 *)buffer, len);

    return 0; // no error
  }

  // invalid address
  return -BSL_SYSEX_DISACK_WRONG_ADDR_RANGE;
}

