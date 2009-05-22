// $Id: sysex.c 78 2008-10-12 22:09:23Z tk $
/*
 * BSL SysEx Parser
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

// STM32: determine page size (mid density devices: 1k, high density devices: 2k)
// TODO: find a proper way, as there could be high density devices with less than 256k?)
#define FLASH_PAGE_SIZE   (MIOS32_SYS_FlashSizeGet() >= (256*1024) ? 0x800 : 0x400)

// STM32: flash memory range (16k BSL range excluded)
#define FLASH_START_ADDR  (0x08000000 + 0x4000)
#define FLASH_END_ADDR    (0x08000000 + MIOS32_SYS_FlashSizeGet() - 0x4000 - 1)


// STM32: base address of SRAM
#define SRAM_START_ADDR   (0x20000000)
#define SRAM_END_ADDR     (0x20000000 + MIOS32_SYS_RAMSizeGet() - 1)

// location of device ID (can be overprogrammed... but not erased!)
// must be 16bit aligned!
#define BSL_DEVICE_ID_ADDR (0x08003ffe)


/////////////////////////////////////////////////////////////////////////////
// Internal Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 BSL_FetchDeviceIDFromFlash(void);

static s32 BSL_SYSEX_Cmd_ReadMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BSL_SYSEX_Cmd_WriteMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BSL_SYSEX_Cmd_ChangeDeviceID(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);

static s32 BSL_SYSEX_RecAddrAndLen(u8 midi_in);

static s32 BSL_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);
static s32 BSL_SYSEX_SendMem(mios32_midi_port_t port, u32 addr, u32 len);
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static bsl_sysex_rec_state_t sysex_rec_state;

static u32 sysex_addr;
static u32 sysex_len;
static u8 sysex_checksum;
static u8 sysex_received_checksum;
static u32 sysex_receive_ctr;


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 sysex_buffer[BSL_SYSEX_BUFFER_SIZE];

static u8 halt_state;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // set to one when writing flash to prevent the execution of application code
  // so long flash hasn't been programmed completely
  halt_state = 0;

  BSL_FetchDeviceIDFromFlash();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if BSL is in halt state (e.g. code is uploaded)
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_HaltStateGet(void)
{
  return halt_state;
}


/////////////////////////////////////////////////////////////////////////////
// Used by MIOS32_MIDI to release halt state instead of triggering a reset
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_ReleaseHaltState(void)
{
  // always send upload request (like if we would come out of reset)
  BSL_SYSEX_SendUploadReq(UART0);    
  BSL_SYSEX_SendUploadReq(USB0);

  // clear halt state
  halt_state = 0;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Fetches device ID from flash location
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_FetchDeviceIDFromFlash(void)
{
  // set device ID if < 0x80
  u16 device_id = MEM16(BSL_DEVICE_ID_ADDR);
  if( device_id < 0x80 )
    MIOS32_MIDI_DeviceIDSet(device_id);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function enhances MIOS32 SysEx commands
// it's called from MIOS32_MIDI_SYSEX_Cmd if the "MIOS32_MIDI_BSL_ENHANCEMENTS"
// switch is set (see code there for details)
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in, u8 sysex_cmd)
{
  // wait 2 additional seconds whenever a SysEx message has been received
  MIOS32_STOPWATCH_Reset();

  // enter the commands here
  switch( sysex_cmd ) {
    // case 0x00: // query command is implemented in MIOS32
    // case 0x0f: // ping command is implemented in MIOS32

    case 0x01:
      BSL_SYSEX_Cmd_ReadMem(port, cmd_state, midi_in);
      break;
    case 0x02:
      BSL_SYSEX_Cmd_WriteMem(port, cmd_state, midi_in);
      break;
    case 0x0c:
      BSL_SYSEX_Cmd_ChangeDeviceID(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      return -1; // command not supported
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 01: Read Memory handler
// TODO: we could provide this command also during runtime, as it isn't destructive
// or it could be available as debug command 0D like known from MIOS8
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_ReadMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      // set initial receive state and address/len
      sysex_rec_state = BSL_SYSEX_REC_A3;
      sysex_addr = 0;
      sysex_len = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state < BSL_SYSEX_REC_PAYLOAD )
	BSL_SYSEX_RecAddrAndLen(midi_in);
      break;

    default: // BSL_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      // did we reach payload state?
      if( sysex_rec_state != BSL_SYSEX_REC_PAYLOAD ) {
	// not enough bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else {
	// send dump
	BSL_SYSEX_SendMem(port, sysex_addr, sysex_len);
      }

      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 02: Write Memory handler
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_WriteMem(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u32 bit_ctr8 = 0;
  static u32 value8 = 0;

  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
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

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
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

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      if( sysex_receive_ctr < sysex_len ) {
	// not enough bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP);
      } else if( sysex_rec_state == BSL_SYSEX_REC_INVALID ) {
	// too many bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else if( sysex_received_checksum != (-sysex_checksum & 0x7f) ) {
	// notify that wrong checksum has been received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_WRONG_CHECKSUM);
      } else {
	// enter halt state (can only be released via BSL reset)
	halt_state = 1;

	// write received data into memory
	s32 error;
	if( error = BSL_SYSEX_WriteMem(sysex_addr, sysex_len, sysex_buffer) ) {
	  // write failed - return negated error status
	  BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, -error);
	} else {
	  // notify that bytes have been received by returning checksum
	  BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, -sysex_checksum & 0x7f);
	}
      }
      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 0C: change the Device ID
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_Cmd_ChangeDeviceID(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 new_device_id = 0;

  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      new_device_id = 0;
      sysex_rec_state = BSL_SYSEX_REC_ID;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      if( sysex_rec_state == BSL_SYSEX_REC_ID ) {
	  new_device_id = midi_in;
	  sysex_rec_state = BSL_SYSEX_REC_ID_OK;
      } else {
	// too many bytes or wrong sequence... wait for F7
	sysex_rec_state = BSL_SYSEX_REC_INVALID;
      }
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      if( sysex_rec_state != BSL_SYSEX_REC_ID_OK ) {
	// too many bytes received
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_MORE_BYTES_THAN_EXP);
      } else {
	// program device ID if it has been changed
	if( new_device_id != MIOS32_MIDI_DeviceIDGet() ) {
	  // No EEPROM emulation used here (to save memory), accordingly the device ID can only be changed once!

	  // if device ID has already been programmed, abort here to avoid invalid values!
	  if( MIOS32_MIDI_DeviceIDGet() != 0x00 ) {
	    BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_PROG_ID_NOT_ALLOWED);
	    break;
	  }

	  MIOS32_IRQ_Disable();

	  // FLASH_* routines are part of the STM32 code library
	  FLASH_Unlock();

	  FLASH_Status status;
	  if( (status=FLASH_ProgramHalfWord(BSL_DEVICE_ID_ADDR, new_device_id)) != FLASH_COMPLETE ) {
	    FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	  }

	  FLASH_Lock();	  

	  MIOS32_IRQ_Enable();
	}
	// send acknowledge via old device ID
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, new_device_id);
	// change device ID
	BSL_FetchDeviceIDFromFlash();
	// send acknowledge via new device ID
	BSL_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, new_device_id);
      }
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
    sysex_addr = (sysex_addr << 7) | ((midi_in & 0x7f) << 4);
    if( sysex_rec_state == BSL_SYSEX_REC_A0 )
      sysex_rec_state = BSL_SYSEX_REC_L3;
    else
      ++sysex_rec_state;
  } else if( sysex_rec_state <= BSL_SYSEX_REC_L0 ) {
    sysex_len = (sysex_len << 7) | ((midi_in & 0x7f) << 4);
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
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
// This function sends an upload request
/////////////////////////////////////////////////////////////////////////////
s32 BSL_SYSEX_SendUploadReq(mios32_midi_port_t port)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send 0x01 to request code upload
  *sysex_buffer_ptr++ = 0x01;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
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
  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // "write mem" command (so that dump could be sent back to overwrite the memory w/o modifications)
  *sysex_buffer_ptr++ = 0x02;

  // send 32bit address (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (addr >> 25) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 18) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >> 11) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (addr >>  4) & 0x7f;

  // send 32bit range (divided by 16) in 7bit format
  checksum += *sysex_buffer_ptr++ = (len >> 25) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 18) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >> 11) & 0x7f;
  checksum += *sysex_buffer_ptr++ = (len >>  4) & 0x7f;

  // send memory content in scrambled format (8bit values -> 7bit values)
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
	checksum += *sysex_buffer_ptr++ = (value7 << (7-bit_ctr7));
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
// We expect that address and length are aligned to 4
/////////////////////////////////////////////////////////////////////////////
static s32 BSL_SYSEX_WriteMem(u32 addr, u32 len, u8 *buffer)
{
  // check for alignment
  if( (addr % 4) || (len % 4) )
    return -MIOS32_MIDI_SYSEX_DISACK_ADDR_NOT_ALIGNED;

  // check for flash memory range
  if( addr >= FLASH_START_ADDR && addr <= FLASH_END_ADDR ) {
    // FLASH_* routines are part of the STM32 code library
    FLASH_Unlock();

    FLASH_Status status;
    int i;
    for(i=0; i<len; addr+=2, i+=2) {
      MIOS32_IRQ_Disable();
      if( (addr % FLASH_PAGE_SIZE) == 0 ) {
	if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE ) {
	  FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	  MIOS32_IRQ_Enable();
	  return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
	}
      }

      if( (status=FLASH_ProgramHalfWord(addr, buffer[i+0] | ((u16)buffer[i+1] << 8))) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	MIOS32_IRQ_Enable();
	return -MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED;
      }
      MIOS32_IRQ_Enable();

      // TODO: verify programmed code
    }

    return 0; // no error
  }

  // check for SRAM memory range
  if( addr >= SRAM_START_ADDR && addr <= SRAM_END_ADDR ) {

    // transfer buffer into SRAM
    memcpy((u8 *)addr, (u8 *)buffer, len);

    return 0; // no error
  }

  // invalid address
  return -MIOS32_MIDI_SYSEX_DISACK_WRONG_ADDR_RANGE;
}

