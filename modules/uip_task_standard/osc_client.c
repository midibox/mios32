// $Id$
/*
 * OSC Client Functions
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
#include <mios32.h>
#include <string.h>

#include "uip.h"
#include "osc_server.h"
#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Transfer mode names
// must be aligned with definitions in osc_client.h!!!
/////////////////////////////////////////////////////////////////////////////
static const char full_mode_names[OSC_CLIENT_NUM_TRANSFER_MODES+1][21] = {
  "MIDI Messages       ",
  "Text Msg (Integer)  ",
  "Text Msg (Float)    ",
  "Pianist Pro (iPad)  ",
  "TouchOSC            ",
  "** invalid mode **  ",
};

static const char short_mode_names[OSC_CLIENT_NUM_TRANSFER_MODES+1][5] = {
  "MIDI",
  "Int.",
  "Flt.",
  "MPP ",
  "TOSC",
  "??? ",
};


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 osc_transfer_mode[OSC_CLIENT_NUM_PORTS];

// small SysEx buffer for optimized blobs
#define OSC_CLIENT_SYSEX_BUFFER_SIZE 64
static u8 sysex_buffer[OSC_CLIENT_NUM_PORTS][OSC_CLIENT_SYSEX_BUFFER_SIZE];
static u8 sysex_buffer_len[OSC_CLIENT_NUM_PORTS];


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC client
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_Init(u32 mode)
{
  int i;

  for(i=0; i<OSC_CLIENT_NUM_PORTS; ++i) {
    osc_transfer_mode[i] = OSC_CLIENT_TRANSFER_MODE_MIDI;
    sysex_buffer_len[i] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Transfer Mode Set/Get functions
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_TransferModeSet(u8 osc_port, u8 mode)
{
  if( osc_port >= OSC_CLIENT_NUM_PORTS )
    return -1; // invalid connection

  osc_transfer_mode[osc_port] = mode;
  return 0;
}

u8 OSC_CLIENT_TransferModeGet(u8 osc_port)
{
  return osc_transfer_mode[osc_port];
}


/////////////////////////////////////////////////////////////////////////////
// returns the full name of the transfer mode (up to 20 chars)
/////////////////////////////////////////////////////////////////////////////
const char* OSC_CLIENT_TransferModeFullNameGet(u8 mode)
{
  return (const char*)&full_mode_names[(mode >= OSC_CLIENT_NUM_TRANSFER_MODES) ? OSC_CLIENT_NUM_TRANSFER_MODES : mode];
}

/////////////////////////////////////////////////////////////////////////////
// returns the short name of the transfer mode (up to 4 chars)
/////////////////////////////////////////////////////////////////////////////
const char* OSC_CLIENT_TransferModeShortNameGet(u8 mode)
{
  return (const char*)&short_mode_names[(mode >= OSC_CLIENT_NUM_TRANSFER_MODES) ? OSC_CLIENT_NUM_TRANSFER_MODES : mode];
}


/////////////////////////////////////////////////////////////////////////////
// Send a MIDI event
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendMIDIEvent(u8 osc_port, mios32_midi_package_t package)
{
  if( osc_port >= OSC_CLIENT_NUM_PORTS )
    return -1; // invalid port

  // check if server is running
  if( !UIP_TASK_ServicesRunning() )
    return -2; 

  // create the OSC packet
  u8 packet[128];
  u8 *end_ptr = packet;

  if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_MCMPP &&
      package.type >= NoteOff && package.type <= PitchBend ) {
    char event_path[30];
    switch( package.type ) {
    case NoteOff:
      package.velocity = 0;
      // fall through
    case NoteOn:
      sprintf(event_path, "/mcmpp/key/%d/%d", package.note, package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case PolyPressure:
      sprintf(event_path, "/mcmpp/polypressure/%d/%d", package.note, package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case CC:
      sprintf(event_path, "/mcmpp/cc/%d/%d", package.cc_number, package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.value/127.0);
      break;

    case ProgramChange:
      sprintf(event_path, "/mcmpp/programchange/%d/%d", package.program_change, package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      break;

    case Aftertouch:
      sprintf(event_path, "/mcmpp/aftertouch/%d", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case PitchBend: {
      sprintf(event_path, "/mcmpp/pitch/%d", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      int value = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
      if( value >= 0 && value <= 127 )
	value = 0;
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)value/8191.0);
    } break;

    default:
      sprintf(event_path, "/mcmpp/invalid/%d", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      break;
    }
  } else if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_TOSC &&
      package.type >= NoteOff && package.type <= PitchBend ) {
    char event_path[30];
    switch( package.type ) {
    case NoteOff:
      package.velocity = 0;
      // fall through
    case NoteOn:
      sprintf(event_path, "/%d/note/%d", package.chn+1, package.note);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case PolyPressure:
      sprintf(event_path, "/%d/polypressure/%d", package.chn+1, package.note);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case CC:
      sprintf(event_path, "/%d/cc/%d", package.chn+1, package.cc_number);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.value/127.0);
      break;

    case ProgramChange:
      sprintf(event_path, "/%d/programchange/%d", package.chn+1, package.program_change);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      break;

    case Aftertouch:
      sprintf(event_path, "/%d/aftertouch", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      break;

    case PitchBend: {
      sprintf(event_path, "/%d/pitch/%d", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      int value = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
      if( value >= 0 && value <= 127 )
	value = 0;
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
      end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)value/8191.0);
    } break;

    default:
      sprintf(event_path, "/%d/invalid", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      break;
    }
  } else if( osc_transfer_mode[osc_port] != OSC_CLIENT_TRANSFER_MODE_MIDI &&
      package.type >= NoteOff && package.type <= PitchBend ) {
    char event_path[30];
    switch( package.type ) {
    case NoteOff:
      package.velocity = 0;
      // fall through
    case NoteOn:
      sprintf(event_path, "/%d/note", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",if");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.note);
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",ii");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.note);
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.velocity);
      }
      break;

    case PolyPressure:
      sprintf(event_path, "/%d/polypressure", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",if");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.note);
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",ii");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.note);
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.velocity);
      }
      break;

    case CC:
      sprintf(event_path, "/%d/cc", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",if");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.cc_number);
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.value/127.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",ii");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.cc_number);
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.value);
      }
      break;

    case ProgramChange:
      sprintf(event_path, "/%d/programchange", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.program_change/127.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",i");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.program_change);
      }
      break;

    case Aftertouch:
      sprintf(event_path, "/%d/aftertouch", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)package.velocity/127.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",i");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, package.velocity);
      }
      break;

    case PitchBend: {
      sprintf(event_path, "/%d/pitchbend", package.chn+1);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      int value = ((package.evnt1 & 0x7f) | (int)((package.evnt2 & 0x7f) << 7)) - 8192;
      if( value >= 0 && value <= 127 )
	value = 0;
      if( osc_transfer_mode[osc_port] == OSC_CLIENT_TRANSFER_MODE_FLOAT ) {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",f");
	end_ptr = MIOS32_OSC_PutFloat(end_ptr, (float)value/8191.0);
      } else {
	end_ptr = MIOS32_OSC_PutString(end_ptr, ",i");
	end_ptr = MIOS32_OSC_PutInt(end_ptr, value);
      }
    } break;

    default:
      sprintf(event_path, "/%d/invalid", package.chn);
      end_ptr = MIOS32_OSC_PutString(end_ptr, event_path);
      break;
    }
  } else {
    // SysEx streams are embedded into blobs
    if( package.evnt0 == 0xf0 )
      sysex_buffer_len[osc_port] = 0;

    int sysex_len = 0;
    if( package.type == 0xf && package.evnt0 < 0xf8 )
      sysex_len = 1;
    else if( package.type == 0x4 || package.type == 0x7 )
      sysex_len = 3;
    else if( package.type == 0x5 )
      sysex_len = 1;
    else if( package.type == 0x6 )
      sysex_len = 2;

    if( sysex_len ) {
      u8 send_sysex = 0;
      u8 *buffer = (u8 *)&sysex_buffer[osc_port];
      u8 *buffer_len = (u8 *)&sysex_buffer_len[osc_port];
      int i;
      for(i=0; i<sysex_len; ++i) {
	// avoid usage of array access in package.* to avoid endianess issues
	u8 evnt;
	if( i == 0 )
	  evnt = package.evnt0;
	else if( i == 1 )
	  evnt = package.evnt1;
	else
	  evnt = package.evnt2;

	buffer[*buffer_len] = evnt;
	*buffer_len += 1;

	if( evnt == 0xf7 || *buffer_len >= OSC_CLIENT_SYSEX_BUFFER_SIZE ) {
	  char midi_path[8];
	  strcpy(midi_path, "/midiX");
	  midi_path[5] = '1' + osc_port;
	  end_ptr = MIOS32_OSC_PutString(end_ptr, midi_path);
	  end_ptr = MIOS32_OSC_PutString(end_ptr, ",b");
	  end_ptr = MIOS32_OSC_PutBlob(end_ptr, buffer, *buffer_len);
	  *buffer_len = 0;
	  send_sysex = 1;
	}
      }

      if( !send_sysex )
	return 0; // wait until sysex stream is terminated (or buffer is full)
    } else {
      char midi_path[8];
      strcpy(midi_path, "/midiX");
      midi_path[5] = '1' + osc_port;
      end_ptr = MIOS32_OSC_PutString(end_ptr, midi_path);
      end_ptr = MIOS32_OSC_PutString(end_ptr, ",m");
      end_ptr = MIOS32_OSC_PutMIDI(end_ptr, package);
    }
  }

  // send packet and exit
  return OSC_SERVER_SendPacket(osc_port, packet, (u32)(end_ptr-packet));
}


/////////////////////////////////////////////////////////////////////////////
// Send a SysEx stream
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendSysEx(u8 osc_port, u8 *stream, u32 count)
{
  if( osc_port >= OSC_CLIENT_NUM_PORTS )
    return -1; // invalid port

  // check if server is running
  if( !UIP_TASK_ServicesRunning() )
    return -2; 

  // we limit the maximum blob size to 64
  // send multiple blobs if required
  int max_bytes = 64;
  int send_offset = 0;
  while( send_offset < count ) {
    u32 bytes_to_send = ((send_offset + max_bytes) > count) ? (count-send_offset) : max_bytes;

    // create the OSC packet
    u8 packet[128];
    u8 *end_ptr = packet;

    char midi_path[8];
    strcpy(midi_path, "/midiX");
    midi_path[5] = '1' + osc_port;
    end_ptr = MIOS32_OSC_PutString(end_ptr, midi_path);
    end_ptr = MIOS32_OSC_PutString(end_ptr, ",b");
    end_ptr = MIOS32_OSC_PutBlob(end_ptr, (u8 *)&stream[send_offset], bytes_to_send);

    OSC_SERVER_SendPacket(osc_port, packet, (u32)(end_ptr-packet));

    send_offset += bytes_to_send;
  };

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Send MIDI events in a bundle (it is recommented not to send more than 8 events!)
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendMIDIEventBundled(u8 osc_port, mios32_midi_package_t *p, u8 num_events, mios32_osc_timetag_t timetag)
{
  if( osc_port >= OSC_CLIENT_NUM_PORTS )
    return -1; // invalid port

  // check if server is running
  if( !UIP_TASK_ServicesRunning() )
    return -2; 

  // create the OSC packet
  u8 packet[256];
  u8 *end_ptr = packet;
  u8 *insert_len_ptr;
  int i;

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

  // send packet and exit
  return OSC_SERVER_SendPacket(osc_port, packet, (u32)(end_ptr-packet));
}
