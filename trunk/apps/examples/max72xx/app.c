// $Id$
/*
 * MAX72xx usage example
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (Thorsten.Klose@gmx.de)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <max72xx.h>

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize all MAX72xx
  MAX72XX_Init(0);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.type == CC ) {
    u8 chain = 0; // testing only the first chain

    int device = midi_package.cc_number - 16;
    if( device >= 0 && device < MAX72XX_NumDevicesPerChainGet(chain) ) {
      //s32 MAX72XX_AllDigitsSet(u8 chain, u8 device, u8 *values)
      MIOS32_MIDI_SendDebugMessage("Setting pattern for device #%d to %d\n", device, midi_package.value);
      MAX72XX_DigitSet(chain, device, 0, midi_package.value);
      MAX72XX_DigitSet(chain, device, 1, midi_package.value);
      MAX72XX_DigitSet(chain, device, 2, midi_package.value);
      MAX72XX_UpdateAllChains();
    } else {
      MIOS32_MIDI_SendDebugMessage("No MAX72xx device mapped to CC#%d\n", midi_package.cc_number);
    }
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
