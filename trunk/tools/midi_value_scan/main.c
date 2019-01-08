// $Id: main.c 597 2009-06-06 19:50:18Z tk $
/*
 * MIDI Value Scan
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
#include <pmutil.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32 // Windows does not have getopt() functionality!
#include <getopt.h>
#else
#include <getopt_long.h>
#endif
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Fixes for WIN32 option (really no other solution?)
/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#define scanf scanf_s
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define MIDI_OUTPUT_BUFFER_SIZE 0

#define PMIDI_DRIVER_INFO NULL
#define PMIDI_TIME_PROC   ((long (*)(void *)) Pt_Time)
#define PMIDI_TIME_INFO   NULL

#define STRING_MAX 256 // used for terminal input/output


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static PmStream *midi_out;
static PmStream *midi_in;
static int midi_out_port;
static int midi_in_port;

static volatile PtTimestamp current_timestamp;

PmQueue *message_queue;


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
// This function is called periodically to check for incoming MIDI events
/////////////////////////////////////////////////////////////////////////////
void receive_poll(PtTimestamp timestamp, void *userData)
{
  PmEvent event;
  int count;
  int i;

  while( (count=Pm_Read(midi_in, &event, 1)) ) {
    
    if( count == 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_IN]  %08X\n", event.message);
#endif
      Pm_Enqueue(message_queue, &event.message);
    } else {
      printf(Pm_GetErrorText(count));
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the scan
/////////////////////////////////////////////////////////////////////////////
static void scan_handler(void)
{
  int i;
  uint8_t *line_ptr;

  // to transfer messages from receive_poll
  if( (message_queue = Pm_QueueCreate(32, sizeof(PmMessage))) == NULL ) {
    printf("ERROR: no memory for queue\n");
    return;
  }

  // It is recommended to start timer before PortMidi
  Pt_Start(1, receive_poll, 0);
  int status = Pm_OpenInput(&midi_in, midi_in_port, NULL, 512, NULL, NULL);
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

  printf("Scan started!\n");

  // CC#16 sweep from 0..127
  {
    uint8_t cc = 16;
    uint8_t received_cc = 17;

    FILE *out;
    if( (out=fopen("cc.dat", "w")) == NULL ) {
      printf("ERROR: failed to open cc.dat\n");
      return;
    }

    FILE *out_ref;
    if( (out_ref=fopen("cc_ref.dat", "w")) == NULL ) {
      printf("ERROR: failed to open cc_ref.dat\n");
      return;
    }

    // send CC#16, espect CC#17 with mapped value
    for(i=0; i<128; ++i) {
      PmMessage message = Pm_Message(0xb0, cc, i);
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_OUT] %08X\n", message);
#endif
      Pm_WriteShort(midi_out, 0, message);

      // wait for response
      PmMessage message_received;
      while( !Pm_Dequeue(message_queue, &message_received) );

      if( ((message ^ message_received) & 0x000000ff) ) {
	printf("WARNING: unexpected message: OUT: %08X IN: %08X\n", message, message_received);
      } else if( (message_received & 0x0000ff00) != (received_cc << 8) ) {
	printf("WARNING: unexpected CC: OUT: CC#%d IN: CC#%d\n", cc, (message_received >> 8) & 0xff);
      } else {
	printf("CC #%d %d -> CC#%d %d\n", (message >> 8) & 0xff, (message >> 16) & 0xff, (message_received >> 8) & 0xff, (message_received >> 16) & 0xff);
	fprintf(out, "%d\t%d\n", (message >> 16) & 0xff, (message_received >> 16) & 0xff);
	fprintf(out_ref, "%d\t%d\n", (message >> 16) & 0xff, (message >> 16) & 0xff);
      }
    }

    fclose(out);
    fclose(out_ref);
  }


  // Pitchbend sweep from 0..16383
  {
    FILE *out;
    if( (out=fopen("pb.dat", "w")) == NULL ) {
      printf("ERROR: failed to open pb.dat\n");
      return;
    }

    FILE *out_ref;
    if( (out_ref=fopen("pb_ref.dat", "w")) == NULL ) {
      printf("ERROR: failed to open pb_ref.dat\n");
      return;
    }

    // send Pitchbender over Chn0, expect returned mapped value at Chn1
    for(i=0; i<16383; ++i) {
      PmMessage message = Pm_Message(0xe0, i & 0x7f, (i >> 7) & 0x7f);
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_OUT] %08X\n", message);
#endif
      Pm_WriteShort(midi_out, 0, message);

      // wait for response
      PmMessage message_received;
      while( !Pm_Dequeue(message_queue, &message_received) );

      if( ((message ^ message_received) & 0x000000f0) ) {
	printf("WARNING: unexpected message: OUT: %08X IN: %08X\n", message, message_received);
      } else if( (message_received & 0x0000000f) != 1 ) {
	printf("WARNING: unexpected channel: OUT: PB Chn%d IN: PB Chn%d\n", message & 0xf, message_received & 0xf);
      } else {
	printf("PB %d -> PB %d\n", i, ((message_received >> 8) & 0xff) | (((message_received >> 16) & 0xff) << 7));
	fprintf(out, "%d\t%d\n", i, ((message_received >> 8) & 0xff) | (((message_received >> 16) & 0xff) << 7));
	fprintf(out_ref, "%d\t%d\n", i, i);
      }
    }

    fclose(out);
    fclose(out_ref);
  }

  // close device (this not explicitly needed in most implementations)
  Pm_Close(midi_out);
  Pm_Close(midi_in);


  Pm_Terminate();
}


int usage(char *program_name)
{
  printf("SYNTAX: %s [--in <in-port-number>] [--out <out-port-number>]\n", program_name);
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
    { NULL,    0,                 NULL,  0 }
  };
  program_name = argv[0];
  opt_in = -1;
  opt_out = -1;
  i=0;
  n=0;
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

  // start scan
  scan_handler();

  return 0; // no error
}
