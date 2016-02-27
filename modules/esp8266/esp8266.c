// $Id$
//! \defgroup ESP8266
//!
//! ESP8266 module driver
//!
//! \{
/* ==========================================================================
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

#include "esp8266.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 256


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mios32_midi_port_t dev_uart;

static char com_line_buffer[STRING_MAX];
static u16 com_line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 ESP8266_COM_Parse(mios32_midi_port_t port, char byte);
static s32 ESP8266_COM_ParseLine(char *input, void *_output_function);


/////////////////////////////////////////////////////////////////////////////
//! Initializes ESP8266 driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // disable UART by default (has to be initialized with ESP8266_InitUart(<uart>)
  dev_uart = 0;

  // clear line buffers
  com_line_buffer[0] = 0;
  com_line_ix = 0;

  // install the callback function which is called on incoming characters
  // from COM port
  MIOS32_COM_ReceiveCallback_Init(ESP8266_COM_Parse);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes the UART which is connected to ESP8266
//! Should be called from Init() during startup after ESP8266_Init(0)
//! (optionally it can be called later - this function will switch a default
//! MIDI port to a COM port)
//! \param[in] port the UART port which should be used
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_InitUart(mios32_midi_port_t port)
{
  // de-initialize UART if it was assigned before
  if( dev_uart )
    ESP8266_DeInitUart();

  // new UART
  dev_uart = port;

  // init default baudrate
  MIOS32_UART_InitPort(dev_uart & 0xf, 115200, MIOS32_BOARD_PIN_MODE_OUTPUT_PP, 0);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! De-Initializes the UART which is connected to ESP8266 and switches back to the
//! default port mode (e.g. MIDI)
//! \return < 0 if deinitialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_DeInitUart(void)
{
  if( dev_uart != 0 )
    return -1; // no UART selected

  // re-init baudrate based on default settings
  MIOS32_UART_InitPortDefault(dev_uart & 0xf);

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
//! COM Parser
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_COM_Parse(mios32_midi_port_t port, char byte)
{
  if( !dev_uart || port != dev_uart )
    return -1; // ignore messages from other UARTs

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    ESP8266_MUTEX_MIDIOUT_TAKE;
    ESP8266_COM_ParseLine(com_line_buffer, MIOS32_MIDI_SendDebugMessage);
    ESP8266_MUTEX_MIDIOUT_GIVE;
    com_line_ix = 0;
    com_line_buffer[com_line_ix] = 0;
  } else if( com_line_ix < (STRING_MAX-1) ) {
    com_line_buffer[com_line_ix++] = byte;
    com_line_buffer[com_line_ix] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! COM Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_COM_ParseLine(char *input, void *_output_function)
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
	MIOS32_MIDI_SendDebugHexDump((u8 *)str, len);
      }
    }
  } else {
    out(input);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a command to the ESP8266 chip
//! \param[in] cmd the command which will be sent (e.g. "AT+RST" to reset the device)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_SendCommand(const char* cmd)
{
  if( !dev_uart ) {
    return -1; // no device selected
  }

  return MIOS32_COM_SendString(dev_uart, cmd);
}


/////////////////////////////////////////////////////////////////////////////
//! Temporary test to send a OSC message
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_SendOscTestMessage(u8 chn, u8 cc, u8 value)
{
  if( !dev_uart )
    return -1; // UART not initialized

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
  ESP8266_SendCommand(cmd);

  MIOS32_DELAY_Wait_uS(2*1000); // TODO: wait for '>' sign, needs a proper communication handler

  MIOS32_COM_SendBuffer(dev_uart, packet, len);
  sprintf(cmd, "\r\n", len);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns help page for implemented terminal commands of this module
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_TerminalHelp(void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;

#if ESP8266_TERMINAL_DIRECT_SEND_CMD
  out("  !<...>                            directly send string to ESP8266");
#endif
  out("  esp8266 reset                     resets the chip");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line
// Returns > 0 if command line matches with UIP terminal commands
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_TerminalParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  // since strtok_r works destructive (separators in *input replaced by NUL), we have to restore them
  // on an unsuccessful call (whenever this function returns < 1)
  int input_len = strlen(input);

#if ESP8266_TERMINAL_DIRECT_SEND_CMD
  // directly send to ESP8266?
  if( input[0] == '!' ) {
    if( !dev_uart ) {
      out("ESP8266 UART not initialized!");
      return 1; // command taken
    }

    u32 len = strlen(input);

    if( len >= (STRING_MAX-3) ) {
      out("ERROR: string too long!\n");
    } else {
      char buffer[STRING_MAX];
      strcpy(buffer, input);
      buffer[len++] = '\r';
      buffer[len++] = '\n';
      buffer[len] = 0;

      // send to ESP8266
      ESP8266_SendCommand(&buffer[1]);
      return 1; // command taken
    }
  }
#endif

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcasecmp(parameter, "esp8266") == 0 ) {
      char *cmd;
      
      if( (cmd = strtok_r(NULL, separators, &brkt)) ) {
	if( strcasecmp(cmd, "reset") == 0 ) {
	  out("Reseting Device");
	  ESP8266_SendCommand("AT+RST\r\n");
	  return 1; // command taken
	} else if( strcmp(cmd, "osctest") == 0 ) {
	  ESP8266_SendOscTestMessage(0, 1, 64);
	} else {
	  out("Unknown esp8266 command: '%s'\n", cmd);
	}
      } else {
	out("Expecting 'esp8266 <cmd>'\n");
      }
      return 1; // command taken
    }
  }

  // restore input line (replace NUL characters by spaces)
  int i;
  char *input_ptr = input;
  for(i=0; i<input_len; ++i, ++input_ptr)
    if( !*input_ptr )
      *input_ptr = ' ';

  return 0; // command not taken
}



//! \}
