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

// this global array is read from MIOS32_IIC_MIDI and MIOS32_UART_MIDI to
// determine the number of MIDI bytes which are part of a package
const u8 mios32_midi_pcktype_num_bytes[16] = {
  0, // 0: invalid/reserved event
  0, // 1: invalid/reserved event
  2, // 2: two-byte system common messages like MTC, Song Select, etc.
  3, // 3: three-byte system common messages like SPP, etc.
  3, // 4: SysEx starts or continues
  1, // 5: Single-byte system common message or sysex sends with following single byte
  2, // 6: SysEx sends with following two bytes
  3, // 7: SysEx sends with following three bytes
  3, // 8: Note Off
  3, // 9: Note On
  3, // a: Poly-Key Press
  3, // b: Control Change
  2, // c: Program Change
  2, // d: Channel Pressure
  3, // e: PitchBend Change
  1  // f: single byte
};

// Number if expected bytes for a common MIDI event - 1
const u8 mios32_midi_expected_bytes_common[8] = {
  2, // Note On
  2, // Note Off
  2, // Poly Preasure
  2, // Controller
  1, // Program Change
  1, // Channel Preasure
  2, // Pitch Bender
  0, // System Message - must be zero, so that mios32_midi_expected_bytes_system[] will be used
};

// // Number if expected bytes for a system MIDI event - 1
const u8 mios32_midi_expected_bytes_system[16] = {
  1, // SysEx Begin (endless until SysEx End F7)
  1, // MTC Data frame
  2, // Song Position
  1, // Song Select
  0, // Reserved
  0, // Reserved
  0, // Request Tuning Calibration
  0, // SysEx End

  // Note: just only for documentation, Realtime Messages don't change the running status
  0, // MIDI Clock
  0, // MIDI Tick
  0, // MIDI Start
  0, // MIDI Continue
  0, // MIDI Stop
  0, // Reserved
  0, // Active Sense
  0, // Reset
};


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initializes MIDI layer
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Init(u32 mode)
{
  s32 ret = 0;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
  if( MIOS32_USB_MIDI_Init(0) < 0 )
    ret |= (1 << 0);
#endif

#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
  if( MIOS32_UART_MIDI_Init(0) < 0 )
    ret |= (1 << 1);
#endif

#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
  if( MIOS32_IIC_MIDI_Init(0) < 0 )
    ret |= (1 << 2);
#endif

  return -ret;
}


/////////////////////////////////////////////////////////////////////////////
// This function checks the availability of a MIDI port
// IN: <port>: MIDI port 
//             DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7
// OUT: 1: port available
//      0: port not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = MIOS32_MIDI_DEFAULT_PORT;
  }

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_CheckAvailable();
#else
      return 0; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_CheckAvailable(port & 0xf);
#else
      return 0; // UART_MIDI has been disabled
#endif

    case 3:
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_CheckAvailable(port & 0xf);
#else
      return 0; // IIC_MIDI has been disabled
#endif

    case 4:
      return 0; // Ethernet not implemented yet
      
    default:
      // invalid port
      return 0;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Sends a package over given port
// This is a low level function. In difference to other MIOS32_MIDI_Send* functions,
// It allows to send packages in non-blocking mode (caller has to retry if -2 is returned)
// IN: <port>: MIDI port 
//             DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7
//     <package>: MIDI package (see definition in mios32_midi.h)
// OUT: returns -1 if port not available
//      returns -2 buffer is full
//                 caller should retry until buffer is free again
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage_NonBlocking(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = MIOS32_MIDI_DEFAULT_PORT;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_MIDIPackageSend_NonBlocking(package);
#else
      return -1; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_PackageSend_NonBlocking(package.cable, package);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case 3:
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_PackageSend_NonBlocking(package.cable, package);
#else
      return -1; // IIC_MIDI has been disabled
#endif

    case 4:
      return -1; // Ethernet not implemented yet
      
    default:
      // invalid port
      return -1;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Sends a package over given port
// This is a low level function - use the remaining MIOS32_MIDI_Send* functions
// to send specific MIDI events
// (blocking function)
// IN: <port>: MIDI port 
//             DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7
//     <package>: MIDI package (see definition in mios32_midi.h)
// OUT: returns -1 if port not available
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = MIOS32_MIDI_DEFAULT_PORT;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_MIDIPackageSend(package);
#else
      return -1; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_PackageSend(package.cable, package);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case 3:
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_PackageSend(package.cable, package);
#else
      return -1; // IIC_MIDI has been disabled
#endif

    case 4:
      return -1; // Ethernet not implemented yet
      
    default:
      // invalid port
      return -1;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Sends a MIDI Event
// This function is provided for a more comfortable use model
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
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendEvent(mios32_midi_port_t port, u8 evnt0, u8 evnt1, u8 evnt2)
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

s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{ MIOS32_MIDI_SendEvent(port, 0x80 | chn, note, vel); }

s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{ MIOS32_MIDI_SendEvent(port, 0x90 | chn, note, vel); }

s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val)
{ MIOS32_MIDI_SendEvent(port, 0xa0 | chn, note, val); }

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc, u8 val)
{ MIOS32_MIDI_SendEvent(port, 0xb0 | chn, cc,   val); }

s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg)
{ MIOS32_MIDI_SendEvent(port, 0xc0 | chn, prg,  0x00); }

s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val)
{ MIOS32_MIDI_SendEvent(port, 0xd0 | chn, val,  0x00); }

s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val)
{ MIOS32_MIDI_SendEvent(port, 0xe0 | chn, val & 0x7f, val >> 7); }


/////////////////////////////////////////////////////////////////////////////
// Sends a special type MIDI Event
// This function is provided for a more comfortable use model
// It is aliased to following functions
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
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendSpecialEvent(mios32_midi_port_t port, u8 type, u8 evnt0, u8 evnt1, u8 evnt2)
{
  mios32_midi_package_t package;

  package.type  = type;
  package.evnt0 = evnt0;
  package.evnt1 = evnt1;
  package.evnt2 = evnt2;
  return MIOS32_MIDI_SendPackage(port, package);
}


s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf1, val, 0x00); }

s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x3, 0xf2, val & 0x7f, val >> 7); }

s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf3, val, 0x00); }

s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf6, 0x00, 0x00); }

s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf8, 0x00, 0x00); }

s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf9, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfa, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfb, 0x00, 0x00); }

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfc, 0x00, 0x00); }

s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfe, 0x00, 0x00); }

s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port)
{ MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xff, 0x00, 0x00); }


/////////////////////////////////////////////////////////////////////////////
// Sends a SysEx Stream
// This function is provided for a more comfortable use model
// IN: <port>: MIDI port 
//     <stream>: pointer to SysEx stream
//     <count>: number of bytes
// OUT: returns -1 if port not available
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count)
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

    res=MIOS32_MIDI_SendPackage(port, package);

    // expection? (e.g., port not available)
    if( res < 0 )
      return res;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Checks for incoming MIDI messages, calls either the callback_event or
// callback_sysex function with following parameters:
//    callback_event(mios32_midi_port_t port, mios32_midi_package_t midi_package)
//    callback_sysex(mios32_midi_port_t port, u8 sysex_byte)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Receive_Handler(void *_callback_event, void *_callback_sysex)
{
  u8 port;
  mios32_midi_package_t package;

  void (*callback_event)(mios32_midi_port_t port, mios32_midi_package_t midi_package) = _callback_event;
  void (*callback_sysex)(mios32_midi_port_t port, u8 sysex_byte) = _callback_sysex;

  u8 intf = 0; // interface to be checked
  u8 total_packages_forwarded = 0; // number of forwards - stop after 10 forwards to yield some CPU time for other tasks
  u8 packages_forwarded = 0;
  u8 again = 1;
  do {
    // Round Robin
    // TODO: maybe a list based approach would be better
    // it would allow to add/remove interfaces dynamically
    // this would also allow to give certain ports a higher priority (to add them multiple times to the list)
    // it would also improve this spagetthi code ;)
    s32 error = -1;
    switch( intf++ ) {
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      case 0: error = MIOS32_USB_MIDI_MIDIPackageReceive(&package); port = USB0 + package.cable; break;
#else
      case 0: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      case 1: error = MIOS32_UART_MIDI_PackageReceive(0, &package); port = UART0; break;
#else
      case 1: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      case 2: error = MIOS32_UART_MIDI_PackageReceive(1, &package); port = UART1; break;
#else
      case 2: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 3: error = MIOS32_IIC_MIDI_PackageReceive(0, &package); port = IIC0; break;
#else
      case 3: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 4: error = MIOS32_IIC_MIDI_PackageReceive(1, &package); port = IIC1; break;
#else
      case 4: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 5: error = MIOS32_IIC_MIDI_PackageReceive(2, &package); port = IIC2; break;
#else
      case 5: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 6: error = MIOS32_IIC_MIDI_PackageReceive(3, &package); port = IIC3; break;
#else
      case 6: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 7: error = MIOS32_IIC_MIDI_PackageReceive(4, &package); port = IIC4; break;
#else
      case 7: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 8: error = MIOS32_IIC_MIDI_PackageReceive(5, &package); port = IIC5; break;
#else
      case 8: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 9: error = MIOS32_IIC_MIDI_PackageReceive(6, &package); port = IIC6; break;
#else
      case 9: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 10: error = MIOS32_IIC_MIDI_PackageReceive(7, &package); port = IIC7; break;
#else
      case 10: error = -1; break;
#endif
      default:
	// allow 10 forwards maximum to yield some CPU time for other tasks
	if( packages_forwarded && total_packages_forwarded < 10 ) {
	  intf = 0; // restart with USB
	  packages_forwarded = 0; // for checking, if packages still have been forwarded in next round
	} else {
	  again = 0; // no more interfaces to be processed
	}
	error = -1; // empty round - no message
    }

    // message received?
    if( error >= 0 ) {
      // notify that a package has been forwarded
      ++packages_forwarded;
      ++total_packages_forwarded;

      // remove cable number from package (MIOS32_MIDI passes it's own port number)
      package.cable = 0;

      // branch depending on package type
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
