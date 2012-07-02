// $Id$
/*
 * MIDIbox LC V2
 * SysEx Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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
#include "lc_hwcfg.h"
#include "lc_lcd.h"
#include "lc_sysex.h"
#include "lc_leddigits.h"
#include "lc_meters.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

sysex_state_t sysex_state;

u8 sysex_cmd;

u8 status_digit_received;

/////////////////////////////////////////////////////////////////////////////
// Static definitions
/////////////////////////////////////////////////////////////////////////////
static const u8 sysex_header[] = { 0xf0, 0x00, 0x00, 0x66 };

static u8 sysex_id;

static u8 serial_number[8];

/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Init(u32 mode)
{
  int i;

  sysex_state.ALL = 0;
#if LC_EMULATION_ID < 0x80
  sysex_id = LC_EMULATION_ID;
#else
  sysex_id = 0;
#endif

  for(i=0; i<8; ++i)
    serial_number[i] = '1' + i;

  // install SysEx parser
  MIOS32_MIDI_SysExCallback_Init(LC_SYSEX_Parser);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MM messages
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // only listen to default port
  if( sysex_state.MY_SYSEX && port != MIOS32_MIDI_DefaultPortGet() )
    return 0; // forward package to APP_MIDI_NotifyPackage()

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {

    // Auto ID feature enabled?
#if LC_EMULATION_ID == 0x80
    if( !sysex_id && sysex_state.CTR == sizeof(sysex_header) && (midi_in == 0x10 || midi_in == 0x14) ) {
      sysex_id = midi_in;
    }
#endif

#if LC_EMULATION_ID == 0x81
    if( !sysex_id && sysex_state.CTR == sizeof(sysex_header) && (midi_in == 0x11 || midi_in == 0x15) ) {
      sysex_id = midi_in;
    }
#endif


    if( (sysex_state.CTR < sizeof(sysex_header) && midi_in != sysex_header[sysex_state.CTR] ) ||
        (sysex_state.CTR == sizeof(sysex_header) && midi_in != sysex_id ) ) {
      // incoming byte doesn't match
      LC_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR == (sizeof(sysex_header) + 1) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	LC_SYSEX_Cmd(SYSEX_CMD_STATE_END, midi_in);
      }
      LC_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	LC_SYSEX_Cmd(SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	LC_SYSEX_Cmd(SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  status_digit_received = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends the SysEx header if merger disabled
// if force == 1, send the footer regardless of merger state
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_SendResponse(u8 *buffer, u8 len)
{
  int i;
  u8 sysex_buffer[128]; // should be enough?
  int sysex_buffer_ix = 0;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

  // add SysEx ID
  sysex_buffer[sysex_buffer_ix++] = sysex_id;

  // send buffer content
  for(i=0; i<len; ++i)
    sysex_buffer[sysex_buffer_ix++] = buffer[i];

  // send footer
  sysex_buffer[sysex_buffer_ix++] = 0xf7;

  // finally send SysEx stream and return error status
  return MIOS32_MIDI_SendSysEx(DEFAULT, (u8 *)sysex_buffer, sysex_buffer_ix);
}


/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case 0x00: return LC_SYSEX_Cmd_Query(cmd_state, midi_in);
    case 0x1a: return LC_SYSEX_Cmd_MCQuery(cmd_state, midi_in);
    case 0x02: return LC_SYSEX_Cmd_HostReply(cmd_state, midi_in);
    case 0x10: return LC_SYSEX_Cmd_WriteMTC(cmd_state, midi_in);
    case 0x11: return LC_SYSEX_Cmd_WriteStatus(cmd_state, midi_in);
    case 0x12: return LC_SYSEX_Cmd_WriteLCD(cmd_state, midi_in);
    case 0x13: return LC_SYSEX_Cmd_VersionReq(cmd_state, midi_in);
    case 0x20: return LC_SYSEX_Cmd_MeterMode(cmd_state, midi_in);
    case 0x21: return LC_SYSEX_Cmd_MeterGMode(cmd_state, midi_in);
  }

  return LC_SYSEX_CmdFinished();
}


/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_Query(u8 cmd_state, u8 midi_in)
{
  s32 status = 0;

  // wait until footer (F7) has been received
  if( cmd_state == SYSEX_CMD_STATE_END ) {
    int i;
    int buffer_ix = 0;
    u8 buffer[32]; // should be enough?

    // send response message
    buffer[buffer_ix++] = 0x01; // Host Connect Query
    for(i=0; i<sizeof(serial_number); ++i)
      buffer[buffer_ix++] = serial_number[i];

    for(i=0; i<4; ++i)
      buffer[buffer_ix++] = 'A' + i;
    
    status |= LC_SYSEX_SendResponse(buffer, buffer_ix);
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_MCQuery(u8 cmd_state, u8 midi_in)
{
  s32 status = 0;

  // wait until footer (F7) has been received
  if( cmd_state == SYSEX_CMD_STATE_END ) {
    int buffer_ix = 0;
    u8 buffer[32]; // should be enough?

    // send response message
    buffer[buffer_ix++] = 0x1b; // Reply
    buffer[buffer_ix++] = 0x42; // dummy byte (seems to work)

    status |= LC_SYSEX_SendResponse(buffer, buffer_ix);
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_HostReply(u8 cmd_state, u8 midi_in)
{
  s32 status = 0;

  // wait until footer (F7) has been received
  if( cmd_state == SYSEX_CMD_STATE_END ) {
    int i;
    int buffer_ix = 0;
    u8 buffer[32]; // should be enough?

    // send response message
    buffer[buffer_ix++] = 0x03; // Host Connect Confirmation
    for(i=0; i<sizeof(serial_number); ++i)
      buffer[buffer_ix++] = serial_number[i];

    status |= LC_SYSEX_SendResponse(buffer, buffer_ix);
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_VersionReq(u8 cmd_state, u8 midi_in)
{
  s32 status = 0;

  // wait until footer (F7) has been received
  if( cmd_state == SYSEX_CMD_STATE_END ) {
    int buffer_ix = 0;
    u8 buffer[32]; // should be enough?

    // send response message
    buffer[buffer_ix++] = 0x14; // Version Number
    buffer[buffer_ix++] = 'V'; // dummy string
    buffer[buffer_ix++] = '1'; // dummy string
    buffer[buffer_ix++] = '.'; // dummy string
    buffer[buffer_ix++] = '4'; // dummy string
    buffer[buffer_ix++] = '2'; // dummy string

    status |= LC_SYSEX_SendResponse(buffer, buffer_ix);
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_WriteMTC(u8 cmd_state, u8 midi_in)
{
  static u8 cursor = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      cursor = 0;
      break;

    case SYSEX_CMD_STATE_CONT:
      LC_LEDDIGITS_MTCSet(9 - cursor, midi_in);
      ++cursor;
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_WriteStatus(u8 cmd_state, u8 midi_in)
{
  static u8 cursor = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      cursor = 0;
      break;

    case SYSEX_CMD_STATE_CONT:
      LC_LEDDIGITS_StatusSet(1 - cursor, midi_in);
      ++cursor;
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_WriteLCD(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.CURSOR_POS_RECEIVED ) {
	// set cursor
	sysex_state.CURSOR_POS_RECEIVED = 1;
	LC_LCD_Msg_CursorSet(midi_in < 56 ? midi_in : (midi_in - 56 + 0x40));
      } else {
	// print character
	LC_LCD_Msg_PrintHost(midi_in);
      }
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_MeterMode(u8 cmd_state, u8 midi_in)
{
  static u8 meter_chn = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.METER_CHN_RECEIVED ) {
	sysex_state.METER_CHN_RECEIVED = 1;
	meter_chn = midi_in;
      } else if( !sysex_state.METER_MODE_RECEIVED ) {
	sysex_state.METER_MODE_RECEIVED = 1;
	if( meter_chn < 8 )
	  LC_METERS_ModeSet(meter_chn, midi_in);
      }
      // else: do nothing
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_MeterGMode(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.METER_MODE_RECEIVED ) {
	sysex_state.METER_MODE_RECEIVED = 1;
	LC_METERS_GlobalModeSet(midi_in);
      }
      // else: do nothing
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

#if 0
/////////////////////////////////////////////////////////////////////////////
// Host application sends "rotary pointer" values
/////////////////////////////////////////////////////////////////////////////
s32 LC_SYSEX_Cmd_WriteValue(u8 cmd_state, u8 midi_in)
{
  static u8 pointer_pos = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.POINTER_TYPE_RECEIVED ) {
	sysex_state.POINTER_TYPE_RECEIVED = 1;
	LC_LCD_PointerInit(midi_in); // init pointer type
      } else if( !sysex_state.POINTER_POS_RECEIVED ) {
	sysex_state.POINTER_POS_RECEIVED = 1;
        pointer_pos = midi_in & 0x07; // store pointer position
      } else {
	LC_LCD_PointerSet(pointer_pos, midi_in); // set pointer value
      }
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

s32 LC_SYSEX_Cmd_Digits(u8 cmd_state, u8 midi_in)
{
  static u8 digit_ctr = 0;
  static u8 pattern = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      digit_ctr = 0;
      break;

    case SYSEX_CMD_STATE_CONT:

      switch( ++digit_ctr ) {
        case 1:
	  pattern = (midi_in << 4);
	  if( midi_in & (1 << 6) )
	    pattern |= 0x80;
	  break;

        case 2:
	  LC_LEDDIGITS_Set(0, pattern | (midi_in & 0x0f));
	  break;

        case 3:
	  pattern = (midi_in << 4);
	  if( midi_in & (1 << 6) )
	    pattern |= 0x80;
	  break;

        case 4:
	  LC_LEDDIGITS_Set(1, pattern | (midi_in & 0x0f));
	  break;
      }

      break;

    default: // SYSEX_CMD_STATE_END
      // nothing to do
      break;
  }

  return 0; // no error
}
#endif
