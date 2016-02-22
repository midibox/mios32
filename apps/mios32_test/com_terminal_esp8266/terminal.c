// $Id$
/*
 * COM Terminal for ESP8266 WiFi module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
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

#include "app.h"
#include "terminal.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 256


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char midi_line_buffer[STRING_MAX];
static u16 midi_line_ix;

static char com_line_buffer[STRING_MAX];
static u16 com_line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_PrintSystem(void *_output_function);

static s32 TERMINAL_MIDI_Parse(mios32_midi_port_t port, u8 byte);
static s32 TERMINAL_MIDI_ParseLine(char *input, void *_output_function);

static s32 TERMINAL_COM_Parse(mios32_midi_port_t port, char byte);
static s32 TERMINAL_COM_ParseLine(char *input, void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_MIDI_Parse);

  // install the callback function which is called on incoming characters
  // from COM port
  MIOS32_COM_ReceiveCallback_Init(TERMINAL_COM_Parse);

  // clear line buffers
  midi_line_buffer[0] = 0;
  midi_line_ix = 0;

  com_line_buffer[0] = 0;
  com_line_ix = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
// MIDI Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_MIDI_Parse(mios32_midi_port_t port, u8 byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_MIDI_ParseLine(midi_line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
    midi_line_ix = 0;
    midi_line_buffer[midi_line_ix] = 0;
  } else if( midi_line_ix < (STRING_MAX-1) ) {
    midi_line_buffer[midi_line_ix++] = byte;
    midi_line_buffer[midi_line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_MIDI_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // send to ESP8266?
  if( input[0] == '!' ) {
    if( midi_line_ix >= (STRING_MAX-3) ) {
      out("ERROR: string too long!\n");
    } else {
      midi_line_buffer[midi_line_ix++] = '\r';
      midi_line_buffer[midi_line_ix++] = '\n';
      midi_line_buffer[midi_line_ix] = 0;

      // send to ESP8266
      MIOS32_COM_SendString(ESP8266_UART, &midi_line_buffer[1]);
      return 0;
    }
  }

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      out("  !<...>                            directly send string to ESP8266");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "osctest") == 0 ) {
      TERMINAL_SendOscTestMessage(0, 1, 64);
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// System Informations
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_PrintSystem(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  out("Application: " MIOS32_LCD_BOOT_MSG_LINE1);
  out("Copyright: " MIOS32_LCD_BOOT_MSG_LINE2);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// COM Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_COM_Parse(mios32_midi_port_t port, char byte)
{
  if( port != ESP8266_UART )
    return -1; // ignore messages from other UARTs

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_COM_ParseLine(com_line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
    com_line_ix = 0;
    com_line_buffer[com_line_ix] = 0;
  } else if( com_line_ix < (STRING_MAX-1) ) {
    com_line_buffer[com_line_ix++] = byte;
    com_line_buffer[com_line_ix] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// COM Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_COM_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

  if( strncmp(input, "+IPD,", 5) == 0 ) {
    char *str = &input[5];

    u32 pos;
    for(pos=0; str[pos] != 0 && str[pos] != ':'; ++pos);

    if( str[pos] == 0 ) {
      out("ERROR: unexpected response: %s\n", input);
    } else {
      str[pos] = 0;
      s32 len = get_dec(str);
      if( len < 0 ) {
	out("ERROR: unexpected length in: %s\n", input);
      } else {
	str = &str[pos+1];
	MIOS32_MIDI_SendDebugHexDump(str, len);
      }
    }
  } else {
    out(input);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Temporary test to send a OSC message
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_SendOscTestMessage(u8 chn, u8 cc, u8 value)
{
  // create OSC packet
  u8 packet[128];
  u8 *end_ptr = packet;
  char event_path[30];
  sprintf(event_path, "/%d/cc_%d", chn, cc);
  end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
  end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
  end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)value/127.0);
  u32 len = (u32)(end_ptr-packet);

  // send command
  char cmd[50];
  sprintf(cmd, "AT+CIPSEND=%d\r\n", len);
  MIOS32_COM_SendString(ESP8266_UART, cmd);

  MIOS32_DELAY_Wait_uS(2*1000); // TODO: wait for '>' sign, needs a proper communication handler

  MIOS32_COM_SendBuffer(ESP8266_UART, packet, len);
  sprintf(cmd, "\r\n", len);

  return 0; // no error
}
