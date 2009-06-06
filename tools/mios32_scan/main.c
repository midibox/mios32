// $Id$
/*
 * MIOS32 Scan
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
#ifndef WIN32
#include <getopt.h>
#endif
#include <string.h>

#include <mios32.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// MIDI commands and acknowledge reply codes
#define MIDI_SYSEX_DISACK   0x0e
#define MIDI_SYSEX_ACK      0x0f

#define MIDI_OUTPUT_BUFFER_SIZE 0

#define PMIDI_DRIVER_INFO NULL
#define PMIDI_TIME_PROC   ((long (*)(void *)) Pt_Time)
#define PMIDI_TIME_INFO   NULL

#define STRING_MAX 256 // used for console input/output

#define MIDI_IN_MAX 256 // 256 ports should be sufficient?

#define TIMEOUT_PRELOAD 100 // timeout after 100 mS

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

static volatile int num_midi_in_ports;
static PmStream *midi_in_ports[MIDI_IN_MAX];

static volatile int terminated;

static volatile u32 timeout_ctr;
static volatile u8 got_query_response;

static sysex_state_t sysex_state;
static int sysex_port;
static u8 sysex_device_id;
static u8 sysex_cmd;

static volatile query_response[STRING_MAX];


/////////////////////////////////////////////////////////////////////////////
// Help functions
/////////////////////////////////////////////////////////////////////////////

// read a number from console
int get_number(char *prompt)
{
  char line[STRING_MAX];
  int n=0, i;
  printf(prompt);
  while (n != 1) {
    n = scanf_s("%d", &i);
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
  sysex_port = -1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the disacknowledge command
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_SYSEX_Cmd_DisAck(mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 buffer[STRING_MAX];
  static int ix;
  ix = 0;

  switch( cmd_state ) {

    case MIDI_SYSEX_CMD_STATE_BEGIN:
      ix = 0;
      break;

    case MIDI_SYSEX_CMD_STATE_CONT:
      if( ix < (STRING_MAX-1) ) {
	buffer[ix++] = midi_in;
        buffer[ix] = 0;
      }
      break;

    default: // MIDI_SYSEX_CMD_STATE_END
      if( !ix ) {
	printf("ERROR: received empty disacknowledge string!\n");
      } else {
	int i;

	printf("ERROR: received disacknowledge with following code: ");
	for(i=0; i<ix; ++i)
	  printf("%02X ", buffer[i]);
	printf("\n");
      }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the acknowledge command
/////////////////////////////////////////////////////////////////////////////
static s32 MIDI_SYSEX_Cmd_Ack(mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 buffer[STRING_MAX];
  static int ix=0;
  int i;
  switch( cmd_state ) {

    case MIDI_SYSEX_CMD_STATE_BEGIN:
      ix = 0;
      query_response[0] = 0;
      break;

    case MIDI_SYSEX_CMD_STATE_CONT:
      if( ix < (STRING_MAX-1) ) {
		buffer[ix++] = midi_in;
        buffer[ix] = 0;
      }
      break;

    default: // MIDI_SYSEX_CMD_STATE_END
      if( !ix ) {
		printf("ERROR: received empty acknowledge string!\n");
      } else {

	// waiting for query response
	if( timeout_ctr ) {
	  got_query_response = 1;
	  strcpy(query_response, buffer);
	} else {
	  printf("ERROR: received unexpected acknowledge with following code: ");
	  for(i=0; i<ix; ++i)
	    printf("%02X ", buffer[i]);
	  printf("\n");
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
    case MIDI_SYSEX_DISACK:
      MIDI_SYSEX_Cmd_DisAck(cmd_state, midi_in);
      break;
    case MIDI_SYSEX_ACK:
      MIDI_SYSEX_Cmd_Ack(cmd_state, midi_in);
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
  int in_port;
  int i;
  u8 midi_bytes[4];
  u8 midi_in;

  // handle timeout counter
  if( timeout_ctr )
    --timeout_ctr;

  // check for incoming MIDI event
  for(in_port=0; in_port<num_midi_in_ports; ++in_port) {
    if( sysex_port == -1 || sysex_port == in_port ) {
      while( (count=Pm_Read(midi_in_ports[in_port], &event, 1)) ) {
    
	if( count == 1 ) {

	  midi_bytes[0] = (event.message >>  0) & 0xff;
	  midi_bytes[1] = (event.message >>  8) & 0xff;
	  midi_bytes[2] = (event.message >> 16) & 0xff;
	  midi_bytes[3] = (event.message >> 24) & 0xff;

#if DEBUG_VERBOSE_LEVEL >= 2
	  printf("[MIDI_IN] %02X %02X %02X %02X\n", midi_bytes[0], midi_bytes[1], midi_bytes[2], midi_bytes[3]);
#endif

	  for(i=0; i<4; ++i) {
	    midi_in = midi_bytes[i];

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
		sysex_port = in_port; // hold port
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
  }
}

/////////////////////////////////////////////////////////////////////////////
// This function scans for MIOS32 devices
/////////////////////////////////////////////////////////////////////////////
static void scan_handler(void)
{
  s32 status;
  int port;
  int query;
  const PmDeviceInfo *info;
  u8 *sysex_buffer_ptr;
  // reset SysEx state
  MIDI_SYSEX_CmdFinished();


  // It is recommended to start timer before PortMidi
  num_midi_in_ports = 0;
  Pt_Start(1, receive_poll, 0);

  for(port=0; port < Pm_CountDevices() && num_midi_in_ports < MIDI_IN_MAX; ++port) {
    info = Pm_GetDeviceInfo(port);
    if( info->input ) {
      printf("MIDI IN '%s: %s' opened.\n", info->interf, info->name);

      status = Pm_OpenInput(&midi_in_ports[num_midi_in_ports], port, NULL, 512, NULL, NULL);

      if( status ) {
	printf(Pm_GetErrorText(status));
      } else {
	++num_midi_in_ports;
      }
    }
  }

  if( !num_midi_in_ports ) {
    printf("ERROR: no MIDI IN port found!\n");
    exit(1);
    return;
  }

  printf("Starting scan for MIOS32 devices\n");

  // scan for MIOS32 devices and request query messages
  for(port=0; port<Pm_CountDevices(); ++port) {
    char msg[100]; // at least + 5 for header + 3 for command + F7

    info = Pm_GetDeviceInfo(port);
    if( info->output ) {
      int i;
      for(i=0; i<80; ++i) { printf("-"); } printf("\n");
      printf("Scanning [%2d] %s: %s\n", port, info->interf, info->name);

      Pm_OpenOutput(&midi_out,
		    port, 
		    PMIDI_DRIVER_INFO,
		    MIDI_OUTPUT_BUFFER_SIZE, 
		    NULL, // (latency == 0 ? NULL : PMIDI_TIME_PROC),
		    NULL, // (latency == 0 ? NULL : PMIDI_TIME_INFO), 
		    0);   //  latency);

      for(query=1; query<=9; ++query) {
	sysex_buffer_ptr = (u8 *)msg;
	for(i=0; i<sizeof(midi_sysex_header); ++i)
	  *sysex_buffer_ptr++ = midi_sysex_header[i];

	// device ID
	*sysex_buffer_ptr++ = sysex_device_id;

	// query command
	*sysex_buffer_ptr++ = 0x00;

	// query request
	*sysex_buffer_ptr++ = query;

	// EOS
	*sysex_buffer_ptr++ = 0xf7;

	got_query_response = 0;
	timeout_ctr = TIMEOUT_PRELOAD;

	Pm_WriteSysEx(midi_out, 0, msg);

	while( timeout_ctr && !got_query_response );

	if( !got_query_response ) {
	  printf("No response on query\n");
	  break;
	} else {
	  switch( query ) {
	    case 1: printf("Operating System: %s\n", query_response); break;
	    case 2: printf("Board: %s\n", query_response); break;
	    case 3: printf("Core Family: %s\n", query_response); break;
	    case 4: printf("Chip ID: 0x%s\n", query_response); break;
	    case 5: printf("Serial Number: #%s\n", query_response); break;
	    case 6: printf("Flash Memory Size: %s bytes\n", query_response); break;
	    case 7: printf("RAM Size: %s bytes\n", query_response); break;
	    case 8: printf("%s\n", query_response); break;
	    case 9: printf("%s\n", query_response); break;
	    default: printf("Response #%d: %s\n", query, query_response);
	  }
	}
      }

      Pm_Close(midi_out);
    }
  }

  Pm_Terminate();
}


int usage(char *program_name)
{
  printf("SYNTAX: %s [--device_id <sysex-device-id>]\n", program_name);
  return 1;
}


int main(int argc, char* argv[])
{
  int ch;
  char *program_name;
  program_name= argv[0];

  sysex_device_id = 0x00;
#ifndef WIN32
  // options descriptor
  const struct option longopts[] = {
    { "device_id", required_argument, NULL, 'd' },
    { NULL,    0,                 NULL,  0 }
  };

  while( (ch=getopt_long(argc, argv, "io", longopts, NULL)) != -1 )
    switch( ch ) {
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
  // start scan
  scan_handler();

  return 0; // no error
}
