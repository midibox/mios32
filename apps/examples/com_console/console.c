// $Id$
/*
 * Debugging Console
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

#include "app.h"
#include "console.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 line_buffer[STRING_MAX];
static u16 line_ix;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 CONSOLE_Init(u32 mode)
{
  // install the callback function which is called on incoming characters
  // from COM port
  MIOS32_COM_ReceiveCallback_Init(CONSOLE_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 CONSOLE_Parse(mios32_com_port_t port, u8 byte)
{
  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    // send back command string for debugging
    MIOS32_COM_SendFormattedString(port, "Command: '%s'\n", line_buffer);

    // example for parsing the command:
    char *separators = " \t";
    char *brkt;
    char *parameter;

    if( parameter = strtok_r(line_buffer, separators, &brkt) ) {
      if( strncmp(parameter, "cmd1", 4) == 0 ) {
	  parameter += 4;

	  MIOS32_COM_SendFormattedString(port, "Command 'cmd1' has been parsed with following parameters:\n");

	  char *arg;
	  int num_arg = 0;
	  while( (arg = strtok_r(NULL, separators, &brkt)) ) {
	    ++num_arg;
	    MIOS32_COM_SendFormattedString(port, "Arg #%d: '%s'\n", num_arg, arg);
	  }
	  MIOS32_COM_SendFormattedString(port, "Found %d arguments\n", num_arg);
      } else {
	MIOS32_COM_SendFormattedString(port, "Unknown command - currently only 'cmd1 [<arg1>] ...' is supported. Please try!\n");
      }
    }

    line_ix = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  return 0; // no error
}
