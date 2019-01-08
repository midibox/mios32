// $Id$
/*
 * MIDIbox MM V3
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
#include "mm_hwcfg.h"
#include "mm_lcd.h"
#include "mm_sysex.h"
#include "mm_leddigits.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

sysex_state_t sysex_state;

u8 sysex_cmd;

u8 status_digit_received;

/////////////////////////////////////////////////////////////////////////////
// Static definitions
/////////////////////////////////////////////////////////////////////////////
const unsigned char sysex_header[] = { 0xf0, 0x00, 0x01, 0x0f, 0x00, 0x11, 0x00 };


/////////////////////////////////////////////////////////////////////////////
// This function initializes the SysEx handler
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_Init(u32 mode)
{
  sysex_state.ALL = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MM messages
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // only listen to default port
  if( sysex_state.MY_SYSEX && port != MIOS32_MIDI_DefaultPortGet() )
    return 0; // forward package to APP_MIDI_NotifyPackage()

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {

    if( midi_in != sysex_header[sysex_state.CTR] ) {
      // incoming byte doesn't match
      MM_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR == sizeof(sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	MM_SYSEX_Cmd(SYSEX_CMD_STATE_END, midi_in);
      }
      MM_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	MM_SYSEX_Cmd(SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	MM_SYSEX_Cmd(SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  status_digit_received = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Call this function on a timeout
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_TimeOut(mios32_midi_port_t port)
{
  s32 status = 0;

  if( port == MIOS32_MIDI_DefaultPortGet() ) {
    status = MM_SYSEX_CmdFinished();
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function sends the SysEx header if merger disabled
// if force == 1, send the footer regardless of merger state
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_SendResponse(u8 *buffer, u8 len)
{
  int i;
  u8 sysex_buffer[128]; // should be enough?
  int sysex_buffer_ix = 0;

  // send header
  for(i=0; i<sizeof(sysex_header); ++i)
    sysex_buffer[sysex_buffer_ix++] = sysex_header[i];

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
s32 MM_SYSEX_Cmd(u8 cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case 0x10: return MM_SYSEX_Cmd_WriteLCD(cmd_state, midi_in);
    case 0x11: return MM_SYSEX_Cmd_WriteValue(cmd_state, midi_in);
    case 0x12: return MM_SYSEX_Cmd_Digits(cmd_state, midi_in);
  }

  return MM_SYSEX_CmdFinished();
}


/////////////////////////////////////////////////////////////////////////////
// Host application sends characters
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_Cmd_WriteLCD(u8 cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.CURSOR_POS_RECEIVED ) {
	// set cursor
	sysex_state.CURSOR_POS_RECEIVED = 1;
	MM_LCD_Msg_CursorSet(midi_in < 40 ? midi_in : (midi_in - 40 + 0x40));
      } else {
	// print character
	MM_LCD_Msg_PrintChar(midi_in);
      }
      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Host application sends "rotary pointer" values
/////////////////////////////////////////////////////////////////////////////
s32 MM_SYSEX_Cmd_WriteValue(u8 cmd_state, u8 midi_in)
{
  static u8 pointer_pos = 0;

  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.POINTER_TYPE_RECEIVED ) {
	sysex_state.POINTER_TYPE_RECEIVED = 1;
	MM_LCD_PointerInit(midi_in); // init pointer type
      } else if( !sysex_state.POINTER_POS_RECEIVED ) {
	sysex_state.POINTER_POS_RECEIVED = 1;
        pointer_pos = midi_in & 0x07; // store pointer position
      } else {
	MM_LCD_PointerSet(pointer_pos, midi_in); // set pointer value
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
s32 MM_SYSEX_Cmd_Digits(u8 cmd_state, u8 midi_in)
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
	  MM_LEDDIGITS_Set(0, pattern | (midi_in & 0x0f));
	  break;

        case 3:
	  pattern = (midi_in << 4);
	  if( midi_in & (1 << 6) )
	    pattern |= 0x80;
	  break;

        case 4:
	  MM_LEDDIGITS_Set(1, pattern | (midi_in & 0x0f));
	  break;
      }

      break;

    default: // SYSEX_CMD_STATE_END
      break;
  }

  return 0; // no error
}
