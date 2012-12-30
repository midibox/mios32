// $Id$
/*
 * The command/configuration Terminal
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "keyboard.h"
#include "uip_terminal.h"
#include "tasks.h"

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_PrintSystem(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

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
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
//static s32 get_on_off(char *word)
//{
//  if( strcmp(word, "on") == 0 )
//    return 1;
//
//  if( strcmp(word, "off") == 0 )
//    return 0;
//
//  return -1;
//}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( UIP_TERMINAL_ParseLine(input, _output_function) > 0 )
    return 0; // command parsed by UIP Terminal

  if( KEYBOARD_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed by Keyboard Terminal

  if( MIDIMON_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed

  if( MIDI_ROUTER_TerminalParseLine(input, _output_function) > 0 )
    return 0; // command parsed

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      KEYBOARD_TerminalHelp(_output_function);
      out("  set srio_num <1..16>:             max. number of scanned DIN/DOUT registers (currently: %d)", MIOS32_SRIO_ScanNumGet());
      UIP_TERMINAL_Help(_output_function);
      MIDIMON_TerminalHelp(_output_function);
      MIDI_ROUTER_TerminalHelp(_output_function);
      out("  store:                            stores current config as preset");
      out("  restore:                          restores config from preset");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
      out("  exit:                             (telnet only) exits the terminal");
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "store") == 0 ) {
      s32 status = PRESETS_StoreAll();
      if( status >= 0 ) {
	out("Presets stored in internal EEPROM!");
      } else {
	out("ERROR: failed to store presets in internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "restore") == 0 ) {
      s32 status = PRESETS_Init(0);
      if( status >= 0 ) {
	out("Presets restored from internal EEPROM!");
      } else {
	out("ERROR: failed to restore presets from internal EEPROM (status %d)!", status);
      }
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "srio_num") == 0 ) {
	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ) {
	    out("Please specify the number of DIN/DOUTs which should be scanned (1..16)!");
	  } else {
	    int srs = get_dec(parameter);

	    if( srs < 1 || srs > 16 ) {
	      out("Number of DIN/DOUTs should be in the range between 1..16!");
	    } else {
	      MIOS32_SRIO_ScanNumSet(srs);
	      out("%d DINs and DOUTs will be scanned!", MIOS32_SRIO_ScanNumGet());
	    }
	  }
        } else {
          out("Unknown set parameter: '%s'!", parameter);
        }
      } else {
	out("Missing parameter after 'set'!");
      }
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

  MIDIMON_TerminalPrintConfig(out);

  return 0; // no error
}
