// $Id$
/*
 * MIDI layer functions for MIOS32
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
// IN: <port>: MIDI port 
//             0..15: USB, 16..31: USART, 32..47: IIC, 48..63: Ethernet
//     <package>: MIDI package (see definition in mios32_midi.h)
// OUT: returns -1 if port not available
//      returns -2 if port available, but buffer full (retry it)
//      returns 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_PackageSend(u8 port, midi_package_t package)
{
  // insert subport number into package
  package.ptype = (package.ptype&0x0f) | (port << 4);

  // branch depending on selected port
  switch( port >> 4 ) {
    case 0:
#if !defined(MIOS32_DONT_USE_USB)
      return (MIOS32_USB_MIDIPackageSend(package.ALL) < 0) ? -2 : 0;
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
// Initializes MIDI layer
// IN: <mode>: currently only mode 0 supported
//             later we could provide operation modes
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_PackageReceive_Handler(void *callback)
{
}


#endif /* MIOS32_DONT_USE_MIDI */
