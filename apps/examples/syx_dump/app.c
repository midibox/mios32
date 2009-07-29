// $Id$
/*
 * Syx Dump Demonstration
 * See README.txt for details
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

#include "patch.h"
#include "sysex.h"

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 patch;
volatile u8 bank;
volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialise SysEx parser
  SYSEX_Init(0);

  // initialize patch structure
  PATCH_Init(0);

  // init local patch/bank
  patch = bank = 0;

  // print first message
  print_msg = PRINT_MSG_INIT;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD screen
  MIOS32_LCD_Clear();

  // endless loop: print status information on LCD
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // new message requested?
    // TODO: add FreeRTOS specific queue handling!
    u8 new_msg = PRINT_MSG_NONE;
    portENTER_CRITICAL(); // port specific FreeRTOS function to disable IRQs (nested)
    if( print_msg ) {
      new_msg = print_msg;
      print_msg = PRINT_MSG_NONE; // clear request
    }
    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable IRQs (nested)

    switch( new_msg ) {
      case PRINT_MSG_INIT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintString("see README.txt   ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("for details     ");
	break;

      case PRINT_MSG_PATCH_AND_BANK:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintFormattedString("Patch: %3d B:%d%c ", patch, bank, MIOS32_IIC_BS_CheckAvailable(bank) ? ' ' : '*');
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("                ");
	break;

      case PRINT_MSG_DUMP_SENT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintFormattedString("Patch: %3d B:%d%c ", patch, bank, MIOS32_IIC_BS_CheckAvailable(bank) ? ' ' : '*');
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("Dump sent!      ");
	break;

      case PRINT_MSG_DUMP_RECEIVED:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintFormattedString("Patch: %3d B:%d%c ", patch, bank, MIOS32_IIC_BS_CheckAvailable(bank) ? ' ' : '*');
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("Dump received!  ");
	break;
    }
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
  // ignore if button has been depressed
  if( pin_value )
    return;

  // call function depending on button number
  switch( pin ) {
    case DIN_NUMBER_EXEC: // Exec button
      // load patch (again)
      PATCH_Load(bank, patch);

      // send dump
      SYSEX_Send(DEFAULT, bank, patch);

      // print patch as temporary message + print "Dump sent"
      print_msg = PRINT_MSG_DUMP_SENT;
      break;

    case DIN_NUMBER_INC: // Inc button
      // increment patch, wrap on overflow
      if( ++patch >= 0x80 )
	patch = 0x00;

      // load patch
      PATCH_Load(bank, patch);

      // print patch as temporary message
      print_msg = PRINT_MSG_PATCH_AND_BANK;
      break;

    case DIN_NUMBER_DEC: // Dec button
      // decrement patch, wrap on underflow
      if( --patch >= 0x80 ) // patch is an unsigned number...
	patch = 0x7f;

      // load patch
      PATCH_Load(bank, patch);

      // print patch as temporary message
      print_msg = PRINT_MSG_PATCH_AND_BANK;
      break;

    case DIN_NUMBER_SNAPSHOT: // Snapshot button
      // increment bank, wrap at 8
      if( ++bank >= 8 )
	bank = 0;

      // load patch
      PATCH_Load(bank, patch);

      // print patch as temporary message
      print_msg = PRINT_MSG_PATCH_AND_BANK;
      break;
  }
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
