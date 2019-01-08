// $Id$
/*
 * MIOS Terminal
 * See README.txt for details
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

#include <portmidi.h>
#include <porttime.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32 // Windows does not have getopt() functionality!
#include <getopt.h>
#else
#include <getopt_long.h>
#endif
#include <string.h>

#include <mios32.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Fixes for WIN32 option (really no other solution?)
/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#define scanf scanf_s
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// MIDI commands and acknowledge reply codes
#define MIDI_SYSEX_DEBUG    0x0d
#define MIDI_SYSEX_DISACK   0x0e
#define MIDI_SYSEX_ACK      0x0f

#define MIDI_OUTPUT_BUFFER_SIZE 0

#define PMIDI_DRIVER_INFO NULL
#define PMIDI_TIME_PROC   ((long (*)(void *)) Pt_Time)
#define PMIDI_TIME_INFO   NULL

#define STRING_MAX 256 // used for terminal input/output

const u8 midi_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x32 };


/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  MIDI_SYSEX_CMD_STATE_BEGIN,
  MIDI_SYSEX_CMD_STATE_CONT,
  MIDI_SYSEX_CMD_STATE_END
} midi_sysex_cmd_state_t;

typedef union {
  struct {
    unsigned ALL:8;
  };

  struct {
    unsigned CTR:3;
    unsigned CMD:1;
    unsigned MY_SYSEX:1;
  };
} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static PmStream *midi_out;
static PmStream *midi_in;
static int midi_out_port;
static int midi_in_port;

static volatile int terminated;

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;

static volatile PtTimestamp current_timestamp;


/////////////////////////////////////////////////////////////////////////////
// Help functions
/////////////////////////////////////////////////////////////////////////////

// read a number from terminal
int get_number(char *prompt)
{
  char line[STRING_MAX];
  int n, i;
  n=0;
  printf(prompt);
  while (n != 1) {
    n = scanf("%d", &i);
    fgets(line, STRING_MAX, stdin);
  }

  return i;
}



/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the debug command
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_SYSEX_Cmd_Debug(mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 str_buffer[STRING_MAX];
  static int str_ix = 0;

  switch( cmd_state ) {

    case MIDI_SYSEX_CMD_STATE_BEGIN:
      str_ix = 0;
      break;

    case MIDI_SYSEX_CMD_STATE_CONT:
      if( str_ix < (STRING_MAX-1) ) {
	str_buffer[str_ix++] = midi_in;
	str_buffer[str_ix+1] = 0;
      }
      break;

    default: // MIDI_SYSEX_CMD_STATE_END
      if( !str_ix ) {
	printf("ERROR: empty debug request!\n");
      } else {
	if( str_buffer[0] == 0x00 ) {
	  // ignore input string command (no echo)
	} else if( str_buffer[0] == 0x40 ) {
	  str_buffer[0] = ' '; // replace command identifier (string: 0x40) by space
	  --str_ix;
	  while( str_ix >= 0 && (str_buffer[str_ix] == '\n' || str_buffer[str_ix] == '\r') ) {
	    str_buffer[str_ix] = 0;
	    --str_ix;
	  }
	  printf("%014u ms |%s\n", current_timestamp, str_buffer);
	} else {
	  printf("ERROR: unknown command 0x%02x, received %d bytes", str_buffer[0], str_ix);
	}
      }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_SYSEX_Cmd(mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case 0x0d:
      MIDI_SYSEX_Cmd_Debug(cmd_state, midi_in);
      break;
    default:
      // unknown command
      MIDI_SYSEX_CmdFinished();      
  }

  return 0; // no error
}





/////////////////////////////////////////////////////////////////////////////
// This function is called periodically to check for incoming MIDI events
/////////////////////////////////////////////////////////////////////////////
void receive_poll(PtTimestamp timestamp, void *userData)
{
  PmEvent event;
  int count;
  int i;

  while( (count=Pm_Read(midi_in, &event, 1)) ) {
    
    if( count == 1 ) {
      u8 midi_bytes[4];

      current_timestamp = timestamp;
      midi_bytes[0] = (event.message >>  0) & 0xff;
      midi_bytes[1] = (event.message >>  8) & 0xff;
      midi_bytes[2] = (event.message >> 16) & 0xff;
      midi_bytes[3] = (event.message >> 24) & 0xff;

#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_IN] %02X %02X %02X %02X\n", midi_bytes[0], midi_bytes[1], midi_bytes[2], midi_bytes[3]);
#endif

      for(i=0; i<4; ++i) {
		u8 midi_in = midi_bytes[i];

	// ignore realtime messages (see MIDI spec - realtime messages can
	// always be injected into events/streams, and don't change the running status)
	if( midi_in >= 0xf8 )
	  continue;

	// branch depending on state
	if( !sysex_state.MY_SYSEX ) {
	  if( (sysex_state.CTR < sizeof(midi_sysex_header) && midi_in != midi_sysex_header[sysex_state.CTR]) ||
	      (sysex_state.CTR == sizeof(midi_sysex_header) && midi_in != sysex_device_id) ) {
	    // incoming byte doesn't match
	    MIDI_SYSEX_CmdFinished();
	  } else {
	    if( ++sysex_state.CTR > sizeof(midi_sysex_header) ) {
	      // complete header received, waiting for data
	      sysex_state.MY_SYSEX = 1;
	    }
	  }
	} else {
	  // check for end of SysEx message or invalid status byte
	  if( midi_in >= 0x80 ) {
	    if( midi_in == 0xf7 && sysex_state.CMD ) {
	      MIDI_SYSEX_Cmd(MIDI_SYSEX_CMD_STATE_END, midi_in);
	    }
	    MIDI_SYSEX_CmdFinished();
	  } else {
	    // check if command byte has been received
	    if( !sysex_state.CMD ) {
	      sysex_state.CMD = 1;
	      sysex_cmd = midi_in;
	      MIDI_SYSEX_Cmd(MIDI_SYSEX_CMD_STATE_BEGIN, midi_in);
	    }
	    else
	      MIDI_SYSEX_Cmd(MIDI_SYSEX_CMD_STATE_CONT, midi_in);
	  }
	}
      }
    } else {
      printf(Pm_GetErrorText(count));
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the MIOS terminal
/////////////////////////////////////////////////////////////////////////////
static void terminal_handler(void)
{
  s32 status;
  int i;
  u8 *sysex_buffer_ptr;
  u8 *line_ptr;
  
  // reset SysEx state
  MIDI_SYSEX_CmdFinished();

  // It is recommended to start timer before PortMidi
  Pt_Start(1, receive_poll, 0);
  status = Pm_OpenInput(&midi_in, midi_in_port, NULL, 512, NULL, NULL);
  {
    const PmDeviceInfo *info = Pm_GetDeviceInfo(midi_in_port);
    printf("MIDI IN '%s: %s' opened.\n", info->interf, info->name);
  }

  if( status ) {
    printf(Pm_GetErrorText(status));
    Pt_Stop();
    exit(1);
    return;
  }

  // open output device
  Pm_OpenOutput(&midi_out,
		midi_out_port, 
		PMIDI_DRIVER_INFO,
		MIDI_OUTPUT_BUFFER_SIZE, 
		NULL, // (latency == 0 ? NULL : PMIDI_TIME_PROC),
		NULL, // (latency == 0 ? NULL : PMIDI_TIME_INFO), 
		0);   //  latency);
  {
    const PmDeviceInfo *info = Pm_GetDeviceInfo(midi_out_port);
    printf("MIDI OUT '%s: %s' opened.\n", info->interf, info->name);
  }

  printf("MIOS Terminal is running!\n");
  terminated = 0;

  // handle keyboard input
  while( !terminated ) {
    char line[STRING_MAX];
    char msg[STRING_MAX + 20]; // at least + 5 for header + 3 for command + F7
    fgets(line, STRING_MAX, stdin);

    sysex_buffer_ptr = (u8 *)msg;
    for(i=0; i<sizeof(midi_sysex_header); ++i)
      *sysex_buffer_ptr++ = midi_sysex_header[i];

    // device ID
    *sysex_buffer_ptr++ = sysex_device_id;

    // debug message
    *sysex_buffer_ptr++ = MIDI_SYSEX_DEBUG;

    // command identifier
    *sysex_buffer_ptr++ = 0x00; // input string

    // search end of string and determine length
    line_ptr = (u8 *)line;
    for(i=0; i<STRING_MAX && (*line_ptr != 0); ++i)
      *sysex_buffer_ptr++ = (*line_ptr++) & 0x7f; // ensure that MIDI protocol won't be violated

    *sysex_buffer_ptr++ = 0xf7;

    Pm_WriteSysEx(midi_out, 0, msg);
  }

  // close device (this not explicitly needed in most implementations)
  Pm_Close(midi_out);
  Pm_Close(midi_in);


  Pm_Terminate();
}


int usage(char *program_name)
{
  printf("SYNTAX: %s [--in <in-port-number>] [--out <out-port-number>] [--device_id <sysex-device-id>]\n", program_name);
  return 1;
}


int main(int argc, char* argv[])
{
  int default_in;
  int opt_in;
  int default_out;
  int opt_out;
  int i, n;
  int ch;
  char *program_name;
  const struct option longopts[] = {
    { "in",        required_argument, NULL, 'i' },
    { "out",       required_argument, NULL, 'o' },
    { "device_id", required_argument, NULL, 'd' },
    { NULL,    0,                 NULL,  0 }
  };
  program_name = argv[0];
  opt_in = -1;
  opt_out = -1;
  i=0;
  n=0;
  sysex_device_id = 0x00;
#ifndef WIN32 // Remove command line processing getopt() as windows does not have it!
  // options descriptor


  while( (ch=getopt_long(argc, argv, "io", longopts, NULL)) != -1 )
    switch( ch ) {
      case 'i':
	opt_in = atoi(optarg);
	break;
      case 'o':
	opt_out = atoi(optarg);
	break;
      case 'd':
	sysex_device_id = atoi(optarg);
	if( sysex_device_id >= 0x80 ) {
	  printf("ERROR: SysEx Device ID must be between 0 and 127!\n");
	}
	break;
    default:
      return usage(program_name);
    }

  argc -= optind;
  argv += optind;
#endif

  // list device information
  default_in = Pm_GetDefaultInputDeviceID();
  default_out = Pm_GetDefaultOutputDeviceID();

  if( opt_in >= 0 ) {
    midi_in_port = opt_in;
  } else {
    // determine which input device to use
    for(i = 0; i < Pm_CountDevices(); i++) {
      char *deflt;
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if( info->input ) {
	deflt = (i == default_in ? "default " : "");
	printf("[%2d] %s, %s (%sinput)\n", i, info->interf, info->name, deflt);
      }
    }
    midi_in_port = get_number("Type input number: ");
  }

  if( opt_out >= 0 ) {
    midi_out_port = opt_out;
  } else {
    // determine which output device to use
    for(i = 0; i < Pm_CountDevices(); i++) {
      char *deflt;
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if( info->output ) {
	deflt = (i == default_out ? "default " : "");
	printf("[%2d] %s: %s (%soutput)\n", i, info->interf, info->name, deflt);
      }
    }
    midi_out_port = get_number("Type output number: ");
  }

  if( opt_in == -1 || opt_out == -1 ) {
    printf("HINT: next time you could select the MIDI In/Out port from command line with: %s ", program_name);
    for(i=0; i<argc; ++i)
      printf("%s ", argv[i]);
    printf("--in %d --out %d\n", midi_in_port, midi_out_port);
  }

  // start terminal
  terminal_handler();

  return 0; // no error
}
