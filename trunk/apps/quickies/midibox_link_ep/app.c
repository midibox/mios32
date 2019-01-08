// $Id$
/*
 * see README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

static u16 midibox_link_filter_uart;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // disable MIDIbox Link Filter for all ports
  midibox_link_filter_uart = 0;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // toggle Status LED on each incoming MIDI package
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // MIDIbox Link Endpoint Handling:
  // activate filter when 0xf9 has been received
  // deactive filter when 0xfe has been received
  // this is only done for UART ports
  if( (port & 0xf0) == UART0 ) {
    if( midi_package.evnt0 == 0xf9 ) {
      midibox_link_filter_uart |= (1 << (port & 0xf));
      return; // control word received: don't forward, exit!
    } else if( midi_package.evnt0 == 0xfe ) {
      midibox_link_filter_uart &= ~(1 << (port & 0xf));
      return; // control word received: don't forward, exit!
    }

    // exit as long as filter is active
    if( midibox_link_filter_uart & (1 << (port & 0xf)) )
      return; // filter active...
  }


  // forward any UART to USB0
  if( (port & 0xf0) == UART0 ) {
    MIOS32_MIDI_SendPackage(USB0,  midi_package);
  }

  // forward USB0 to UART0
  if( port == USB0 ) {
    MIOS32_MIDI_SendPackage(UART0, midi_package);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
