// $Id$
/*
 * MIDI layer functions for MIOS32
 *
 * the mios32_midi_package_t format complies with USB MIDI spec (details see there)
 * and is used for transfers between other MIDI ports as well.
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_MIDI)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes MIDI layer
// IN: <mode>: currently only mode 0 supported
//             later we could provide operation modes
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Init(u32 mode)
{
  s32 ret = 0;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if !defined(MIOS32_DONT_USE_USB)
  if( MIOS32_USB_Init(mode) < 0 )
    ret |= (1 << 0);
#endif

  return -ret;
}


/////////////////////////////////////////////////////////////////////////////
// Sends a package over given port
// This is a low level function - use the remaining MIOS32_MIDI_Send* functions
// to send specific MIDI events
// IN: <port>: MIDI port 
//             0..15: USB, 16..31: USART, 32..47: IIC, 48..63: Ethernet
//     <package>: MIDI package (see definition in mios32_midi.h)
// OUT: returns -1 if port not available
//      returns -2 if port available, but buffer full (retry it)
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage(u8 port, mios32_midi_package_t package)
{
  // insert subport number into package
  package.type = (package.type&0x0f) | (port << 4);

  // branch depending on selected port
  switch( port >> 4 ) {
    case 0:
#if !defined(MIOS32_DONT_USE_USB)
      return MIOS32_USB_MIDIPackageSend(package.ALL);
#else
      return -1; // USB has been disabled
#endif

    case 1:
      return -1; // USART not implemented yet

    case 2:
      return -1; // IIC not implemented yet

    case 3:
      return -1; // Ethernet not implemented yet
      
    default:
      // invalid port
      return -1;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Sends a MIDI Event
// This function is provided for a more comfortable use model
// It is aliased to following functions (defined in mios32_midi.h)
//    o MIOS32_MIDI_SendNoteOff(port, chn, note, vel)
//    o MIOS32_MIDI_SendNoteOn(port, chn, note, vel)
//    o MIOS32_MIDI_SendPolyAftertouch(port, chn, note, val)
//    o MIOS32_MIDI_SendCC(port, chn, cc, val)
//    o MIOS32_MIDI_SendProgramChange(port, chn, prg)
//    o MIOS32_MIDI_ChannelAftertouch(port, chn, val)
//    o MIOS32_MIDI_PitchBend(port, chn, val)
//
// IN: <port>: MIDI port 
//     <evnt0> <evnt1> <evnt2> up to 3 bytes
// OUT: returns -1 if port not available
//      returns -2 if port available, but buffer full (retry it)
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendEvent(u8 port, u8 evnt0, u8 evnt1, u8 evnt2)
{
  mios32_midi_package_t package;

  // MEMO: don't optimize this function by calling MIOS32_MIDI_SendSpecialEvent
  // from here, because the 4 * u8 parameter list of this function leads
  // to best compile results (4*u8 combined to a single u32)

  package.type  = evnt0 >> 4;
  package.evnt0 = evnt0;
  package.evnt1 = evnt1;
  package.evnt2 = evnt2;
  return MIOS32_MIDI_SendPackage(port, package);
}


/////////////////////////////////////////////////////////////////////////////
// Sends a special type MIDI Event
// This function is provided for a more comfortable use model
// It is aliased to following functions (defined in mios32_midi.h)
//    o MIOS32_MIDI_SendMTC(port, val)
//    o MIOS32_MIDI_SendSongPosition(port, val)
//    o MIOS32_MIDI_SendSongSelect(port, val)
//    o MIOS32_MIDI_SendTuneRequest()
//    o MIOS32_MIDI_SendClock()
//    o MIOS32_MIDI_SendTick()
//    o MIOS32_MIDI_SendStart()
//    o MIOS32_MIDI_SendStop()
//    o MIOS32_MIDI_SendContinue()
//    o MIOS32_MIDI_SendActiveSense()
//    o MIOS32_MIDI_SendReset()
//
// IN: <port>: MIDI port 
//     <type>: the event type
//     <evnt0> <evnt1> <evnt2> up to 3 bytes
// OUT: returns -1 if port not available
//      returns -2 if port available, but buffer full (retry it)
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendSpecialEvent(u8 port, u8 type, u8 evnt0, u8 evnt1, u8 evnt2)
{
  mios32_midi_package_t package;

  package.type  = type;
  package.evnt0 = evnt0;
  package.evnt1 = evnt1;
  package.evnt2 = evnt2;
  return MIOS32_MIDI_SendPackage(port, package);
}


/////////////////////////////////////////////////////////////////////////////
// Sends a SysEx Stream
// This function is provided for a more comfortable use model
// IN: <port>: MIDI port 
//     <stream>: pointer to SysEx stream
//     <count>: number of bytes
// OUT: returns -1 if port not available
//      returns -2 if port available, but buffer full (retry it)
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendSysEx(u8 port, u8 *stream, u32 count)
{
  s32 res;
  u32 offset;
  mios32_midi_package_t package;

  // MEMO: have a look into the project.lss file - gcc optimizes this code pretty well :)

  for(offset=0; offset<count;) {
    // package type depends on number of remaining bytes
    switch( count-offset ) {
      case 1: 
	package.type = 0x5; // SysEx ends with following single byte. 
	package.evnt0 = stream[offset++];
	package.evnt1 = 0x00;
	package.evnt2 = 0x00;
	break;
      case 2:
	package.type = 0x6; // SysEx ends with following two bytes.
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = 0x00;
	break;
      case 3:
	package.type = 0x7; // SysEx ends with following three bytes. 
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = stream[offset++];
	break;
      default:
	package.type = 0x4; // SysEx starts or continues
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = stream[offset++];
    }

    while( (res=MIOS32_MIDI_SendPackage(port, package)) == -2 ) {
      // TODO: this is a blocking function! We poll until buffer is free again.
      // Are there better ways?
    }

    // other expection? (e.g., port not available)
    if( res < 0 )
      return res;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Checks for incoming MIDI messages, calls either the callback_event or
// callback_sysex function with following parameters:
//    callback_event(u8 port, mios32_midi_package_t midi_package)
//    callback_sysex(u8 port, u8 sysex_byte)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Receive_Handler(void *_callback_event, void *_callback_sysex)
{
  u8 port;
  mios32_midi_package_t package;
  u8 again;

  void (*callback_event)(u8 port, mios32_midi_package_t midi_package) = _callback_event;
  void (*callback_sysex)(u8 port, u8 sysex_byte) = _callback_sysex;

  do {
    // notifies, that there are more messages to be processed
    again = 0;

    // TODO: more generic handler used by all port types (USB, USART, IIC, etc..)

    // check for USB messages
    if( MIOS32_USB_MIDIPackageReceive(&package) ) {
      // get port and mask out channel number
      port = package.type >> 4;
      package.type &= 0xf;

      if( package.type >= 0x8 ) {
	callback_event(port, package);
      } else {
	switch( package.type ) {
  	case 0x0: // reserved, ignore
	case 0x1: // cable events, ignore
	  break;

	case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc. 
	case 0x3: // Three-byte System Common messages like SPP, etc. 
	  callback_event(port, package); // -> forwarded as event
	  break;
	case 0x4: // SysEx starts or continues (3 bytes)
	  callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	  callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	  callback_sysex(port, package.evnt2); // -> forwarded as SysEx
	  break;
	case 0x5: // Single-byte System Common Message or SysEx ends with following single byte. 
	  if( package.evnt0 >= 0xf8 )
	    callback_event(port, package); // -> forwarded as event
	  else
	    callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	  break;
	case 0x6: // SysEx ends with following two bytes.
	  callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	  callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	  break;
	case 0x7: // SysEx ends with following three bytes.
	  callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	  callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	  callback_sysex(port, package.evnt2); // -> forwarded as SysEx
	  break;
	}
      }
    }
  } while( again );

  return 0;
}


#endif /* MIOS32_DONT_USE_MIDI */
