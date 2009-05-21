// $Id$
/*
 * OSC->MIDI Proxy
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
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <mios32.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 3


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define MIDI_OUTPUT_BUFFER_SIZE 0

#define PMIDI_DRIVER_INFO NULL
#define PMIDI_TIME_PROC   ((long (*)(void *)) Pt_Time)
#define PMIDI_TIME_INFO   NULL

#define STRING_MAX 80 // used for console input 

#define OSC_BUFFER_MAX 1024 // OSC datagram buffer size)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static int osc_server_socket;

PmStream *midi_out;
PmStream *midi_in;
int midi_out_port;
int midi_in_port;

volatile int terminated;

struct sockaddr_in remote_address_info;


/////////////////////////////////////////////////////////////////////////////
// Search Tree for OSC Methods (used by MIOS32_OSC_ParsePacket())
/////////////////////////////////////////////////////////////////////////////

static s32 OSC_SERVER_Method_MIDI(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // only checking for MIDI event ('m') -- we could support more parameter types here!
  if( osc_args->arg_type[0] == 'm' ) {
    PmEvent e;
    mios32_midi_package_t p = MIOS32_OSC_GetMIDI(osc_args->arg_ptr[0]);
    e.timestamp = 0;
    e.message = Pm_Message(p.evnt0, p.evnt1, p.evnt2);
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[MIDI_OUT] %02X %02X %02X\n", p.evnt0, p.evnt1, p.evnt2);
#endif
    Pm_Write(midi_out, &e, 1);
  }

  return 0; // no error
}


static mios32_osc_search_tree_t parse_root[] = {
  { "midi", NULL, &OSC_SERVER_Method_MIDI, 0x00000000 },
  { NULL, NULL, NULL, 0 } // terminator
};




/////////////////////////////////////////////////////////////////////////////
// Help functions
/////////////////////////////////////////////////////////////////////////////

// read a number from console
int get_number(char *prompt)
{
  char line[STRING_MAX];
  int n = 0, i;
  printf(prompt);
  while (n != 1) {
    n = scanf("%d", &i);
    fgets(line, STRING_MAX, stdin);
  }

  return i;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called on incoming MIDI events
/////////////////////////////////////////////////////////////////////////////
void receive_poll(PtTimestamp timestamp, void *userData)
{
  mios32_midi_package_t p[8];
  PmEvent event;
  int count;
  int max_bundle = 8;
  int bundle_ix;

  do {
    bundle_ix = 0;
    while( (bundle_ix < max_bundle) && (count=Pm_Read(midi_in, &event, 1)) ) {
      if( count == 1 ) {
	p[bundle_ix].evnt0 = (event.message >>  0) & 0xff;
	p[bundle_ix].evnt1 = (event.message >>  8) & 0xff;
	p[bundle_ix].evnt2 = (event.message >> 16) & 0xff;
	p[bundle_ix].cable = 0;
	p[bundle_ix].cin = p[bundle_ix].evnt0 >> 4;
	
#if DEBUG_VERBOSE_LEVEL >= 2
	printf("[MIDI_IN] %02X %02X %02X\n", p[bundle_ix].evnt0, p[bundle_ix].evnt1, p[bundle_ix].evnt2);
#endif
	++bundle_ix;
      } else {
	printf(Pm_GetErrorText(count));
      }
    }
    
    if( bundle_ix ) {
      u8 num_events = bundle_ix;
      u8 packet[OSC_BUFFER_MAX];
      u8 *end_ptr = packet;
      u8 *insert_len_ptr;
      int i;

      mios32_osc_timetag_t timetag;
      timetag.seconds = 0;
      timetag.fraction = 0;

      end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
      end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
      for(i=0; i<num_events; ++i) {
	insert_len_ptr = end_ptr; // remember this address - we will insert the length later
	end_ptr += 4;
	end_ptr = MIOS32_OSC_PutString(end_ptr, "/midi");
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",m");
	end_ptr = MIOS32_OSC_PutMIDI(end_ptr, p[i]);
	MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));
      }

      size_t len = (size_t)(end_ptr-packet);
      size_t size;
      if( (size=sendto(osc_server_socket, packet, len, 0, &remote_address_info, sizeof(remote_address_info))) != len ) {
	printf("ERROR while sending OSC packet (unexpected len: %d, expected %d)\n", size, len);
      }
    }
  } while( bundle_ix );
}

/////////////////////////////////////////////////////////////////////////////
// This function handles OSC and MIDI messages
/////////////////////////////////////////////////////////////////////////////
static void proxy_handler(void)
{
  char buffer[OSC_BUFFER_MAX];
  s32 status;

  // It is recommended to start timer before PortMidi
  Pt_Start(1, receive_poll, 0);
  status = Pm_OpenInput(&midi_in, midi_in_port, NULL, 512, NULL, NULL);
  printf("MIDI IN opened.\n");

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
  printf("MIDI OUT opened.\n");

  printf("Proxy is running!\n");
  terminated = 0;
  while( !terminated ) {
    int size;

    if( (size=recv(osc_server_socket, buffer, OSC_BUFFER_MAX, 0)) > 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      printf("### Received %d bytes\n", size);
      // MIOS32_MIDI_SendDebugHexDump(buffer, size);
#endif
      status = MIOS32_OSC_ParsePacket((u8 *)buffer, size, parse_root);
      if( status < 0 ) {
	printf("Invalid OSC packet, status %d\n", status);
      }	
    }
  }

  // close device (this not explicitly needed in most implementations)
  Pm_Close(midi_out);
  Pm_Close(midi_in);


  Pm_Terminate();
}




int main(int argc, char* argv[])
{
  int default_in;
  int default_out;
  int i = 0, n = 0;
  char line[STRING_MAX];
  char *remote_host_name;
    
  struct sockaddr_in host_address_info;
  int host_port;
  int status;
  struct hostent *remote_host_info;
  long remote_address;

  if(argc < 3) {
    printf("SYNTAX: osc_midi_proxy <remote-host> <port>\n");
    return 0;
  } else {
    remote_host_name = argv[1];
    host_port = atoi(argv[2]);
  }

  // make a socket
  if( (osc_server_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
    printf("ERROR: couldn't make a socket!\n");
    return 1;
  }

  /* get IP address from name */
  remote_host_info = gethostbyname(remote_host_name);

  /* fill address struct */
  memcpy(&remote_address, remote_host_info->h_addr, remote_host_info->h_length);
  remote_address_info.sin_addr.s_addr = remote_address;
  remote_address_info.sin_port        = htons(host_port);
  remote_address_info.sin_family      = AF_INET;

  printf("Connecting to %s on port %d\n", remote_host_name, host_port);

  host_address_info.sin_addr.s_addr=INADDR_ANY;
  host_address_info.sin_port=htons(host_port);
  host_address_info.sin_family=AF_INET;

  if( (status=bind(osc_server_socket, (struct sockaddr*)&host_address_info, sizeof(host_address_info))) < 0 ) {
    printf("ERROR: couldn't connect to host (status %d) - try another port!\n", status);
    return 1;
  }

  /* list device information */
  default_in = Pm_GetDefaultInputDeviceID();
  default_out = Pm_GetDefaultOutputDeviceID();

  // determine which output device to use
  for(i = 0; i < Pm_CountDevices(); i++) {
    char *deflt;
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if( info->output ) {
      deflt = (i == default_out ? "default " : "");
      printf("%d: %s, %s (%soutput)\n", i, info->interf, info->name, deflt);
    }
  }
  midi_out_port = get_number("Type output number: ");

  // determine which input device to use
  for(i = 0; i < Pm_CountDevices(); i++) {
    char *deflt;
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if( info->input ) {
      deflt = (i == default_in ? "default " : "");
      printf("%d: %s, %s (%sinput)\n", i, info->interf, info->name, deflt);
    }
  }
  midi_in_port = get_number("Type input number: ");
  
  // start proxy loop
  proxy_handler();
    
  return 0; // no error
}
