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
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// optional non-blocking mode
static u8 non_blocking_mode = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes UART MIDI layer
// IN: <mode>: 0: MIOS32_UART_MIDI_Send* works in blocking mode - function will
//                (shortly) stall if the output buffer is full
//             1: MIOS32_UART_MIDI_Send* works in non-blocking mode - function will
//                return -2 if buffer is full, the caller has to loop if this
//                value is returned until the transfer was successful
//                A common method is to release the RTOS task for 1 mS
//                so that other tasks can be executed until the sender can
//                continue
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_Init(u32 mode)
{
  switch( mode ) {
    case 0:
      non_blocking_mode = 0;
      break;
    case 1:
      non_blocking_mode = 1;
      break;
    default:
      return -1; // unsupported mode
  }

  // initialize U(S)ART interface
  if( MIOS32_UART_Init(0) < 0 )
    return -1; // initialisation of U(S)ART Interface failed

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
  return uart_port >= MIOS32_UART_NUM ? 0 : 1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a new MIDI package to the selected UART_MIDI port
// IN: UART_MIDI module number (0..1) in <uart_port>, MIDI package in <package>
// OUT: 0: no error
//      -1: UART_MIDI device not available
//      -2: if non-blocking mode activated: UART_MIDI buffer is full
//          caller should retry until buffer is free again
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_MIDI_PackageSend(u8 uart_port, mios32_midi_package_t package)
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

    if( non_blocking_mode ) {
      switch( MIOS32_UART_TxBufferPutMore(uart_port, buffer, len) ) {
        case  0: return  0; // transfer successfull
        case -2: return -2; // buffer full, request retry
      }
    } else {
      s32 error;
      while( (error=MIOS32_UART_TxBufferPutMore(uart_port, buffer, len)) == -2 ); // retry until error status != -2

      if( !error )
	return 0; // no error
    }
    return -1; // UART error
  } else {
    return 0; // no bytes to send -> no error
  }
#endif
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

  // TODO
  return -1;
#endif
}

#endif /* MIOS32_DONT_USE_UART_MIDI */
