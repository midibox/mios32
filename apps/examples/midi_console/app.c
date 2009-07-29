// $Id$
/*
 * Demo application for a Debug Console via MIDI
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

#include "app.h"
#include "console.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
// should be enabled for this application!
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs

  // initialize console
  CONSOLE_Init(0);

#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
  DEBUG_MSG("Please send some characters to the console (e.g. via 'mios_terminal')!");
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD
  MIOS32_LCD_Clear();

  // print text
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("READY.");

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
