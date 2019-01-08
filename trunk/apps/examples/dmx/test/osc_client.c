// $Id: osc_client.c 387 2009-03-04 23:15:36Z tk $
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
// Send a Button Value
// Path: /cs/button/state <button-number> <0 or 1>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendButtonState(mios32_osc_timetag_t timetag, u32 button, u8 pressed)
{
  // create the OSC packet
  u8 packet[128];
  u8 *end_ptr = packet;
  u8 *insert_len_ptr;

  end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
  end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
  insert_len_ptr = end_ptr; // remember this address - we will insert the length later
  end_ptr += 4;
  end_ptr = MIOS32_OSC_PutString(end_ptr, "/cs/button/state");
  end_ptr = MIOS32_OSC_PutString(end_ptr, ",ii");
  end_ptr = MIOS32_OSC_PutInt(end_ptr, button);
  end_ptr = MIOS32_OSC_PutInt(end_ptr, pressed);

  // now insert the message length
  MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));

  // send packet and exit
  return OSC_SERVER_SendPacket(packet, (u32)(end_ptr-packet));
}


/////////////////////////////////////////////////////////////////////////////
// Send a Pot value as float
// Path: /cs/pot/value <pot-number> <0.0000..1.0000>
/////////////////////////////////////////////////////////////////////////////
s32 OSC_CLIENT_SendPotValue(mios32_osc_timetag_t timetag, u32 pot, float value)
{
  // create the OSC packet
  u8 packet[128];
  u8 *end_ptr = packet;
  u8 *insert_len_ptr;

  end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
  end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
  insert_len_ptr = end_ptr; // remember this address - we will insert the length later
  end_ptr += 4;
  end_ptr = MIOS32_OSC_PutString(end_ptr, "/cs/pot/value");
  end_ptr = MIOS32_OSC_PutString(end_ptr, ",if");
  end_ptr = MIOS32_OSC_PutInt(end_ptr, pot);
  end_ptr = MIOS32_OSC_PutFloat(end_ptr, value);

  // now insert the message length
  MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));

  // send packet and exit
  return OSC_SERVER_SendPacket(packet, (u32)(end_ptr-packet));
}
