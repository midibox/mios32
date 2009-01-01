// $Id$
//! \defgroup MIOS32_MIDI
//!
//! MIDI layer functions for MIOS32
//!
//! the mios32_midi_package_t format complies with USB MIDI spec (details see there)
//! and is used for transfers between other MIDI ports as well.
//!
//! \{
/* ==========================================================================
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

//! this global array is read from MIOS32_IIC_MIDI and MIOS32_UART_MIDI to
//! determine the number of MIDI bytes which are part of a package
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

//! Number if expected bytes for a common MIDI event - 1
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

//! Number if expected bytes for a system MIDI event - 1
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

//! should only be used by MIOS32 internally and by the Bootloader!
const u8 mios32_midi_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x32 };


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };

  struct {
    unsigned CTR:3;
    unsigned CMD:1;
    unsigned MY_SYSEX:1;
  };
} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mios32_midi_port_t default_port = MIOS32_MIDI_DEFAULT_PORT;

static void (*direct_rx_callback_func)(mios32_midi_port_t port, u8 midi_byte);
static void (*direct_tx_callback_func)(mios32_midi_port_t port, u8 midi_byte);

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;
static mios32_midi_port_t last_sysex_port = DEFAULT;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIOS32_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_CmdFinished(void);
static s32 MIOS32_MIDI_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_Cmd_Query(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);
static s32 MIOS32_MIDI_SYSEX_SendAckStr(mios32_midi_port_t port, char *str);


/////////////////////////////////////////////////////////////////////////////
//! Initializes MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Init(u32 mode)
{
  s32 ret = 0;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // set default port as defined in mios32.h/mios32_config.h
  default_port = MIOS32_MIDI_DEFAULT_PORT;

  // disable Rx/Tx callback functions
  direct_rx_callback_func = NULL;
  direct_tx_callback_func = NULL;

  // initialize interfaces
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

  last_sysex_port = DEFAULT;
  sysex_state.ALL = 0;

  // TODO: allow to change device ID (read from flash, resp. BSL based EEPROM emulation)
  sysex_device_id = 0x00;

  return -ret;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a MIDI port
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \return 1: port available
//! \return 0: port not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = default_port;
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
//! Sends a package over given port
//! This is a low level function. In difference to other MIOS32_MIDI_Send* functions,
//! It allows to send packages in non-blocking mode (caller has to retry if -2 is returned)
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] package MIDI package
//! \return -1 if port not available
//! \return -2 buffer is full
//!         caller should retry until buffer is free again
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage_NonBlocking(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = default_port;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_PackageSend_NonBlocking(package);
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
//! Sends a package over given port
//! This is a low level function - use the remaining MIOS32_MIDI_Send* functions
//! to send specific MIDI events
//! (blocking function)
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] package MIDI package
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = default_port;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_PackageSend(package);
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
//! Sends a MIDI Event
//! This function is provided for a more comfortable use model
//!    o MIOS32_MIDI_SendNoteOff(port, chn, note, vel)
//!    o MIOS32_MIDI_SendNoteOn(port, chn, note, vel)
//!    o MIOS32_MIDI_SendPolyAftertouch(port, chn, note, val)
//!    o MIOS32_MIDI_SendCC(port, chn, cc, val)
//!    o MIOS32_MIDI_SendProgramChange(port, chn, prg)
//!    o MIOS32_MIDI_ChannelAftertouch(port, chn, val)
//!    o MIOS32_MIDI_PitchBend(port, chn, val)
//!
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] evnt0 first MIDI byte
//! \param[in] evnt1 second MIDI byte
//! \param[in] evnt2 third MIDI byte
//! \return -1 if port not available
//! \return 0 on success
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
{ return MIOS32_MIDI_SendEvent(port, 0x80 | chn, note, vel); }

s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{ return MIOS32_MIDI_SendEvent(port, 0x90 | chn, note, vel); }

s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xa0 | chn, note, val); }

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xb0 | chn, cc,   val); }

s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg)
{ return MIOS32_MIDI_SendEvent(port, 0xc0 | chn, prg,  0x00); }

s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xd0 | chn, val,  0x00); }

s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val)
{ return MIOS32_MIDI_SendEvent(port, 0xe0 | chn, val & 0x7f, val >> 7); }


/////////////////////////////////////////////////////////////////////////////
//! Sends a special type MIDI Event
//! This function is provided for a more comfortable use model
//! It is aliased to following functions
//!    o MIOS32_MIDI_SendMTC(port, val)
//!    o MIOS32_MIDI_SendSongPosition(port, val)
//!    o MIOS32_MIDI_SendSongSelect(port, val)
//!    o MIOS32_MIDI_SendTuneRequest()
//!    o MIOS32_MIDI_SendClock()
//!    o MIOS32_MIDI_SendTick()
//!    o MIOS32_MIDI_SendStart()
//!    o MIOS32_MIDI_SendStop()
//!    o MIOS32_MIDI_SendContinue()
//!    o MIOS32_MIDI_SendActiveSense()
//!    o MIOS32_MIDI_SendReset()
//!
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] type the event type
//! \param[in] evnt0 first MIDI byte
//! \param[in] evnt1 second MIDI byte
//! \param[in] evnt2 third MIDI byte
//! \return -1 if port not available
//! \return 0 on success
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
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf1, val, 0x00); }

s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x3, 0xf2, val & 0x7f, val >> 7); }

s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf3, val, 0x00); }

s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf6, 0x00, 0x00); }

s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf8, 0x00, 0x00); }

s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf9, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfa, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfb, 0x00, 0x00); }

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfc, 0x00, 0x00); }

s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfe, 0x00, 0x00); }

s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xff, 0x00, 0x00); }


/////////////////////////////////////////////////////////////////////////////
//! Sends a SysEx Stream
//! This function is provided for a more comfortable use model
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] stream pointer to SysEx stream
//! \param[in] count number of bytes
//! \return -1 if port not available
//! \return 0 on success
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
//! Checks for incoming MIDI messages, calls either the callback_event or
//! callback_sysex function with following parameters:
//! \code
//!    callback_event(mios32_midi_port_t port, mios32_midi_package_t midi_package)
//!    callback_sysex(mios32_midi_port_t port, u8 sysex_byte)
//! \endcode
//! \return < 0 on errors
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
      case 0: error = MIOS32_USB_MIDI_PackageReceive(&package); port = USB0 + package.cable; break;
#else
      case 0: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART0_ASSIGNMENT == 1
      case 1: error = MIOS32_UART_MIDI_PackageReceive(0, &package); port = UART0; break;
#else
      case 1: error = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART1_ASSIGNMENT == 1
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
	if( callback_event != NULL )
	  callback_event(port, package);
      } else {
	switch( package.type ) {
  	case 0x0: // reserved, ignore
	case 0x1: // cable events, ignore
	  break;

	case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc. 
	case 0x3: // Three-byte System Common messages like SPP, etc. 
	  if( callback_event != NULL )
	    callback_event(port, package); // -> forwarded as event
	  break;
	case 0x4: // SysEx starts or continues (3 bytes)
	  if( callback_sysex != NULL ) {
	    callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	    callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	    callback_sysex(port, package.evnt2); // -> forwarded as SysEx
	  }
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt0); // -> forward to MIOS32 SysEx Parser
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt1); // -> forward to MIOS32 SysEx Parser
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt2); // -> forward to MIOS32 SysEx Parser
	  break;
	case 0x5: // Single-byte System Common Message or SysEx ends with following single byte. 
	  if( package.evnt0 >= 0xf8 ) {
	    if( callback_event != NULL )
	      callback_event(port, package); // -> forwarded as event
	  } else {
	    if( callback_sysex != NULL )
	      callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	    MIOS32_MIDI_SYSEX_Parser(port, package.evnt0); // -> forward to MIOS32 SysEx Parser
	  }
	  break;
	case 0x6: // SysEx ends with following two bytes.
	  if( callback_sysex != NULL ) {
	    callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	    callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	  }
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt0); // -> forward to MIOS32 SysEx Parser
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt1); // -> forward to MIOS32 SysEx Parser
	  break;
	case 0x7: // SysEx ends with following three bytes.
	  if( callback_sysex != NULL ) {
	    callback_sysex(port, package.evnt0); // -> forwarded as SysEx
	    callback_sysex(port, package.evnt1); // -> forwarded as SysEx
	    callback_sysex(port, package.evnt2); // -> forwarded as SysEx
	  }
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt0); // -> forward to MIOS32 SysEx Parser
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt1); // -> forward to MIOS32 SysEx Parser
	  MIOS32_MIDI_SYSEX_Parser(port, package.evnt2); // -> forward to MIOS32 SysEx Parser
	  break;
	}
      }
    }
  } while( again );

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs Rx/Tx callback functions which are executed immediately on each
//! incoming/outgoing MIDI byte, partly from interrupt handlers with following
//! parameters:
//! \code
//!    callback_rx(mios32_midi_port_t port, u8 midi_byte);
//!    callback_tx(mios32_midi_port_t port, u8 midi_byte);
//! \endcode
//!
//! These functions should be executed so fast as possible. They can be used
//! to trigger MIDI Rx/Tx LEDs or to trigger on MIDI clock events. In order to
//! avoid MIDI buffer overruns, the max. recommented execution time is 100 uS!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DirectRxTxCallback_Init(void *callback_rx, void *callback_tx)
{
  direct_rx_callback_func = callback_rx;
  direct_tx_callback_func = callback_tx;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI bytes to the Rx Callback routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendByteToRxCallback(mios32_midi_port_t port, u8 midi_byte)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_rx_callback_func != NULL )
    direct_rx_callback_func(port, midi_byte);
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI packages to the Rx Callback routine (byte by byte)
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_package MIDI package
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackageToRxCallback(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_rx_callback_func != NULL ) {
    u8 buffer[3] = {midi_package.evnt0, midi_package.evnt1, midi_package.evnt2};
    int len = mios32_midi_pcktype_num_bytes[midi_package.cin];
    int i;
    for(i=0; i<len; ++i)
      direct_rx_callback_func(port, buffer[i]);
  }
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI bytes to the Tx Callback routine
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_byte MIDI byte
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendByteToTxCallback(mios32_midi_port_t port, u8 midi_byte)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_tx_callback_func != NULL )
    direct_tx_callback_func(port, midi_byte);
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI packages to the Tx Callback routine (byte by byte)
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] midi_package MIDI package
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackageToTxCallback(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_tx_callback_func != NULL ) {
    u8 buffer[3] = {midi_package.evnt0, midi_package.evnt1, midi_package.evnt2};
    int len = mios32_midi_pcktype_num_bytes[midi_package.cin];
    int i;
    for(i=0; i<len; ++i)
      direct_tx_callback_func(port, buffer[i]);
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function allows to change the DEFAULT port.<BR>
//! The preset which will be used after application reset can be set in
//! mios32_config.h via "#define MIOS32_MIDI_DEFAULT_PORT <port>".<BR>
//! It's set to USB0 so long not overruled in mios32_config.h
//! \param[in] port MIDI port (USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DefaultPortSet(mios32_midi_port_t port)
{
  if( port == DEFAULT ) // avoid recursion
    return -1;

  default_port = port;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the DEFAULT port
//! \return the default port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t MIOS32_MIDI_DefaultPortGet(void)
{
  return default_port;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets the SysEx Device ID, which is used during parsing
//! incoming SysEx Requests to MIOS32<BR>
//! It can also be used by an application for additional parsing with the same ID.<BR>
//! ID changes will get lost after reset. It can be changed permanently by the
//! user via Bootloader Command 0x0c
//! \param[in] device_id a new (temporary) device ID (0x00..0x7f)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DeviceIDSet(u8 device_id)
{
  sysex_device_id = device_id & 0x7f;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the SysEx Device ID, which is used during parsing
//! incoming SysEx Requests to MIOS32<BR>
//! It can also be used by an application for additional parsing with the same ID.<BR>
//! The initial ID is stored inside the BSL range and will be recovered after
//! reset. It can be changed by the user via Bootloader Command 0x0c
//! \return SysEx device ID (0x00..0x7f)
//! \todo store device ID via BSL based EEPROM emulation
/////////////////////////////////////////////////////////////////////////////
u8 MIOS32_MIDI_DeviceIDGet(void)
{
  return sysex_device_id;
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // TODO: here we could send an error notification, that multiple devices are trying to access the device
  if( sysex_state.MY_SYSEX && port != last_sysex_port )
    return -1;

  last_sysex_port = port;

  // branch depending on state
  if( !sysex_state.MY_SYSEX ) {
    if( (sysex_state.CTR < sizeof(mios32_midi_sysex_header) && midi_in != mios32_midi_sysex_header[sysex_state.CTR]) ||
	(sysex_state.CTR == sizeof(mios32_midi_sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      MIOS32_MIDI_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.CTR > sizeof(mios32_midi_sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.CMD ) {
      	MIOS32_MIDI_SYSEX_Cmd(port, MIOS32_MIDI_SYSEX_CMD_STATE_END, midi_in);
      }
      MIOS32_MIDI_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.CMD ) {
	sysex_state.CMD = 1;
	sysex_cmd = midi_in;
	MIOS32_MIDI_SYSEX_Cmd(port, MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	MIOS32_MIDI_SYSEX_Cmd(port, MIOS32_MIDI_SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called at the end of a sysex command or on 
// an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  // enter the commands here
  switch( sysex_cmd ) {
    case 0x00:
      MIOS32_MIDI_SYSEX_Cmd_Query(port, cmd_state, midi_in);
      break;
    case 0x0f:
      MIOS32_MIDI_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;
    default:
#if MIOS32_MIDI_BSL_ENHANCEMENTS
      // this compile switch should only be activated for the bootloader!
      if( BSL_SYSEX_Cmd(port, cmd_state, midi_in, sysex_cmd) >= 0 )
	return 0; // BSL has serviced this command - no error
#endif
      // unknown command
      // TODO: send 0xf7 if merger enabled
      MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND);
      MIOS32_MIDI_SYSEX_CmdFinished();      
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Command 00: Query core informations and request BSL entry
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Cmd_Query(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 query_req = 0;
  char str_buffer[40];

  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      query_req = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      query_req = midi_in;
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      switch( query_req ) {
        case 0x01: // operating system
	  MIOS32_MIDI_SYSEX_SendAckStr(port, "MIOS32");
	  break;
        case 0x02: // MBHP Board
	  MIOS32_MIDI_SYSEX_SendAckStr(port, MIOS32_BOARD_STR);
	  break;
        case 0x03: // Core Family
	  MIOS32_MIDI_SYSEX_SendAckStr(port, MIOS32_FAMILY_STR);
	  break;
        case 0x04: // Chip ID
	  sprintf(str_buffer, "%08x", MIOS32_SYS_ChipIDGet());
	  MIOS32_MIDI_SYSEX_SendAckStr(port, (char *)str_buffer);
	  break;
        case 0x05: // Serial Number
	  if( MIOS32_SYS_SerialNumberGet((char *)str_buffer) >= 0 )
	    MIOS32_MIDI_SYSEX_SendAckStr(port, str_buffer);
	  else
	    MIOS32_MIDI_SYSEX_SendAckStr(port, "?");
	  break;
        case 0x06: // Flash Memory Size
	  sprintf(str_buffer, "%d", MIOS32_SYS_FlashSizeGet());
	  MIOS32_MIDI_SYSEX_SendAckStr(port, str_buffer);
	  break;
        case 0x07: // RAM Memory Size
	  sprintf(str_buffer, "%d", MIOS32_SYS_RAMSizeGet());
	  MIOS32_MIDI_SYSEX_SendAckStr(port, str_buffer);
	  break;
        case 0x08: // Application Name Line #1
	  MIOS32_MIDI_SYSEX_SendAckStr(port, MIOS32_LCD_BOOT_MSG_LINE1);
	  break;
        case 0x09: // Application Name Line #2
	  MIOS32_MIDI_SYSEX_SendAckStr(port, MIOS32_LCD_BOOT_MSG_LINE2);
	  break;
        case 0x7f:
	  // reset core (this will send an upload request)
	  MIOS32_SYS_Reset();
	  // at least on STM32 we will never reach this point
	  // but other core families could contain an empty stumb!
	  break;
        default: 
	  // unknown query
	  MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_UNKNOWN_QUERY);
      }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge)
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      // nothing to do
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      // nothing to do
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      // send acknowledge
      MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends a SysEx acknowledge to notify the user about the received command
// expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}

/////////////////////////////////////////////////////////////////////////////
// This function sends an SysEx acknowledge with a string (used on queries)
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_SendAckStr(mios32_midi_port_t port, char *str)
{
  u8 sysex_buffer[128]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // send ack code
  *sysex_buffer_ptr++ = MIOS32_MIDI_SYSEX_ACK;

  // send string
  for(i=0; i<100 && str[i] != 0; ++i)
    *sysex_buffer_ptr++ = str[i];

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}

#endif /* MIOS32_DONT_USE_MIDI */
