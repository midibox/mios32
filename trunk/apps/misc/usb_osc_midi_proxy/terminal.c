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

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "uip_terminal.h"
#include "midimon.h"
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
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 )
    return 0;

  return -1;
}


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

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  system:                           print system info");
      out("  set midimon <on|off>:             enables/disables the MIDI monitor");
      out("  set midimon_filter <on|off>:      enables/disables MIDI monitor filters");
      out("  set midimon_tempo <on|off>:       enables/disables the tempo display");
      UIP_TERMINAL_Help(_output_function);
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
	if( strcmp(parameter, "midimon") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_ActiveSet(on_off);
	    out("MIDI Monitor %s!", MIDIMON_ActiveGet() ? "enabled" : "disabled");
	  }
	} else if( strcmp(parameter, "midimon_filter") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_FilterActiveSet(on_off);
	    out("MIDI Monitor Filter %s!", MIDIMON_FilterActiveGet() ? "enabled" : "disabled");
	  }
	} else if( strcmp(parameter, "midimon_tempo") == 0 ) {
	  s32 on_off = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    on_off = get_on_off(parameter);

	  if( on_off < 0 ) {
	    out("Expecting 'on' or 'off'!");
	  } else {
	    MIDIMON_TempoActiveSet(on_off);
	    out("MIDI Monitor Tempo Display %s!", MIDIMON_TempoActiveGet() ? "enabled" : "disabled");
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

  out("MIDI Monitor: %s", MIDIMON_ActiveGet() ? "enabled" : "disabled");
  out("MIDI Monitor Filters: %s", MIDIMON_FilterActiveGet() ? "enabled" : "disabled");
  out("MIDI Monitor Tempo Display: %s", MIDIMON_TempoActiveGet() ? "enabled" : "disabled");

  return 0; // no error
}
