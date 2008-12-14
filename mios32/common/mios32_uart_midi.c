// $Id$
/*
 * UART MIDI layer for MIOS32
 *
 * TODO: optional running status optimisation for MIDI Out
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
#if !defined(MIOS32_DONT_USE_UART_MIDI)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  mios32_midi_package_t package;
  u8 running_status;
  u8 expected_bytes;
  u8 wait_bytes;
  u8 sysex_ctr;
} midi_rec_t;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// handler data structure
static midi_rec_t midi_rec[MIOS32_UART_NUM];


/////////////////////////////////////////////////////////////////////////////
// Initializes UART MIDI layer
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_Init(u32 mode)
{
  int i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // initialize MIDI record
  for(i=0; i<MIOS32_UART_NUM; ++i) {
    midi_rec[i].package.ALL = 0;
    midi_rec[i].running_status = 0x00;
    midi_rec[i].expected_bytes = 0x00;
    midi_rec[i].wait_bytes = 0x00;
    midi_rec[i].sysex_ctr = 0x00;
  }

  // if any MIDI assignment:
#if MIOS32_UART0_ASSIGNMENT == 1 || MIOS32_UART1_ASSIGNMENT == 1
  // initialize U(S)ART interface
  if( MIOS32_UART_Init(0) < 0 )
    return -1; // initialisation of U(S)ART Interface failed
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function can be used to determine, if a UART interface is available
// IN: UART number (0..1)
// OUT: 1: interface available
//      0: interface not available
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
  }
  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a new MIDI package to the selected UART_MIDI port
// IN: UART_MIDI module number (0..1) in <uart_port>, MIDI package in <package>
// OUT: 0: no error
//      -1: UART_MIDI device not available
//      -2: UART_MIDI buffer is full
//          caller should retry until buffer is free again
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
// This function sends a new MIDI package to the selected UART_MIDI port
// (blocking function)
// IN: UART_MIDI module number (0..1) in <uart_port>, MIDI package in <package>
// OUT: 0: no error
//      -1: UART_MIDI device not available
//      -2: UART_MIDI buffer is full
//          caller should retry until buffer is free again
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_PackageSend(u8 uart_port, mios32_midi_package_t package)
{
  s32 error;

  while( (error=MIOS32_UART_MIDI_PackageSend_NonBlocking(uart_port, package)) == -2);

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// This function checks for a new package
// IN: UART_MIDI module number (0..1) in <uart_port>, 
//     pointer to MIDI package in <package> (received package will be put into the given variable)
// OUT: 0: no error
//      -1: no package in buffer
//      -2: UART interface allocated - retry (only in Non Blocking mode)
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
	  midix->wait_bytes = midix->expected_bytes - 1;
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

  // return 0 if new package in buffer, otherwise -1
  return package_complete ? 0 : -1;
#endif
}

#endif /* MIOS32_DONT_USE_UART_MIDI */
