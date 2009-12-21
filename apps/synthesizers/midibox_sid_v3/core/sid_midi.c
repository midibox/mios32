// $Id$
/*
 * MBSID MIDI Parser
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

#include <sid.h>

#include "sid_midi.h"

#include "sid_patch.h"
#include "sid_se.h"
#include "sid_se_l.h"
#include "sid_knob.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_Init(u32 mode)
{
  s32 status = 0;

  status |= SID_MIDI_L_Init(mode);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // TODO: support for multiple SIDs
  u8 sid = 0;
  sid_se_engine_t engine = sid_patch[sid].engine;

  switch( midi_package.event ) {
    case NoteOff:
      midi_package.velocity = 0;
      // no break, fall through

    case NoteOn:
      switch( engine ) {
        case SID_SE_LEAD:      return SID_MIDI_L_Receive_Note(sid, midi_package);
        case SID_SE_BASSLINE:  return -2; // TODO
        case SID_SE_DRUM:      return -2; // TODO
        case SID_SE_MULTI:     return -2; // TODO
        default:               return -1; // unsupported engine
      }
      break;

    case CC:
      // knob values are available for all engines
      if( midi_package.chn == sid_se_midi_voice[sid][0].midi_channel ) {
	switch( midi_package.cc_number ) {
	  case  1: SID_KNOB_SetValue(sid, SID_KNOB_1, midi_package.value << 1); break;
	  case 16: SID_KNOB_SetValue(sid, SID_KNOB_2, midi_package.value << 1); break;
	  case 17: SID_KNOB_SetValue(sid, SID_KNOB_3, midi_package.value << 1); break;
	  case 18: SID_KNOB_SetValue(sid, SID_KNOB_4, midi_package.value << 1); break;
	  case 19: SID_KNOB_SetValue(sid, SID_KNOB_5, midi_package.value << 1); break;
	}
      }

      switch( engine ) {
        case SID_SE_LEAD:      return SID_MIDI_L_Receive_CC(sid, midi_package);
        case SID_SE_BASSLINE:  return -2; // TODO
        case SID_SE_DRUM:      return -2; // TODO
        case SID_SE_MULTI:     return -2; // TODO
        default:               return -1; // unsupported engine
      }
      break;

    case PitchBend: {
      u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
      u16 pitchbend_value_8bit = pitchbend_value_14bit >> 6;

      // copy pitchbender value into mod matrix source
      SID_KNOB_SetValue(sid, SID_KNOB_PITCHBENDER, pitchbend_value_8bit);
    } break;


  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help Functions
/////////////////////////////////////////////////////////////////////////////
s32 SID_MIDI_PushWT(sid_se_midi_voice_t *mv, u8 note)
{
  return 0; // no error
}

s32 SID_MIDI_PopWT(sid_se_midi_voice_t *mv, u8 note)
{
  return 0; // no error
}

