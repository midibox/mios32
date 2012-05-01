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
#include <string.h>
#include <stdarg.h>

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
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned PING_BYTE_RECEIVED;
  };
} sysex_state_t;


typedef union {
  struct {
    unsigned ALL:32;
  };

  struct {
    unsigned usb_receives:16;
    unsigned iic_receives:16;
  };
} sysex_timeout_ctr_flags_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mios32_midi_port_t default_port = MIOS32_MIDI_DEFAULT_PORT;
static mios32_midi_port_t debug_port   = MIOS32_MIDI_DEBUG_PORT;

static s32 (*direct_rx_callback_func)(mios32_midi_port_t port, u8 midi_byte);
static s32 (*direct_tx_callback_func)(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 (*sysex_callback_func)(mios32_midi_port_t port, u8 sysex_byte);
static s32 (*timeout_callback_func)(mios32_midi_port_t port);
static s32 (*debug_command_callback_func)(mios32_midi_port_t port, char c);

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;
static mios32_midi_port_t last_sysex_port = DEFAULT;


// SysEx timeout counter: in order to save memory and execution time, we only
// protect a single SysEx stream for packet oriented interfaces.
// Serial interfaces (UART) are protected separately -> see MIOS32_UART_MIDI_PackageReceive
// Approach: the first interface which sends F0 resets the timeout counter.
// The flag is reset with F7
// Once one second has passed, and the flag is still set, MIOS32_MIDI_TimeOut() will
// be called to notify the failure.
// Drawback: if another interface starts a SysEx transfer at the same time, the stream
// will be ignored, and a timeout won't be detected.
// It's unlikely that this is an issue in practice, especially if SysEx parsers only
// take streams of the first interface which sends F0 like MIOS32_MIDI_SYSEX_Parser()...
//
// If a stronger protection is desired (SysEx parser handles streams of multiple interfaces),
// it's recommented to implement a separate timeout mechanism at application side.
static u16 sysex_timeout_ctr;
static sysex_timeout_ctr_flags_t sysex_timeout_ctr_flags;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIOS32_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_CmdFinished(void);
static s32 MIOS32_MIDI_SYSEX_Cmd(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_Cmd_Query(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_Cmd_Debug(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 MIOS32_MIDI_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);
static s32 MIOS32_MIDI_SYSEX_SendAckStr(mios32_midi_port_t port, char *str);
static s32 MIOS32_MIDI_TimeOut(mios32_midi_port_t port);


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

  // set default/debug port as defined in mios32.h/mios32_config.h
  default_port = MIOS32_MIDI_DEFAULT_PORT;
  debug_port = MIOS32_MIDI_DEBUG_PORT;

  // disable callback functions
  direct_rx_callback_func = NULL;
  direct_tx_callback_func = NULL;
  sysex_callback_func = NULL;
  timeout_callback_func = NULL;
  debug_command_callback_func = NULL;

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

  sysex_device_id = 0x00;
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  // read from bootloader info range
  u8 *device_id_confirm = (u8 *)MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM;
  u8 *device_id = (u8 *)MIOS32_SYS_ADDR_DEVICE_ID;
  if( *device_id_confirm == 0x42 && *device_id < 0x80 )
    sysex_device_id = *device_id;
#endif

  // SysEx timeout mechanism
  sysex_timeout_ctr = 0;
  sysex_timeout_ctr_flags.ALL = 0;

  return -ret;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a MIDI port
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \return 1: port available
//! \return 0: port not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_CheckAvailable(port & 0xf);
#else
      return 0; // USB has been disabled
#endif

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_CheckAvailable(port & 0xf);
#else
      return 0; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_CheckAvailable(port & 0xf);
#else
      return 0; // IIC_MIDI has been disabled
#endif
  }

  return 0; // invalid port
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables/disables running status optimisation for a given
//! MIDI OUT port to improve bandwidth if MIDI events with the same
//! status byte are sent back-to-back.<BR>
//! The optimisation is currently only used for UART based port (enabled by
//! default), IIC: TODO, USB: not required).
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \param[in] enable 0=optimisation disabled, 1=optimisation enabled
//! \return -1 if port not available or if it doesn't support running status
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_RS_OptimisationSet(mios32_midi_port_t port, u8 enable)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
      return -1; // not required for USB

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_RS_OptimisationSet(port & 0xf, enable);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_RS_OptimisationSet(port & 0xf, enable);
#else
      return -1; // IIC_MIDI has been disabled
#endif
  }

  return -1; // invalid port
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the running status optimisation enable/disable flag
//! for the given MIDI OUT port.
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \return -1 if port not available or if it doesn't support running status
//! \return 0 if optimisation disabled
//! \return 1 if optimisation enabled
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_RS_OptimisationGet(mios32_midi_port_t port)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
      return -1; // not required for USB

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_RS_OptimisationGet(port & 0xf);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_RS_OptimisationGet(port & 0xf);
#else
      return -1; // IIC_MIDI has been disabled
#endif
  }

  return -1; // invalid port
}


/////////////////////////////////////////////////////////////////////////////
//! This function resets the current running status, so that it will be sent
//! again with the next MIDI Out package.
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \return -1 if port not available or if it doesn't support running status
//! \return 0 if optimisation disabled
//! \return 1 if optimisation enabled
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_RS_Reset(mios32_midi_port_t port)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
      return -1; // not required for USB

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_RS_Reset(port & 0xf);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_RS_Reset(port & 0xf);
#else
      return -1; // IIC_MIDI has been disabled
#endif
  }

  return -1; // invalid port
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a package over given port
//!
//! This is a low level function. In difference to other MIOS32_MIDI_Send* functions,
//! It allows to send packages in non-blocking mode (caller has to retry if -2 is returned)
//!
//! Before the package is forwarded, an optional Tx Callback function will be called
//! which allows to filter/monitor/route the package, or extend the MIDI transmitter
//! by custom MIDI Output ports (e.g. for internal busses, OSC, AOUT, etc.)
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \param[in] package MIDI package
//! \return -1 if port not available
//! \return -2 buffer is full
//!         caller should retry until buffer is free again
//! \return -3 Tx Callback reported an error
//! \return 1 if package has been filtered by Tx callback
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage_NonBlocking(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // forward to Tx callback function and break if package has been filtered
  if( direct_tx_callback_func != NULL ) {
    s32 status;
    if( (status=direct_tx_callback_func(port, package)) )
      return status;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_PackageSend_NonBlocking(package);
#else
      return -1; // USB has been disabled
#endif

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_PackageSend_NonBlocking(package.cable, package);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_PackageSend_NonBlocking(package.cable, package);
#else
      return -1; // IIC_MIDI has been disabled
#endif
      
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
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \param[in] package MIDI package
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == MIDI_DEBUG) ? debug_port : default_port;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  // forward to Tx callback function and break if package has been filtered
  if( direct_tx_callback_func != NULL ) {
    s32 status;
    if( (status=direct_tx_callback_func(port, package)) )
      return status;
  }

  // branch depending on selected port
  switch( port & 0xf0 ) {
    case USB0://..15
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      return MIOS32_USB_MIDI_PackageSend(package);
#else
      return -1; // USB has been disabled
#endif

    case UART0://..15
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI)
      return MIOS32_UART_MIDI_PackageSend(package.cable, package);
#else
      return -1; // UART_MIDI has been disabled
#endif

    case IIC0://..15
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      return MIOS32_IIC_MIDI_PackageSend(package.cable, package);
#else
      return -1; // IIC_MIDI has been disabled
#endif
      
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
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
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

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc_number, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xb0 | chn, cc_number,   val); }

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
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
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

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfb, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfc, 0x00, 0x00); }

s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfe, 0x00, 0x00); }

s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xff, 0x00, 0x00); }


/////////////////////////////////////////////////////////////////////////////
//! Sends a SysEx Stream
//!
//! This function is provided for a more comfortable use model
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
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
//! Sends a formatted Debug Message to the MIOS Terminal in MIOS Studio.
//!
//! Formatting parameters are like known from printf, e.g.
//! \code
//!   MIOS32_MIDI_SendDebugMessage("Button %d %s\n", button, value ? "depressed" : "pressed");
//! \endcode
//!
//! The MIDI port used for debugging (MIDI_DEBUG) can be declared in mios32_config.h:
//! \code
//!   #define MIOS32_MIDI_DEBUG_PORT USB0
//! \endcode
//! (USB0 is the default value)
//!
//! Optionally, the port can be changed during runtime with MIOS32_MIDI_DebugPortSet
//!
//! Please note that the resulting string shouldn't be longer than 128 characters!<BR>
//! If the *format string is already longer than 100 characters an error message will
//! be sent to notify about the programming error.<BR>
//! The limit is set to save allocated stack memory! Just reduce the formated string to
//! print out the intended message.
//! \param[in] *format zero-terminated format string - 128 characters supported maximum!
//! \param ... additional arguments
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendDebugMessage(char *format, ...)
{
#ifdef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
  // for bootloader to save memory
  return -1;
#else
  u8 buffer[128+5+3+1]; // 128 chars allowed + 5 for header + 3 for command + F7
  va_list args;
  int i;

  // failsave: if format string is longer than 100 chars, break here
  // note that this is a weak protection: if %s is used, or a lot of other format tokens,
  // the resulting string could still lead to a buffer overflow
  // other the other hand we don't want to allocate too many byte for buffer[] to save stack
  char *str = (char *)((size_t)buffer+sizeof(mios32_midi_sysex_header)+3);
  if( strlen(format) > 100 ) {
    // exit with less costly message
    return MIOS32_MIDI_SendDebugString("(ERROR: string passed to MIOS32_MIDI_SendDebugMessage() is longer than 100 chars!\n");
  } else {
    // transform formatted string into string
    va_start(args, format);
    vsprintf(str, format, args);
  }

  u8 *sysex_buffer_ptr = buffer;
  for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
    *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

  // debug message: ack code
  *sysex_buffer_ptr++ = MIOS32_MIDI_SYSEX_DEBUG;

  // command identifier
  *sysex_buffer_ptr++ = 0x40; // output string

  // search end of string and determine length
  u16 len = sizeof(mios32_midi_sysex_header) + 3;
  for(i=0; i<128 && (*sysex_buffer_ptr != 0); ++i) {
    *sysex_buffer_ptr++ &= 0x7f; // ensure that MIDI protocol won't be violated
    ++len;
  }

  // send footer
  *sysex_buffer_ptr++ = 0xf7;
  ++len;

  return MIOS32_MIDI_SendSysEx(debug_port, buffer, len);
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a string to the MIOS Terminal in MIOS Studio.
//!
//! In distance to MIOS32_MIDI_SendDebugMessage this version is less costly (it
//! doesn't consume so much stack space), but the string must already be prepared.
//!
//! Example:
//! \code
//!   MIOS32_MIDI_SendDebugString("ERROR: something strange happened in myFunction()!");
//! \endcode
//!
//! The string size isn't limited.
//!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendDebugString(char *str)
{
#ifdef MIOS32_MIDI_DISABLE_DEBUG_MESSAGE
  // for bootloader to save memory
  return -1;
#else
  s32 status = 0;
  mios32_midi_package_t package;

// unfortunately doesn't work, and runtime check would be unnecessary costly
//#if sizeof(mios32_midi_sysex_header) != 5
//# error "Please adapt MIOS32_MIDI_SendDebugString"
//#endif

  package.type = 0x4; // SysEx starts or continues
  package.evnt0 = mios32_midi_sysex_header[0];
  package.evnt1 = mios32_midi_sysex_header[1];
  package.evnt2 = mios32_midi_sysex_header[2];
  status |= MIOS32_MIDI_SendPackage(debug_port, package);

  package.type = 0x4; // SysEx starts or continues
  package.evnt0 = mios32_midi_sysex_header[3];
  package.evnt1 = mios32_midi_sysex_header[4];
  package.evnt2 = MIOS32_MIDI_DeviceIDGet();
  status |= MIOS32_MIDI_SendPackage(debug_port, package);

  package.type = 0x4; // SysEx starts or continues
  package.evnt0 = MIOS32_MIDI_SYSEX_DEBUG;
  package.evnt1 = 0x40; // output string
  package.evnt2 = str[0]; // will be 0x00 if string already ends (""), thats ok, MIOS Studio can handle this
  status |= MIOS32_MIDI_SendPackage(debug_port, package);

  u32 len = strlen(str);
  if( len > 0 ) {
    int i = 1; // because str[0] has already been sent
    for(i=1; i<len; i+=3) {
      u8 b;
      u8 terminated = 0;

      package.type = 0x4; // SysEx starts or continues
      if( (b=str[i+0]) ) {
	package.evnt0 = b & 0x7f;
      } else {
	package.evnt0 = 0x00;
	terminated = 1;
      }

      if( !terminated && (b=str[i+1]) ) {
	package.evnt1 = b & 0x7f;
      } else {
	package.evnt1 = 0x00;
	terminated = 1;
      }

      if( !terminated && (b=str[i+2]) ) {
	package.evnt2 = b & 0x7f;
      } else {
	package.evnt2 = 0x00;
	terminated = 1;
      }

      status |= MIOS32_MIDI_SendPackage(debug_port, package);
    }
  }

  package.type = 0x5; // SysEx ends with following single byte. 
  package.evnt0 = 0xf7;
  package.evnt1 = 0x00;
  package.evnt2 = 0x00;
  status |= MIOS32_MIDI_SendPackage(debug_port, package);

  return status;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Sends an hex dump (formatted representation of memory content) to the 
//! MIOS Terminal in MIOS Studio.
//!
//! The MIDI port used for debugging (MIDI_DEBUG) can be declared in mios32_config.h:
//! \code
//!   #define MIOS32_MIDI_DEBUG_PORT USB0
//! \endcode
//! (USB0 is the default value)
//!
//! Optionally, the port can be changed during runtime with MIOS32_MIDI_DebugPortSet
//! \param[in] *src pointer to memory location which should be dumped
//! \param[in] len number of bytes which should be sent
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendDebugHexDump(u8 *src, u32 len)
{
  // check if any byte has to be sent
  if( !len )
    return 0;

  // send hex dump line by line
  u8 *src_begin = src;
  u8 *src_end;
  for(src_end=(u8 *)((size_t)src + len - 1); src < src_end;) {
    u8 buffer[80+5+3+1]; // we need at least 8+2+3*16+2+16+1 chars + 5 for header + 3 for command + F7
    int i;

    // create SysEx header
    u8 *sysex_buffer_ptr = buffer;
    for(i=0; i<sizeof(mios32_midi_sysex_header); ++i)
      *sysex_buffer_ptr++ = mios32_midi_sysex_header[i];

    // device ID
    *sysex_buffer_ptr++ = MIOS32_MIDI_DeviceIDGet();

    // debug message: ack code
    *sysex_buffer_ptr++ = MIOS32_MIDI_SYSEX_DEBUG;

    // command identifier
    *sysex_buffer_ptr++ = 0x40; // output string

    // build line:
    // add source address
    sprintf((char *)sysex_buffer_ptr, "%08X ", (u32)(src-src_begin));
    sysex_buffer_ptr += 9;

    // add up to 16 bytes
    u8 *src_chars = src; // for later
    for(i=0; i<16; ++i) {
      sprintf((char *)sysex_buffer_ptr, (src <= src_end) ? " %02X" : "   ", *src);
      sysex_buffer_ptr += 3;

      ++src;
    }

    // add two spaces
    for(i=0; i<2; ++i)
      *sysex_buffer_ptr++ = ' ';

    // add characters
    for(i=0; i<16; ++i) {
      if( *src_chars < 32 || *src_chars >= 128 )
	*sysex_buffer_ptr++ = '.';
      else
	*sysex_buffer_ptr++ = *src_chars;

      if( src_chars == src_end )
	break;

      ++src_chars;
    }

    // linebreak
    *sysex_buffer_ptr++ = '\n';

    // add F7
    *sysex_buffer_ptr++ = 0xf7;

    s32 status = MIOS32_MIDI_SendSysEx(debug_port, buffer, (u32)(sysex_buffer_ptr-buffer));
    if( status < 0 )
      return status;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Checks for incoming MIDI messages amd calls callback_package function
//! with following parameters:
//! \code
//!    callback_package(mios32_midi_port_t port, mios32_midi_package_t midi_package)
//! \endcode
//!
//! Not for use in an application - this function is called by
//! by a task in the programming model, callback_package is APP_MIDI_NotifyPackage()
//!
//! SysEx streams can be optionally redirected to a separate callback function 
//! which can be installed via MIOS32_MIDI_SysExCallback_Init()
//!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Receive_Handler(void *_callback_package)
{
  u8 port;
  mios32_midi_package_t package;

  void (*callback_package)(mios32_midi_port_t port, mios32_midi_package_t midi_package) = _callback_package;

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
    s32 status = -1;
    switch( intf++ ) {
#if !defined(MIOS32_DONT_USE_USB) && !defined(MIOS32_DONT_USE_USB_MIDI)
      case 0: status = MIOS32_USB_MIDI_PackageReceive(&package); port = USB0 + package.cable; break;
#else
      case 0: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART0_ASSIGNMENT == 1
      case 1: status = MIOS32_UART_MIDI_PackageReceive(0, &package); port = UART0; break;
#else
      case 1: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART1_ASSIGNMENT == 1
      case 2: status = MIOS32_UART_MIDI_PackageReceive(1, &package); port = UART1; break;
#else
      case 2: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART2_ASSIGNMENT == 1
      case 3: status = MIOS32_UART_MIDI_PackageReceive(2, &package); port = UART2; break;
#else
      case 3: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && !defined(MIOS32_DONT_USE_UART_MIDI) && MIOS32_UART3_ASSIGNMENT == 1
      case 4: status = MIOS32_UART_MIDI_PackageReceive(3, &package); port = UART3; break;
#else
      case 4: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 5: status = MIOS32_IIC_MIDI_PackageReceive(0, &package); port = IIC0; break;
#else
      case 5: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 6: status = MIOS32_IIC_MIDI_PackageReceive(1, &package); port = IIC1; break;
#else
      case 6: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 7: status = MIOS32_IIC_MIDI_PackageReceive(2, &package); port = IIC2; break;
#else
      case 7: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 8: status = MIOS32_IIC_MIDI_PackageReceive(3, &package); port = IIC3; break;
#else
      case 8: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 9: status = MIOS32_IIC_MIDI_PackageReceive(4, &package); port = IIC4; break;
#else
      case 9: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 10: status = MIOS32_IIC_MIDI_PackageReceive(5, &package); port = IIC5; break;
#else
      case 10: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 11: status = MIOS32_IIC_MIDI_PackageReceive(6, &package); port = IIC6; break;
#else
      case 11: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_IIC) && !defined(MIOS32_DONT_USE_IIC_MIDI)
      case 12: status = MIOS32_IIC_MIDI_PackageReceive(7, &package); port = IIC7; break;
#else
      case 12: status = -1; break;
#endif
      default:
	// allow 10 forwards maximum to yield some CPU time for other tasks
	if( packages_forwarded && total_packages_forwarded < 10 ) {
	  intf = 0; // restart with USB
	  packages_forwarded = 0; // for checking, if packages still have been forwarded in next round
	} else {
	  again = 0; // no more interfaces to be processed
	}
	status = -1; // empty round - no message
    }

    // timeout detected by interface?
    if( status == -10 ) {
      MIOS32_MIDI_TimeOut(port);
      again = 0;
    } else if( status >= 0 ) { // message received?
      // notify that a package has been forwarded
      ++packages_forwarded;
      ++total_packages_forwarded;

      // remove cable number from package (MIOS32_MIDI passes it's own port number)
      package.cable = 0;

      // branch depending on package type
      if( package.type >= 0x8 && package.type < 0xf ) {
	if( callback_package != NULL )
	  callback_package(port, package);
      } else {
	u8 filter_sysex = 0;

	switch( package.type ) {
  	  case 0x0: // reserved, ignore
	  case 0x1: // cable events, ignore
	    break;

	  case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc. 
	  case 0x3: // Three-byte System Common messages like SPP, etc. 
	    if( callback_package != NULL )
	      callback_package(port, package); // -> forwarded as event
	    break;

	  case 0x4: // SysEx starts or continues (3 bytes)
	  case 0xf: // Single byte is interpreted as SysEx as well (I noticed that portmidi sometimes sends single bytes!)

	    if( package.evnt0 >= 0xf8 ) { // relevant for package type 0xf
	      if( callback_package != NULL )
		callback_package(port, package); // -> realtime event is forwarded as event
	      break;
	    }

	    if( package.evnt0 == 0xf0 ) {
	      // cheap timeout mechanism - see comments above the sysex_timeout_ctr declaration
	      if( !sysex_timeout_ctr_flags.ALL ) {
		switch( port & 0xf0 ) {
		  case USB0://..15
		    sysex_timeout_ctr = 0;
		    sysex_timeout_ctr_flags.usb_receives = (1 << (port & 0xf));
		    break;
		  case UART0://..15
		    // already done in MIOS32_UART_MIDI_PackageReceive()
		    break;
		  case IIC0://..15
		    sysex_timeout_ctr = 0;
		    sysex_timeout_ctr_flags.iic_receives = (1 << (port & 0xf));
		    break;
		    // no timeout protection for remaining interfaces (yet)
		}
	      }
	    }

	    MIOS32_MIDI_SYSEX_Parser(port, package.evnt0); // -> forward to MIOS32 SysEx Parser
	    if( package.type != 0x0f ) {
	      MIOS32_MIDI_SYSEX_Parser(port, package.evnt1); // -> forward to MIOS32 SysEx Parser
	      MIOS32_MIDI_SYSEX_Parser(port, package.evnt2); // -> forward to MIOS32 SysEx Parser
	    }

	    if( sysex_callback_func != NULL ) {
	      filter_sysex |= sysex_callback_func(port, package.evnt0); // -> forwarded as SysEx
	      if( package.type != 0x0f ) {
		filter_sysex |= sysex_callback_func(port, package.evnt1); // -> forwarded as SysEx
		filter_sysex |= sysex_callback_func(port, package.evnt2); // -> forwarded as SysEx
	      }
	    }

	    if( callback_package != NULL && !filter_sysex )
	      callback_package(port, package);

	    break;

	  case 0x5:   // Single-byte System Common Message or SysEx ends with following single byte. 
	    if( package.evnt0 >= 0xf8 ) {
	      if( callback_package != NULL )
		callback_package(port, package); // -> forwarded as event
	      break;
	    }
	    // no >= 0xf8 event: continue!

	  case 0x6:   // SysEx ends with following two bytes.
	  case 0x7: { // SysEx ends with following three bytes.
	    u8 num_bytes = package.type - 0x5 + 1;
	    u8 current_byte = 0;

	    if( num_bytes >= 1 ) {
	      current_byte = package.evnt0;
	      MIOS32_MIDI_SYSEX_Parser(port, current_byte); // -> forward to MIOS32 SysEx Parser
	      if( sysex_callback_func != NULL )
		filter_sysex |= sysex_callback_func(port, current_byte); // -> forwarded as SysEx
	    }

	    if( num_bytes >= 2 ) {
	      current_byte = package.evnt1;
	      MIOS32_MIDI_SYSEX_Parser(port, current_byte); // -> forward to MIOS32 SysEx Parser
	      if( sysex_callback_func != NULL )
		filter_sysex |= sysex_callback_func(port, current_byte); // -> forwarded as SysEx
	    }

	    if( num_bytes >= 3 ) {
	      current_byte = package.evnt2;
	      MIOS32_MIDI_SYSEX_Parser(port, current_byte); // -> forward to MIOS32 SysEx Parser
	      if( sysex_callback_func != NULL )
		filter_sysex |= sysex_callback_func(port, current_byte); // -> forwarded as SysEx
	    }

	    // reset timeout protection if required
	    if( current_byte == 0xf7 )
	      sysex_timeout_ctr_flags.ALL = 0;

	    // forward as package if not filtered
	    if( callback_package != NULL && !filter_sysex )
	      callback_package(port, package);
	    
	  } break;
	}	  
      }
    }

    // timeout detected by this handler?
    if( sysex_timeout_ctr_flags.ALL && sysex_timeout_ctr > 1000 ) {
      u8 timeout_port = 0;

      // determine port
      if( sysex_timeout_ctr_flags.usb_receives ) {
	int i; // i'm missing a prio instruction in C!
	for(i=0; i<16; ++i)
	  if( sysex_timeout_ctr_flags.usb_receives & (1 << i) )
	    break;
	if( i >= 16 ) // failsafe
	  i = 0;
	timeout_port = USB0 + i;
      } else if( sysex_timeout_ctr_flags.iic_receives ) {
	int i; // i'm missing a prio instruction in C!
	for(i=0; i<16; ++i)
	  if( sysex_timeout_ctr_flags.iic_receives & (1 << i) )
	    break;
	if( i >= 16 ) // failsafe
	  i = 0;
	timeout_port = IIC0 + i;
      }

      MIOS32_MIDI_TimeOut(timeout_port);
      sysex_timeout_ctr_flags.ALL = 0;
      again = 0;
    }
  } while( again );

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to handle timeout
//! and expire counters.
//!
//! Not for use in an application - this function is called by
//! by a task in the programming model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_Periodic_mS(void)
{
  s32 status = 0;

#ifndef MIOS32_DONT_USE_USB_MIDI
  status |= MIOS32_USB_MIDI_Periodic_mS();
#endif

#ifndef MIOS32_DONT_USE_UART_MIDI
  status |= MIOS32_UART_MIDI_Periodic_mS();
#endif

#ifndef MIOS32_DONT_USE_IIC_MIDI
  status |= MIOS32_IIC_MIDI_Periodic_mS();
#endif

  // increment timeout counter for incoming packages
  // an incomplete event will be timed out after 1000 ticks (1 second)
  if( sysex_timeout_ctr < 65535 )
    ++sysex_timeout_ctr;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs the Tx callback function which is executed by
//! MIOS32_MIDI_SendPackage_NonBlocking() before the MIDI package will be
//! forwarded to the physical interface.
//!
//! The callback allows following usecases:
//! <UL>
//!   <LI>package filter
//!   <LI>duplicating/routing packages
//!   <LI>monitoring packages (sniffer)
//!   <LI>create virtual busses; loopbacks
//!   <LI>extend available ports (e.g. by an OSC or AOUT port)<BR>
//!       It is recommented to give port extensions a port number >= 0x80 to
//!       avoid incompatibility with future MIOS32 port extensions.
//! </UL>
//! \param[in] *callback_tx pointer to callback function:<BR>
//! \code
//!    s32 callback_tx(mios32_midi_port_t port, mios32_midi_package_t package)
//!    {
//!    }
//! \endcode
//! The package will be forwarded to the physical interface if the function 
//! returns 0.<BR>
//! Should return 1 to filter a package.
//! Should return -2 to initiate a retry (function will be called again)
//! Should return -3 to report any other error.
//! These error codes comply with MIOS32_MIDI_SendPackage_NonBlocking()
//! \return < 0 on errors
//! \note Please use the filtering capabilities with special care - if a port
//! is filtered which is also used for code upload, you won't be able to exchange
//! the erroneous code w/o starting the bootloader in hold mode after power-on.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DirectTxCallback_Init(s32 (*callback_tx)(mios32_midi_port_t port, mios32_midi_package_t package))
{
  direct_tx_callback_func = callback_tx;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs the Rx callback function which is executed immediately on each
//! incoming/outgoing MIDI byte, partly from interrupt handlers.
//!
//! This function should be executed so fast as possible. It can be used
//! to trigger MIDI Rx LEDs or to trigger on MIDI clock events. In order to
//! avoid MIDI buffer overruns, the max. recommented execution time is 100 uS!
//!
//! It is possible to filter incoming MIDI bytes with the return value of the
//! callback function.<BR>
//! \param[in] *callback_rx pointer to callback function:<BR>
//! \code
//!    s32 callback_rx(mios32_midi_port_t port, u8 midi_byte)
//!    {
//!    }
//! \endcode
//! The byte will be forwarded into the MIDI Rx queue if the function returns 0.<BR>
//! It will be filtered out if the callback returns != 0 (e.g. 1 for "filter", 
//! or -1 for "error").
//! \return < 0 on errors
//! \note Please use the filtering capabilities with special care - if a port
//! is filtered which is also used for code upload, you won't be able to exchange
//! the erroneous code w/o starting the bootloader in hold mode after power-on.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DirectRxCallback_Init(s32 (*callback_rx)(mios32_midi_port_t port, u8 midi_byte))
{
  direct_rx_callback_func = callback_rx;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI bytes to the Rx Callback routine.
//!
//! It shouldn't be used by applications.
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \param[in] midi_byte received MIDI byte
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendByteToRxCallback(mios32_midi_port_t port, u8 midi_byte)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_rx_callback_func != NULL )
    return direct_rx_callback_func(port, midi_byte);
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function is used by MIOS32 internal functions to forward received
//! MIDI packages to the Rx Callback routine (byte by byte)
//!
//! It shouldn't be used by applications.
//! \param[in] port MIDI port (DEFAULT, USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \param[in] midi_package received MIDI package
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SendPackageToRxCallback(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // note: here we could filter the user hook execution on special situations
  if( direct_rx_callback_func != NULL ) {
    u8 buffer[3] = {midi_package.evnt0, midi_package.evnt1, midi_package.evnt2};
    int len = mios32_midi_pcktype_num_bytes[midi_package.cin];
    int i;
    s32 status = 0;
    for(i=0; i<len; ++i)
      status |= direct_rx_callback_func(port, buffer[i]);
    return status;
  }
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function allows to change the DEFAULT port.<BR>
//! The preset which will be used after application reset can be set in
//! mios32_config.h via "#define MIOS32_MIDI_DEFAULT_PORT <port>".<BR>
//! It's set to USB0 as long as not overruled in mios32_config.h
//! \param[in] port MIDI port (USB0..USB7, UART0..UART3, IIC0..IIC7)
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
//! This function allows to change the MIDI_DEBUG port.<BR>
//! The preset which will be used after application reset can be set in
//! mios32_config.h via "#define MIOS32_MIDI_DEBUG_PORT <port>".<BR>
//! It's set to USB0 as long as not overruled in mios32_config.h
//! \param[in] port MIDI port (USB0..USB7, UART0..UART3, IIC0..IIC7)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DebugPortSet(mios32_midi_port_t port)
{
  if( port == MIDI_DEBUG ) // avoid recursion
    return -1;

  debug_port = port;
 
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the MIDI_DEBUG port
//! \return the debug port
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t MIOS32_MIDI_DebugPortGet(void)
{
  return debug_port;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets the SysEx Device ID, which is used during parsing
//! incoming SysEx Requests to MIOS32<BR>
//! It can also be used by an application for additional parsing with the same ID.<BR>
//! ID changes will get lost after reset. It can be changed permanently by the
//! user via the bootloader update tool
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
//! reset. It can be changed by the user with the bootloader update tool
//! \return SysEx device ID (0x00..0x7f)
/////////////////////////////////////////////////////////////////////////////
u8 MIOS32_MIDI_DeviceIDGet(void)
{
  return sysex_device_id;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs an optional SysEx callback which is called by 
//! MIOS32_MIDI_Receive_Handler() to simplify the parsing of SysEx streams.
//!
//! Without this callback (or with MIOS32_MIDI_SysExCallback_Init(NULL)),
//! SysEx messages are only forwarded to APP_MIDI_NotifyPackage() in chunks of 
//! 1, 2 or 3 bytes, tagged with midi_package.type == 0x4..0x7 or 0xf
//! 
//! In this case, the application has to take care for different transmission
//! approaches which are under control of the package sender. E.g., while Windows
//! uses Package Type 4..7 to transmit a SysEx stream, PortMIDI under MacOS sends 
//! a mix of 0xf (single byte) and 0x4 (continued 3-byte) packages instead.
//! 
//! By using the SysEx callback, the type of package doesn't play a role anymore,
//! instead the application can parse a serial stream.
//!
//! MIOS32 ensures, that realtime events (0xf8..0xff) are still forwarded to
//! APP_MIDI_NotifyPackage(), regardless if they are transmitted in a package
//! type 0x5 or 0xf, so that the SysEx parser doesn't need to filter out such
//! events, which could otherwise appear inside a SysEx stream.
//! 
//! \param[in] *callback_sysex pointer to callback function:<BR>
//! \code
//!    s32 callback_sysex(mios32_midi_port_t port, u8 sysex_byte)
//!    {
//!       //
//!       // .. parse stream
//!       //
//!     
//!       return 1; // don't forward package to APP_MIDI_NotifyPackage()
//!    }
//! \endcode
//! If the function returns 0, SysEx bytes will be forwarded to APP_MIDI_NotifyPackage() as well.
//! With return value != 0, APP_MIDI_NotifyPackage() won't get the already processed package.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_SysExCallback_Init(s32 (*callback_sysex)(mios32_midi_port_t port, u8 midi_in))
{
  sysex_callback_func = callback_sysex;

  return 0; // no error
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

  // USB upload is only allowed via USB0
  // this covers the scenario where other USB1..7 ports are used for MIDI Port forwarding, and a MIOS8 core
  // is connected to one of these ports
  // MIOS Studio reports "Detected MIOS8 and MIOS32 response - selection not supported yet!" in this case
  // By ignoring >= USB1 <= USB7 we have at least a workaround which works (for example) for MIDIbox LC
  if( port >= USB1 && port <= USB7 )
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
#if MIOS32_MIDI_BSL_ENHANCEMENTS
  // this compile switch should only be activated for the bootloader!
  if( BSL_SYSEX_Cmd(port, cmd_state, midi_in, sysex_cmd) >= 0 )
    return 0; // BSL has serviced this command - no error
#endif
  switch( sysex_cmd ) {
    case 0x00:
      MIOS32_MIDI_SYSEX_Cmd_Query(port, cmd_state, midi_in);
      break;
    case 0x0d:
      MIOS32_MIDI_SYSEX_Cmd_Debug(port, cmd_state, midi_in);
      break;
    case 0x0e: // ignore to avoid loopbacks
      break;
    case 0x0f:
      MIOS32_MIDI_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;
    default:
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
        case 0x02: // Board
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
#if MIOS32_MIDI_BSL_ENHANCEMENTS
	  // release halt state (or sending upload request) instead of reseting the core
	  BSL_SYSEX_ReleaseHaltState();
#else
	  // reset core (this will send an upload request)
	  MIOS32_SYS_Reset();
	  // at least on STM32 we will never reach this point
	  // but other core families could contain an empty stumb!
#endif
	  break;
        default: 
	  // unknown query
	  MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_UNKNOWN_QUERY);
      }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Command 0D: Debug Input/Output
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Cmd_Debug(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  static u8 debug_req = 0xff;

  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      debug_req = 0xff;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      if( debug_req == 0xff ) {
	debug_req = midi_in;
      } else {
	switch( debug_req ) {
	  case 0x00: // input string
	    if( debug_command_callback_func != NULL )
	      debug_command_callback_func(last_sysex_port, (char)midi_in);
	    break;

	  case 0x40: // output string
	    // not supported - DisAck will be sent
	    break;

	  default: // others
	    // not supported - DisAck will be sent
	    break;
	}
      }
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      if( debug_req == 0x00 ) {
	// send acknowledge
	MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

	if( debug_req == 0 && debug_command_callback_func == NULL ) {
	  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
	  MIOS32_MIDI_DebugPortSet(port);
	  MIOS32_MIDI_SendDebugString("[MIOS32_MIDI_SYSEX_Cmd_Debug] command handler not implemented by application\n");
	  MIOS32_MIDI_DebugPortSet(prev_debug_port);
	}

      } else {
	// send disacknowledge
	MIOS32_MIDI_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_UNSUPPORTED_DEBUG);
      }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_SYSEX_Cmd_Ping(mios32_midi_port_t port, mios32_midi_sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      sysex_state.PING_BYTE_RECEIVED = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      sysex_state.PING_BYTE_RECEIVED = 1;
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // TODO: send 0xf7 if merger enabled

      // send acknowledge if no additional byte has been received
      // to avoid feedback loop if two cores are directly connected
      if( !sysex_state.PING_BYTE_RECEIVED )
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
  for(i=0; i<100 && (str[i] != 0); ++i)
    *sysex_buffer_ptr++ = str[i];

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  return MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
}


/////////////////////////////////////////////////////////////////////////////
//! Installs the debug command callback function which is executed on incoming
//! characters from a MIOS Terminal
//!
//! Example:
//! \code
//! s32 CONSOLE_Parse(mios32_midi_port_t port, char c)
//! {
//!   // see $MIOS32_PATH/apps/examples/midi_console/
//!   
//!   return 0; // no error
//! }
//! \endcode
//!
//! The callback function has been installed in an Init() function with:
//! \code
//!   MIOS32_MIDI_DebugCommandCallback_Init(CONSOLE_Parse);
//! \endcode
//! \param[in] callback_debug_command the callback function (NULL disables the callback)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_DebugCommandCallback_Init(s32 (*callback_debug_command)(mios32_midi_port_t port, char c))
{
  debug_command_callback_func = callback_debug_command;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs the Timeout callback function which is executed on incomplete
//! MIDI packages received via UART, or on incomplete SysEx streams.
//!
//! A timeout is detected after 1 second.
//!
//! On a timeout, it is recommented to reset MIDI parsing relevant variables,
//! e.g. the state of a SysEx parser.
//!
//! Example:
//! \code
//! s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
//! {
//!   // if my SysEx parser receives a command (MY_SYSEX flag set), abort parser if port matches
//!   if( sysex_state.MY_SYSEX && port == last_sysex_port )
//!     MySYSEX_CmdFinished();
//!
//!   return 0; // no error
//! }
//! \endcode
//!
//! The callback function has been installed in an Init() function with:
//! \code
//!   MIOS32_MIDI_TimeOutCallback_Init(NOTIFY_MIDI_TimeOut)
//!   {
//!   }
//! \endcode
//! \param[in] callback_timeout the callback function (NULL disables the callback)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_TimeOutCallback_Init(s32 (*callback_timeout)(mios32_midi_port_t port))
{
  timeout_callback_func = callback_timeout;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called if a MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_MIDI_TimeOut(mios32_midi_port_t port)
{
  // if MIOS32 receives a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.MY_SYSEX && port == last_sysex_port )
    MIOS32_MIDI_SYSEX_CmdFinished();

  // optional hook to application
  if( timeout_callback_func != NULL )
    timeout_callback_func(port);

#if 1
  // this debug message should always be active, so that common users are informed about the exception
  MIOS32_MIDI_SendDebugMessage("[MIOS32_MIDI_Receive_Handler] Timeout on port 0x%02x\n", port);
#endif

  return 0; // no error
}

#endif /* MIOS32_DONT_USE_MIDI */
