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
#include <getopt.h>

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

#define SYSEX_BUFFER_MAX 0x100000 // 1 MB
#define SYSEX_BLOB_LEN_MAX 512 // maximal length of a blob

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static int osc_server_socket;

static PmStream *midi_out;
static PmStream *midi_in;
static int midi_out_port;
static int midi_in_port;

static volatile int terminated;

static struct sockaddr_in remote_address_info;


static unsigned char sysex_buffer_in[SYSEX_BUFFER_MAX];
static unsigned sysex_buffer_in_len;
static unsigned char sysex_buffer_out[SYSEX_BUFFER_MAX];
static unsigned sysex_buffer_out_len;

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

  // MIDI Event
  if( osc_args->arg_type[0] == 'm' ) {
    PmEvent e;
    mios32_midi_package_t p = MIOS32_OSC_GetMIDI(osc_args->arg_ptr[0]);

    e.timestamp = 0;
    e.message = Pm_Message(p.evnt0, p.evnt1, p.evnt2);

#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_OUT] %02X %02X %02X\n", p.evnt0, p.evnt1, p.evnt2);
#endif
      Pm_Write(midi_out, &e, 1);

  } else if( osc_args->arg_type[0] == 'b' ) {
    // SysEx stream is embedded into blob
    int i;
    int send_sysex = 0;
    u32 len = MIOS32_OSC_GetBlobLength(osc_args->arg_ptr[0]);
    u8 *blob = MIOS32_OSC_GetBlobData(osc_args->arg_ptr[0]);

    if( *blob == 0xf0 )
      sysex_buffer_out_len = 0;

    // copy into out buffer
    for(i=0; i<len; ++i, ++blob) {
      if( sysex_buffer_out_len < SYSEX_BUFFER_MAX ) {
	sysex_buffer_out[sysex_buffer_out_len++] = *blob;
	if( *blob == 0xf7 ) {
	  send_sysex = 1;
	  break;
	}
      }
    }

    if( send_sysex ) {
      Pm_WriteSysEx(midi_out, 0, sysex_buffer_out);
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MIDI_OUT] SysEx stream with %u bytes.\n", (unsigned)sysex_buffer_out_len);
#endif
      sysex_buffer_out_len = 0;
    }
  }

  return 0; // no error
}


static s32 OSC_SERVER_Method_Event(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get channel and status nibble
  int evnt0 = method_arg & 0xff;

  // get note
  int note = 60;
  if( osc_args->arg_type[0] == 'i' )
    note = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
  else if( osc_args->arg_type[0] == 'f' )
    note = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 127.0);
  if( note < 0 ) note = 0; else if( note > 127 ) note = 127;

  // get velocity
  int velocity = 127;
  if( osc_args->num_args >= 2 ) {
    if( osc_args->arg_type[1] == 'i' )
      velocity = MIOS32_OSC_GetInt(osc_args->arg_ptr[1]);
    else if( osc_args->arg_type[1] == 'f' )
      velocity = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[1]) * 127.0);
    if( velocity < 0 ) velocity = 0; else if( velocity > 127 ) velocity = 127;
  }

  PmEvent e;
  e.timestamp = 0;
  if( (evnt0 & 0xf0) == 0xc0 || (evnt0 & 0xf0) == 0xd0 ) {
    e.message = Pm_Message(evnt0, note, 0);
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[MIDI_OUT] %02X %02X\n", (unsigned)(e.message>>0)&0xff, (unsigned)(e.message>>8)&0xff);
#endif
  } else {
    e.message = Pm_Message(evnt0, note, velocity);
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[MIDI_OUT] %02X %02X %02X\n", (unsigned)(e.message>>0)&0xff, (unsigned)(e.message>>8)&0xff, (unsigned)(e.message>>16)&0xff);
#endif
  }
  Pm_Write(midi_out, &e, 1);

  return 0; // no error
}

static s32 OSC_SERVER_Method_EventPB(mios32_osc_args_t *osc_args, u32 method_arg)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
#endif

  // we expect at least 1 argument
  if( osc_args->num_args < 1 )
    return -1; // wrong number of arguments

  // get channel and status nibble
  int evnt0 = method_arg & 0xff;

  // get pitchbender value
  int value = 8192;
  if( osc_args->arg_type[0] == 'i' )
    value = MIOS32_OSC_GetInt(osc_args->arg_ptr[0]);
  else if( osc_args->arg_type[0] == 'f' )
    value = (int)(MIOS32_OSC_GetFloat(osc_args->arg_ptr[0]) * 8191.0);
  value += 8192;
  if( value < 0 ) value = 0; else if( value > 16383 ) value = 16383;

  PmEvent e;
  e.timestamp = 0;
  e.message = Pm_Message(evnt0, value & 0x7f, (value >> 7) & 0x7f);
#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[MIDI_OUT] %02X %02X %02X\n", (unsigned)(e.message>>0)&0xff, (unsigned)(e.message>>8)&0xff, (unsigned)(e.message>>16)&0xff);
#endif
  Pm_Write(midi_out, &e, 1);

  return 0; // no error
}


static mios32_osc_search_tree_t parse_event[] = {
  { "note",          NULL, &OSC_SERVER_Method_Event,   0x00000090 }, // bit [7:4] contains status byte
  { "polypressure",  NULL, &OSC_SERVER_Method_Event,   0x000000a0 }, // bit [7:4] contains status byte
  { "cc",            NULL, &OSC_SERVER_Method_Event,   0x000000b0 }, // bit [7:4] contains status byte
  { "programchange", NULL, &OSC_SERVER_Method_Event,   0x000000c0 }, // bit [7:4] contains status byte
  { "aftertouch",    NULL, &OSC_SERVER_Method_Event,   0x000000b0 }, // bit [7:4] contains status byte
  { "pitchbend",     NULL, &OSC_SERVER_Method_EventPB, 0x000000e0 }, // bit [7:4] contains status byte
};

static mios32_osc_search_tree_t parse_root[] = {
  { "midi",  NULL, &OSC_SERVER_Method_MIDI, 0x00000000 },
  { "midi1", NULL, &OSC_SERVER_Method_MIDI, 0x00000000},
  { "midi2", NULL, &OSC_SERVER_Method_MIDI, 0x00000000},
  { "midi3", NULL, &OSC_SERVER_Method_MIDI, 0x00000000},
  { "midi4", NULL, &OSC_SERVER_Method_MIDI, 0x00000000},

  { "1",  parse_event, NULL, 0x00000000}, // bit [0:3] selects MIDI channel
  { "2",  parse_event, NULL, 0x00000001}, // bit [0:3] selects MIDI channel
  { "3",  parse_event, NULL, 0x00000002}, // bit [0:3] selects MIDI channel
  { "4",  parse_event, NULL, 0x00000003}, // bit [0:3] selects MIDI channel
  { "5",  parse_event, NULL, 0x00000004}, // bit [0:3] selects MIDI channel
  { "6",  parse_event, NULL, 0x00000005}, // bit [0:3] selects MIDI channel
  { "7",  parse_event, NULL, 0x00000006}, // bit [0:3] selects MIDI channel
  { "8",  parse_event, NULL, 0x00000007}, // bit [0:3] selects MIDI channel
  { "9",  parse_event, NULL, 0x00000008}, // bit [0:3] selects MIDI channel
  { "10", parse_event, NULL, 0x00000009}, // bit [0:3] selects MIDI channel
  { "11", parse_event, NULL, 0x0000000a}, // bit [0:3] selects MIDI channel
  { "12", parse_event, NULL, 0x0000000b}, // bit [0:3] selects MIDI channel
  { "13", parse_event, NULL, 0x0000000c}, // bit [0:3] selects MIDI channel
  { "14", parse_event, NULL, 0x0000000d}, // bit [0:3] selects MIDI channel
  { "15", parse_event, NULL, 0x0000000e}, // bit [0:3] selects MIDI channel
  { "16", parse_event, NULL, 0x0000000f}, // bit [0:3] selects MIDI channel
  { NULL, NULL, NULL, 0 } // terminator
};




/////////////////////////////////////////////////////////////////////////////
// Sends a SysEx bundle
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_SendSysEx(u8 osc_port, u8 *stream, u32 count)
{
  u32 offset;
  u8 packet[OSC_BUFFER_MAX];
  u8 *end_ptr;
  u8 *insert_len_ptr;

  mios32_osc_timetag_t timetag;
  timetag.seconds = 0;
  timetag.fraction = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[MIDI_IN] SysEx stream with %u bytes.\n", (unsigned)count);
#endif

  for(offset=0; offset<count; offset+=SYSEX_BLOB_LEN_MAX) {
    end_ptr = packet;
    end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
    end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
    insert_len_ptr = end_ptr; // remember this address - we will insert the length later
    end_ptr += 4;
    char midi_path[8];
    sprintf(midi_path, "/midi%d", osc_port+1);
    end_ptr = MIOS32_OSC_PutString(end_ptr, midi_path);
    end_ptr = MIOS32_OSC_PutString(end_ptr, ",b");

    // determine length of blob
    u32 blob_len = count - offset;
    if( blob_len > SYSEX_BLOB_LEN_MAX )
      blob_len = SYSEX_BLOB_LEN_MAX;

    // add blob to packet
    end_ptr = MIOS32_OSC_PutBlob(end_ptr, (u8 *)&stream[offset], blob_len);

    // insert length of OSC packet
    MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));

    // send OSC packet
    size_t len = (size_t)(end_ptr-packet);
    size_t size;
    if( (size=sendto(osc_server_socket, packet, len, 0, (const struct sockaddr *)&remote_address_info, sizeof(remote_address_info))) != len ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("ERROR while sending OSC packet (unexpected len: %u, expected %u)\n", (unsigned)size, (unsigned)len);
#endif
      return -1; // transfer error
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends one or more MIDI events
/////////////////////////////////////////////////////////////////////////////
s32 OSC_SERVER_SendMIDIEvents(u8 osc_port, mios32_midi_package_t *p, u32 count)
{
  u8 packet[OSC_BUFFER_MAX];
  u8 *end_ptr = packet;
  u8 *insert_len_ptr;
  int i;

  mios32_osc_timetag_t timetag;
  timetag.seconds = 0;
  timetag.fraction = 0;

  end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
  end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
  for(i=0; i<count; ++i) {
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[MIDI_IN] %02X %02X %02X\n", p[i].evnt0, p[i].evnt1, p[i].evnt2);
#endif

    insert_len_ptr = end_ptr; // remember this address - we will insert the length later
    end_ptr += 4;
    char midi_path[8];
    sprintf(midi_path, "/midi%d", osc_port+1);
    end_ptr = MIOS32_OSC_PutString(end_ptr, midi_path);
    end_ptr = MIOS32_OSC_PutString(end_ptr, ",m");
    end_ptr = MIOS32_OSC_PutMIDI(end_ptr, p[i]);
    MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));
  }

  size_t len = (size_t)(end_ptr-packet);
  size_t size;
  if( (size=sendto(osc_server_socket, packet, len, 0, (const struct sockaddr *)&remote_address_info, sizeof(remote_address_info))) != len ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("ERROR while sending OSC packet (unexpected len: %u, expected %u)\n", (unsigned)size, (unsigned)len);
#endif
    return -1; // transfer error
  }

  return 0; // no error
}


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
// This function is called periodically to check for incoming MIDI events
/////////////////////////////////////////////////////////////////////////////
void receive_poll(PtTimestamp timestamp, void *userData)
{
  u8 osc_port = 0; // always send to /midi1
  mios32_midi_package_t p[8];
  PmEvent event;
  int count;
  int max_bundle = 8;
  int bundle_ix;
  int sysex_transfer = 0;

  do {
    bundle_ix = 0;
    while( (bundle_ix < max_bundle) && (count=Pm_Read(midi_in, &event, 1)) ) {
      if( count == 1 ) {
	u8 evnt0 = (event.message >>  0) & 0xff;
	u8 evnt1 = (event.message >>  8) & 0xff;
	u8 evnt2 = (event.message >> 16) & 0xff;
	u8 evnt3 = (event.message >> 24) & 0xff;

	if( sysex_transfer || evnt0 == 0xf0 ) {
	  // (ongoing) sysex transfer
	  // buffer sysex data, send it in special OSC packets once 0xf7 has been received
	  sysex_transfer = 1;
	  if( evnt0 == 0xf0 ) {
	    sysex_buffer_in_len = 0;
	  }
	  // ignore too long sysex streams
	  if( sysex_buffer_in_len < (SYSEX_BUFFER_MAX-4) ) {
	    sysex_buffer_in[sysex_buffer_in_len++] = evnt0;
	    if( evnt0 != 0xf7 ) {
	      sysex_buffer_in[sysex_buffer_in_len++] = evnt1;
	      if( evnt1 != 0xf7 ) {
		sysex_buffer_in[sysex_buffer_in_len++] = evnt2;
		if( evnt2 != 0xf7 ) {
		  sysex_buffer_in[sysex_buffer_in_len++] = evnt3;
		}
	      }
	    }

	    // sysex ends?
	    if( evnt0 == 0xf7 || evnt1 == 0xf7 || evnt2 == 0xf7 || evnt3 == 0xf7 ) {
	      OSC_SERVER_SendSysEx(osc_port, sysex_buffer_in, sysex_buffer_in_len);
	      sysex_buffer_in_len = 0;
	      sysex_transfer = 0;
	    }
	  }
	} else {
          sysex_transfer = 0;
	  p[bundle_ix].evnt0 = evnt0;
	  p[bundle_ix].evnt1 = evnt1;
	  p[bundle_ix].evnt2 = evnt2;
	  p[bundle_ix].cable = 0;
	  p[bundle_ix].cin = p[bundle_ix].evnt0 >> 4;
       	  ++bundle_ix;
	}
      } else {
	printf(Pm_GetErrorText(count));
      }
    }
    
    if( bundle_ix )
      OSC_SERVER_SendMIDIEvents(osc_port, p, bundle_ix);

  } while( bundle_ix );
}

/////////////////////////////////////////////////////////////////////////////
// This function handles OSC and MIDI messages
/////////////////////////////////////////////////////////////////////////////
static void proxy_handler(void)
{
  char buffer[OSC_BUFFER_MAX];
  s32 status;

  // clear sysex buffer
  sysex_buffer_in_len = 0;
  sysex_buffer_out_len = 0;

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

  // start proxy
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
	printf("Invalid OSC packet, status %d\n", (int)status);
      }	
    }
  }

  // close device (this not explicitly needed in most implementations)
  Pm_Close(midi_out);
  Pm_Close(midi_in);


  Pm_Terminate();
}


int usage(char *program_name)
{
  printf("SYNTAX: %s <remote-host> <remote-port> [<local-port>] [--in <in-port-number>] [--out <out-port-number>]\n", program_name);
  return 1;
}


int main(int argc, char* argv[])
{
  int default_in;
  int opt_in = -1;
  int default_out;
  int opt_out = -1;
  int i = 0;
  char *remote_host_name;
  char *program_name = argv[0];
    
  struct sockaddr_in host_address_info;
  int remote_port;
  int local_port;
  int status;
  struct hostent *remote_host_info;
  long remote_address;
  int ch;

  // options descriptor
  const struct option longopts[] = {
    { "in",    required_argument, NULL, 'i' },
    { "out",   required_argument, NULL, 'o' },
    { NULL,    0,                 NULL,  0 }
  };

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

  if(argc < 2) {
    return usage(program_name);
  } else {
    remote_host_name = argv[0];
    remote_port = atoi(argv[1]);
    local_port = (argc >= 3) ? atoi(argv[2]) : remote_port;
  }

  // make a socket
  if( (osc_server_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
    printf("ERROR: couldn't make a socket!\n");
    return 1;
  }

  // get IP address from name
  remote_host_info = gethostbyname(remote_host_name);

  // fill address struct
  memcpy(&remote_address, remote_host_info->h_addr, remote_host_info->h_length);
  remote_address_info.sin_addr.s_addr = remote_address;
  remote_address_info.sin_port        = htons(remote_port);
  remote_address_info.sin_family      = AF_INET;

  printf("Connecting to %s on port %d\n", remote_host_name, remote_port);

  host_address_info.sin_addr.s_addr=INADDR_ANY;
  host_address_info.sin_port=htons(local_port);
  host_address_info.sin_family=AF_INET;

  if( (status=bind(osc_server_socket, (struct sockaddr*)&host_address_info, sizeof(host_address_info))) < 0 ) {
    printf("ERROR: couldn't connect to host (status %d) - try another port!\n", status);
    return 1;
  }
  printf("Receiving on port %d\n", local_port);

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
  
  // start proxy loop
  proxy_handler();
    
  return 0; // no error
}
