// $Id$
/*
 * MIDI Monitor functions
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

#include "midimon.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// shortcut for printing a message on MIOS Terminal
// could also be replaced by "printf" if source code used in a different environment
#define MSG MIOS32_MIDI_SendDebugMessage


// jumper options (all pin states are low-active, and will be inverted here)
#define JUMPER_MIDIMON_ACTIVE  (MIOS32_BOARD_J5_PinGet(4) ? 0 : 1)
#define JUMPER_FILTER_ACTIVE   (MIOS32_BOARD_J5_PinGet(5) ? 0 : 1)
#define JUMPER_TEMPO_ACTIVE    (MIOS32_BOARD_J5_PinGet(6) ? 0 : 1)
#define JUMPER_MTC_ACTIVE      (MIOS32_BOARD_J5_PinGet(7) ? 0 : 1)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

const char note_name[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

/////////////////////////////////////////////////////////////////////////////
// Initialize the monitor
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Packet Receiver function
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_Receive(mios32_midi_port_t port, mios32_midi_package_t package, u32 timestamp, u8 filter_sysex_message)
{
  char pre_str[32];

  if( !JUMPER_MIDIMON_ACTIVE )
    return 0; // MIDImon mode not selected via jumper

  // derive port name and build pre-string
  u8 port_ix = port & 0x0f;
  char port_ix_name = (port_ix < 9) ? ('1'+port_ix) : ('A'+(port_ix-9));
  switch( port & 0xf0 ) {
    case USB0:  sprintf(pre_str, "[USB%c]", port_ix_name); break;
    case UART0: sprintf(pre_str, "[IN%c ]", port_ix_name); break;
    case IIC0:  sprintf(pre_str, "[IIC%c]", port_ix_name); break;
    default:    sprintf(pre_str, "[P.%02X ]", port);
  }

  // branch depending on package type
  u8 msg_sent = 0;
  switch( package.type ) {
    case 0x2:  // Two-byte System Common messages like MTC, SongSelect, etc.
      if( package.evnt0 == 0xf1 ) {
	if( !JUMPER_FILTER_ACTIVE ) {
	  switch( package.evnt1 & 0xf0 ) {
	    case 0x00: MSG("%s MTC Frame   Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x10: MSG("%s MTC Frame   High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x20: MSG("%s MTC Seconds Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x30: MSG("%s MTC Seconds High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x40: MSG("%s MTC Minutes Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x50: MSG("%s MTC Minutes High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x60: MSG("%s MTC Hours   Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x70: MSG("%s MTC Hours   High: %X (SMPTE Type: %d)\n", 
			   pre_str, package.evnt1 & 0x1, (package.evnt1>>1) & 0x7); break;
	    default:
	      MSG("%s MTC Invalid: %02X %02X %02X\n", pre_str, package.evnt0, package.evnt1, package.evnt2);
	  }
	}
	msg_sent = 1;
      } else if( package.evnt0 == 0xf3 ) {
	MSG("%s Song Number #%d\n", pre_str, package.evnt1);
	msg_sent = 1;
      }
      break;

    case 0x3:  // Three-byte System Common messages like SPP, etc.
      if( package.evnt0 == 0xf2 ) {
	u16 song_pos = package.evnt1 | (package.evnt2 >> 7);
	MSG("%s Song Position %d.%d:%d\n", pre_str, song_pos >> 4, (song_pos >> 2) & 3, song_pos/4);
	msg_sent = 1;
      }
      break;

    case 0x4:  // SysEx starts or continues (3 bytes)
    case 0x7:  // SysEx ends with following three bytes
      if( !filter_sysex_message )
	MSG("%s SysEx: %02X %02X %02X\n", pre_str, package.evnt0, package.evnt1, package.evnt2);
      msg_sent = 1;
      break;

    case 0x5: // Single-byte System Common Message or SysEx ends with following single bytes
    case 0xf: // Single Byte
      switch( package.evnt0 ) {
        case 0xf6: MSG("%s Tune Request (F6)\n", pre_str); break;
        case 0xf7: if( !filter_sysex_message ) { MSG("%s SysEx End (F7)\n", pre_str); } break;
        case 0xf8: if( !JUMPER_FILTER_ACTIVE ) { MSG("%s MIDI Clock (F8)\n", pre_str); } break;
        case 0xf9: MSG("%s MIDI Tick (F9)\n", pre_str); break;
        case 0xfa: MSG("%s MIDI Clock Start (FA)\n", pre_str); break;
        case 0xfb: MSG("%s MIDI Clock Continue (FB)\n", pre_str); break;
        case 0xfc: MSG("%s MIDI Clock Stop (FC)\n", pre_str); break;
        case 0xfd: MSG("%s Inspecified Realtime Event (FD)\n", pre_str); break;
        case 0xfe: if( !JUMPER_FILTER_ACTIVE ) { MSG("%s Active Sense (FE)\n", pre_str); } break;
        case 0xff: MSG("%s Reset (FF)\n", pre_str); break;
        default:   MSG("%s Invalid Single-Byte Event (%02X)\n", pre_str, package.evnt0); break;
      }
      msg_sent = 1;
      break;

    case 0x6:  // SysEx ends with following two bytes
      if( !filter_sysex_message )
	MSG("%s SysEx: %02X %02X\n", pre_str, package.evnt0, package.evnt1);
      msg_sent = 1;
      break;

    case 0x8: // Note Off
      MSG("%s Chn%2d  Note Off %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0x9: // Note On
      MSG("%s Chn%2d  Note On  %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xa: // Poly Aftertouch
      MSG("%s Chn%2d  Poly Aftertouch %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xb: // CC
      MSG("%s Chn%2d  CC#%3d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, package.evnt1, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xc: // Program Change
      MSG("%s Chn%2d  Program Change #%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, package.evnt1);
      msg_sent = 1;
      break;
      
    case 0xd: // Channel Aftertouch
      MSG("%s Chn%2d  Channel Aftertouch %s%d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2);
      msg_sent = 1;
      break;
  }

  // unspecified or invalid packages
  if( !msg_sent ) {
    MSG("%s Invalid Package (Type %d: %02X %02X %02X)\n",
	pre_str, package.type, package.evnt0, package.evnt1, package.evnt2);
  }

  return 0; // no error
}
