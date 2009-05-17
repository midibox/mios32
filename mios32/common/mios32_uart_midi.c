// $Id$
//! \defgroup MIOS32_UART_MIDI
//!
//! UART MIDI layer for MIOS32
//! 
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_MIDI layer functions
//! 
//! \todo optional running status optimisation for MIDI Out
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
#if !defined(MIOS32_DONT_USE_UART_MIDI)

/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_package_t package;
  u8 running_status;
  u8 expected_bytes;
  u8 wait_bytes;
  u8 sysex_ctr;
  u16 timeout_ctr;
} midi_rec_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// handler data structure
#if MIOS32_UART_NUM
static midi_rec_t midi_rec[MIOS32_UART_NUM];

// seperated from midi_rec, since midi_rec variables are
// only used for parser and could be reseted on a timeout
static u8 rs_optimisation;
static u8 rs_last[MIOS32_UART_NUM];
static u16 rs_expire_ctr[MIOS32_UART_NUM];
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


// internal function to reset the record structure
static s32 MIOS32_UART_MIDI_RecordReset(u8 uart_port)
{
  midi_rec_t *midix = &midi_rec[uart_port];// simplify addressing of midi record

  midix->package.ALL = 0;
  midix->running_status = 0x00;
  midix->expected_bytes = 0x00;
  midix->wait_bytes = 0x00;
  midix->sysex_ctr = 0x00;
  midix->timeout_ctr = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes UART MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_Init(u32 mode)
{
#if MIOS32_UART_NUM == 0
  return -1; // no UART enabled
#else
  int i;


  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // initialize MIDI record
  for(i=0; i<MIOS32_UART_NUM; ++i)
    MIOS32_UART_MIDI_RecordReset(i);

  // if any MIDI assignment:
#if MIOS32_UART0_ASSIGNMENT == 1 || MIOS32_UART1_ASSIGNMENT == 1
  // initialize U(S)ART interface
  if( MIOS32_UART_Init(0) < 0 )
    return -1; // initialisation of U(S)ART Interface failed
#endif

  // enable running status optimisation by default for all ports
  // clear timeout counters
  rs_optimisation = ~0; // -> all-one
  for(i=0; i<MIOS32_UART_NUM; ++i)
    MIOS32_UART_MIDI_RS_Reset(i);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function can be used to determine, if a UART interface is available
//! \param[in] uart_port UART number (0..1)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_CheckAvailable(u8 uart_port)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled
#else
  switch( uart_port ) {
    case 0: return MIOS32_UART0_ASSIGNMENT == 1 ? 1 : 0; // UART0 assigned to MIDI?
#if MIOS32_UART_NUM >= 2
    case 1: return MIOS32_UART1_ASSIGNMENT == 1 ? 1 : 0; // UART1 assigned to MIDI?
#endif
#if MIOS32_UART_NUM >= 3
    case 2: return MIOS32_UART2_ASSIGNMENT == 1 ? 1 : 0; // UART2 assigned to MIDI?
#endif
#if MIOS32_UART_NUM >= 4
    case 3: return MIOS32_UART3_ASSIGNMENT == 1 ? 1 : 0; // UART3 assigned to MIDI?
#endif
  }
  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables/disables running status optimisation for a given
//! MIDI OUT port to improve bandwidth if MIDI events with the same
//! status byte are sent back-to-back.<BR>
//! Note that the optimisation is enabled by default.
//! \param[in] uart_port UART number (0..1)
//! \param[in] enable 0=optimisation disabled, 1=optimisation enabled
//! \return -1 if port not available
//! \return 0 on success
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_RS_OptimisationSet(u8 uart_port, u8 enable)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled
#else
  if( uart_port >= MIOS32_UART_NUM )
    return -1; // port not available

  u8 mask = 1 << uart_port;
  rs_optimisation &= ~mask;
  if( enable )
    rs_optimisation |= mask;

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the running status optimisation enable/disable flag
//! for the given MIDI OUT port.
//! \param[in] uart_port UART number (0..1)
//! \return -1 if port not available
//! \return 0 if optimisation disabled
//! \return 1 if optimisation enabled
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_RS_OptimisationGet(u8 uart_port)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled
#else
  if( uart_port >= MIOS32_UART_NUM )
    return -1; // port not available

  return (rs_optimisation & (1 << uart_port)) ? 1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function resets the current running status, so that it will be sent
//! again with the next MIDI Out package.
//! \param[in] uart_port UART number (0..1)
//! \return -1 if port not available
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_RS_Reset(u8 uart_port)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled
#else
  if( uart_port >= MIOS32_UART_NUM )
    return -1; // port not available

  MIOS32_IRQ_Disable();
  rs_last[uart_port] = 0xff;
  rs_expire_ctr[uart_port] = 0;
  MIOS32_IRQ_Enable();

  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to handle timeout
//! and expire counters.
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_Periodic_mS(void)
{
#if MIOS32_UART_NUM
  u8 uart_port;

  MIOS32_IRQ_Disable();
  for(uart_port=0; uart_port<MIOS32_UART_NUM; ++uart_port) {
    // increment the expire counters for running status optimisation.
    //
    // The running status will expire after 1000 ticks (1 second) 
    // to ensure, that the current status will be sent at least each second
    // to cover the case that the MIDI cable is (re-)connected during runtime.
    if( rs_expire_ctr[uart_port] < 65535 )
      ++rs_expire_ctr[uart_port];

    // increment timeout counter for incoming packages
    // an incomplete event will be timed out after 1000 ticks (1 second)
    if( midi_rec[uart_port].timeout_ctr < 65535 )
      ++midi_rec[uart_port].timeout_ctr;
  }
  MIOS32_IRQ_Enable();
  // (atomic operation not required in MIOS32_UART_MIDI_PackageSend_NonBlocking() due to single-byte accesses)
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected UART_MIDI port
//! \param[in] uart_port UART_MIDI module number (0..1)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: UART_MIDI device not available
//! \return -2: UART_MIDI buffer is full
//!             caller should retry until buffer is free again
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_PackageSend_NonBlocking(u8 uart_port, mios32_midi_package_t package)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled
#else
  // exit if UART port not available
  if( !MIOS32_UART_MIDI_CheckAvailable(uart_port) )
    return -1;

  u8 len = mios32_midi_pcktype_num_bytes[package.cin];
  if( len ) {
    u8 buffer[3] = {package.evnt0, package.evnt1, package.evnt2};

    if( rs_expire_ctr[uart_port] > 1000 ) {
      // the current RS is expired each second to ensure that a status byte will be sent
      // if the MIDI cable is (re)connected during runtime
      MIOS32_UART_MIDI_RS_Reset(uart_port);
#if 0
      // for optional monitoring of the optimisation
      MIOS32_MIDI_SendDebugMessage("[MIOS32_UART_MIDI:%d] RS 0x%02x expired!\n", uart_port);
#endif
    } else {
      if( (rs_optimisation & (1 << uart_port)) &&
	  package.cin >= NoteOff && package.cin <= PitchBend &&
	  len > 1 ) { // (len check is a failsafe measure)
	if( package.evnt0 == rs_last[uart_port] ) {
	  buffer[0] = package.evnt1;
	  buffer[1] = package.evnt2;
	  --len;
#if 0
	  // for optional monitoring of the optimisation
	  MIOS32_MIDI_SendDebugMessage("[MIOS32_UART_MIDI:%d] RS optimized (%02x) %02x %02x\n", uart_port, package.evnt0, package.evnt1, package.evnt2);
#endif
	} else {
	  // new running status
	  rs_expire_ctr[uart_port] = 0;
	}
      }
    }

    // note: packages != Note Off, On, ... Pitch Bend will disable running status - thats acceptable
    // only realtime events won't touch it (according to MIDI spec)
    if( package.evnt0 < 0xf8 )
      rs_last[uart_port] = package.evnt0;


    switch( MIOS32_UART_TxBufferPutMore(uart_port, buffer, len) ) {
      case  0: return  0; // transfer successfull
      case -2: return -2; // buffer full, request retry
      default: return -1; // UART error
    }

  } else {
    return 0; // no bytes to send -> no error
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected UART_MIDI port
//! (blocking function)
//! \param[in] uart_port UART_MIDI module number (0..1)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: UART_MIDI device not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_PackageSend(u8 uart_port, mios32_midi_package_t package)
{
  s32 error;

  while( (error=MIOS32_UART_MIDI_PackageSend_NonBlocking(uart_port, package)) == -2);

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[in] uart_port UART_MIDI module number (0..1)
//! \param[out] package pointer to MIDI package (received package will be put into the given variable
//! \return 0: no error
//! \return -1: no package in buffer
//! \return -10: incoming MIDI package timed out (incomplete package received)
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_PackageReceive(u8 uart_port, mios32_midi_package_t *package)
{
#if MIOS32_UART_NUM == 0
  return -1; // all UARTs explicitely disabled - accordingly no package in buffer
#else
  // exit if UART port not available
  if( !MIOS32_UART_MIDI_CheckAvailable(uart_port) )
    return -1;

  // parses the next incoming byte(s), stop until we got a complete MIDI event
  // (-> complete package) and forward it to the caller
  midi_rec_t *midix = &midi_rec[uart_port];// simplify addressing of midi record
  u8 package_complete = 0;
  s32 status;
  while( !package_complete && (status=MIOS32_UART_RxBufferGet(uart_port)) >= 0 ) {
    u8 byte = (u8)status;

    if( byte & 0x80 ) { // new MIDI status
      if( byte >= 0xf8 ) { // events >= 0xf8 don't change the running status and can just be forwarded
	// Realtime messages don't change the running status and can be sent immediately
	// They also don't touch the timeout counter!
	package->cin = 0xf; // F: single byte
	package->evnt0 = byte;
	package->evnt1 = 0x00;
	package->evnt2 = 0x00;
	package_complete = 1;
      } else {
	midix->running_status = byte;
	midix->expected_bytes = mios32_midi_expected_bytes_common[(byte >> 4) & 0x7];

	if( !midix->expected_bytes ) { // System Message, take number of bytes from expected_bytes_system[] array
	  midix->expected_bytes = mios32_midi_expected_bytes_system[byte & 0xf];

	  if( byte == 0xf0 ) {
	    midix->package.evnt0 = 0xf0; // midix->package.evnt0 only used by SysEx handler for continuous data streams!
	    midix->sysex_ctr = 0x01;
	  } else if( byte == 0xf7 ) {
	    switch( midix->sysex_ctr ) {
 	      case 0:
		midix->package.cin = 5; // 5: SysEx ends with single byte
		midix->package.evnt0 = 0xf7;
		midix->package.evnt1 = 0x00;
		midix->package.evnt2 = 0x00;
		break;
	      case 1:
		midix->package.cin = 6; // 6: SysEx ends with two bytes
		// midix->package.evnt0 = // already stored
		midix->package.evnt1 = 0xf7;
		midix->package.evnt2 = 0x00;
		break;
	      default:
		midix->package.cin = 7; // 7: SysEx ends with three bytes
		// midix->package.evnt0 = // already stored
		// midix->package.evnt1 = // already stored
		midix->package.evnt2 = 0xf7;
		break;
	    }
	    *package = midix->package;
	    package_complete = 1; // -> forward to caller
	    midix->sysex_ctr = 0x00; // ensure that next F7 will just send F7
	  }
	}

	midix->wait_bytes = midix->expected_bytes;
	midix->timeout_ctr = 0; // reset timeout counter
      }
    } else {
      if( midix->running_status == 0xf0 ) {
	switch( ++midix->sysex_ctr ) {
  	  case 1:
	    midix->package.evnt0 = byte; 
	    break;
	  case 2: 
	    midix->package.evnt1 = byte; 
	    break;
	  default: // 3
	    midix->package.evnt2 = byte;

	    // Send three-byte event
	    midix->package.cin = 4;  // 4: SysEx starts or continues
	    *package = midix->package;
	    package_complete = 1; // -> forward to caller
	    midix->sysex_ctr = 0x00; // reset and prepare for next packet
	}
      } else { // Common MIDI message or 0xf1 >= status >= 0xf7
	if( !midix->wait_bytes ) {
	  // received new MIDI event with running status
	  midix->wait_bytes = midix->expected_bytes - 1;
	  midix->timeout_ctr = 0; // reset timeout counter
	} else {
	  --midix->wait_bytes;
	}

	if( midix->expected_bytes == 1 ) {
	  midix->package.evnt1 = byte;
	  midix->package.evnt2 = 0x00;
	} else {
	  if( midix->wait_bytes )
	    midix->package.evnt1 = byte;
	  else
	    midix->package.evnt2 = byte;
	}
	
	if( !midix->wait_bytes ) {
	  if( (midix->running_status & 0xf0) != 0xf0 ) {
	    midix->package.cin = midix->running_status >> 4; // common MIDI message
	  } else {
	    switch( midix->expected_bytes ) { // MEMO: == 0 comparison was a bug in original MBHP_USB code
  	      case 0: 
		midix->package.cin = 5; // 5: SysEx common with one byte
		break;
  	      case 1: 
		midix->package.cin = 2; // 2: SysEx common with two bytes
		break;
  	      default: 
		midix->package.cin = 3; // 3: SysEx common with three bytes
		break;
	    }
	  }

	  midix->package.evnt0 = midix->running_status;
	  // midix->package.evnt1 = // already stored
	  // midix->package.evnt2 = // already stored
	  *package = midix->package;
	  package_complete = 1; // -> forward to caller
	}
      }
    }
  }

  // incoming MIDI package timed out (incomplete package received)
  if( midix->wait_bytes && midix->timeout_ctr > 1000 ) { // 1000 mS = 1 second
    // stop waiting
    MIOS32_UART_MIDI_RecordReset(uart_port);
    // notify that incomplete package has been received
    return -10;
  }

  // return 0 if new package in buffer, otherwise -1
  return package_complete ? 0 : -1;
#endif
}

//!}

#endif /* MIOS32_DONT_USE_UART_MIDI */
