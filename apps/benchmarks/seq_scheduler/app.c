// $Id$
/*
 * Benchmark for MIDI Out Scheduler
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

#include <FreeRTOS.h>
#include <portmacro.h>

#include <seq_midi_out.h>
#include "benchmark.h"
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 benchmark_cycles;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize stopwatch for measuring delays
  MIOS32_STOPWATCH_Init(100);

  // initialize benchmark
  BENCHMARK_Init(0);

  // init benchmark result
  benchmark_cycles = 0;

  // print first message
  print_msg = PRINT_MSG_INIT;

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Settings:\n");
  MIOS32_MIDI_SendDebugMessage("#define SEQ_MIDI_OUT_MALLOC_METHOD %d\n", SEQ_MIDI_OUT_MALLOC_METHOD);
  MIOS32_MIDI_SendDebugMessage("#define SEQ_MIDI_OUT_MAX_EVENTS %d\n", SEQ_MIDI_OUT_MAX_EVENTS);
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Play any MIDI note to start the benchmark\n");
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
    // new message requested?
    // TODO: add FreeRTOS specific queue handling!
    u8 new_msg = PRINT_MSG_NONE;
    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)
    if( print_msg ) {
      new_msg = print_msg;
      print_msg = PRINT_MSG_NONE; // clear request
    }
    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable tasks (nested)

    switch( new_msg ) {
      case PRINT_MSG_INIT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintString("see README.txt   ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("for details     ");
	break;

      case PRINT_MSG_STATUS:
      {
        MIOS32_LCD_CursorSet(0, 0);
	MIOS32_LCD_PrintFormattedString("A%3d  M%3d  D%3d",
					seq_midi_out_allocated,
					seq_midi_out_max_allocated,
					seq_midi_out_dropouts);

        MIOS32_LCD_CursorSet(0, 1);
	if( benchmark_cycles == 0xffffffff )
	  MIOS32_LCD_PrintFormattedString("Time: overrun   ");
	else
	  MIOS32_LCD_PrintFormattedString("Time: %5d.%d mS  ", benchmark_cycles/10, benchmark_cycles%10);

	// request status screen again (will stop once a new screen is requested by another task)
	print_msg = PRINT_MSG_STATUS;
      }
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
    // reset benchmark
    BENCHMARK_Reset();

    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)

    // turn on LED (e.g. for measurements with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    BENCHMARK_Start();

    // capture counter value
    benchmark_cycles = MIOS32_STOPWATCH_ValueGet();

    // turn off LED
    MIOS32_BOARD_LED_Set(0xffffffff, 0);

    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable tasks (nested)

    // print result on MIOS terminal
    if( benchmark_cycles == 0xffffffff )
      MIOS32_MIDI_SendDebugMessage("Time: overrun!\n");
    else
      MIOS32_MIDI_SendDebugMessage("Time: %5d.%d mS\n", benchmark_cycles/10, benchmark_cycles%10);

    // print status screen
    print_msg = PRINT_MSG_STATUS;
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
