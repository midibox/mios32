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
#include "terminal.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX          100 // recommended size for file transfers via FILE_BrowserHandler()
#define KISSBOX_STRING_MAX  100 // for kissbox messages

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;

static char kissbox_line_buffer[KISSBOX_STRING_MAX];
static u16 kissbox_line_ix;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_PrintSystem(void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Init(u32 mode)
{
  // install the callback function which is called on incoming characters from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  kissbox_line_buffer[0] = 0;
  kissbox_line_ix = 0;

  return 0; // no error
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
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
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

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  kissbox <message>:                send a message to kissbox");
      out("  system:                           print system info");
      out("  reset:                            resets the MIDIbox (!)\n");
      out("  help:                             this page");
    } else if( strcmp(parameter, "kissbox") == 0 ) {
      if( brkt == NULL ) {
	out("SYNTAX: kissbox <message>");
      } else {
	TERMINAL_KissboxSendMsg(brkt);
      }
    } else if( strcmp(parameter, "system") == 0 ) {
      TERMINAL_PrintSystem(_output_function);
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Send a message to KissBox
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_KissboxSendMsg(char *msg)
{
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = 0x1; // reserved in USB MIDI spec, used to send strings to KissBox

  int len;
  for(len=strlen(msg); len > 0; len-=3, msg+=3) {
    p.evnt0 = (len >= 1) ? msg[0] : 0;
    p.evnt1 = (len >= 2) ? msg[1] : 0;
    p.evnt2 = (len >= 3) ? msg[2] : 0;

#if 0
    MIOS32_MIDI_SendDebugMessage("[KISSBOX_MSG] 0x%08x\n", p.ALL);
#endif
    MIOS32_MIDI_SendPackage(SPIM0, p);
  }

  // special case: message length is dividable by 3: we need to send an extra package with 0 to terminate the message
  if( p.evnt2 ) {
    p.evnt0 = 0;
    p.evnt1 = 0;
    p.evnt2 = 0;

#if 0
    MIOS32_MIDI_SendDebugMessage("[KISSBOX_MSG] 0x%08x\n", p.ALL);
#endif
    MIOS32_MIDI_SendPackage(SPIM0, p);
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Receive a response from KissBox
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_KissboxReceiveMsgPackage(mios32_midi_package_t p)
{
  if( p.type != 1 )
    return -1; // no message package

  u8 buffer[3];
  buffer[0] = p.evnt0;
  buffer[1] = p.evnt1;
  buffer[2] = p.evnt2;

  {
    int i;
    for(i=0; i<3; ++i) {
      // end of message string or max string size reached
      if( !buffer[i] || kissbox_line_ix >= (KISSBOX_STRING_MAX-1) ) {
	// -> send to MIOS terminal
	MIOS32_MIDI_SendDebugString(kissbox_line_buffer);

	// reset line buffer
	kissbox_line_buffer[0] = 0;
	kissbox_line_ix = 0;

	// terminate if 0 has been sent
	if( !buffer[i] )
	  break;
      } else {
	kissbox_line_buffer[kissbox_line_ix++] = buffer[i];
	kissbox_line_buffer[kissbox_line_ix] = 0;
      }
    }
  }

  return 0; // no error
}

