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

#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialize the OSC client
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Send a MIDI event
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendMIDIEvent(mios32_osc_timetag_t timetag, mios32_midi_package_t p)
{
  // create the OSC packet
  u8 packet[128];
  u8 *end_ptr = packet;
  u8 *insert_len_ptr;

  end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
  end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
  insert_len_ptr = end_ptr; // remember this address - we will insert the length later
  end_ptr += 4;
  end_ptr = MIOS32_OSC_PutString(end_ptr, "/midi");
  end_ptr = MIOS32_OSC_PutString(end_ptr, ",m");
  end_ptr = MIOS32_OSC_PutMIDI(end_ptr, p);

  // now insert the message length
  MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));

  // send packet and exit
  return OSC_SERVER_SendPacket(packet, (u32)(end_ptr-packet));
}


/////////////////////////////////////////////////////////////////////////////
// Send MIDI events in a bundle (it is recommented not to send more than 8 events!)
// Path: /midi <midi-package>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendMIDIEventBundled(mios32_osc_timetag_t timetag, mios32_midi_package_t *p, u8 num_events)
{
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
  return OSC_SERVER_SendPacket(packet, (u32)(end_ptr-packet));
}
