// $Id$
/*
 * MBHP_CORE_STM32 module test
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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
#include "check.h"

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

s32 check_delay;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD
  MIOS32_LCD_Clear();

  // print message
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Please          ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("disconnect LCD! ");

  // run endless
  while( 1 ) {
    // start IO test
    // results will be sent to MIOS Terminal
    s32 status = CHECK_Pins();

    // LED is lit when error has been found
    MIOS32_BOARD_LED_Init(1); // initialize all LEDs
    MIOS32_BOARD_LED_Set(1, (status < 0) ? 1 : 0);

    // init UART again
    MIOS32_UART_Init(0);

    // wait for 5 seconds
    // check_delay will be reset to 5000 whenever a SysEx byte is received
    for(check_delay=5000; check_delay>= 0; --check_delay)
      MIOS32_DELAY_Wait_uS(1000);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // reset wait delay - this ensures, that new code can be uploaded w/o the danger that
  // the background task disturbs it
  if( midi_package.evnt0 == 0xf0 )
    check_delay = 5000;
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
