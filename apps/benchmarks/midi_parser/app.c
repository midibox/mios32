// $Id: app.c 1118 2010-10-24 13:54:51Z tk $
/*
 * Benchmark for MIDI parser
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

#include "benchmark.h"
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 benchmark_cycles;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


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

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Play MIDI notes to start different benchmarks\n");
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD screen
  MIOS32_LCD_Clear();

  // print message
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // wait endless
  while( 1 );
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  static s32 (*benchmark_reset)(u32 par);
  static s32 (*benchmark_start)(u32 par);
  u32 benchmark_par = 0;
  u32 num_loops = 100;

  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {

    // determine test number (use note number, remove octave)
    u8 test_number = midi_package.note % 12;

    // set the tested port and RS optimisation
    switch( test_number ) {
      case 0:
	MIOS32_MIDI_SendDebugMessage("Testing linear search in RAM\n");
	benchmark_reset = BENCHMARK_Reset_LinearRAM;
	benchmark_start = BENCHMARK_Start_LinearRAM;
	benchmark_par = 0;
	num_loops = 100;
	break;

      case 1:
	MIOS32_MIDI_SendDebugMessage("Testing linear search in AHB RAM\n");
	benchmark_reset = BENCHMARK_Reset_LinearRAM;
	benchmark_start = BENCHMARK_Start_LinearRAM;
	benchmark_par = 1;
	num_loops = 100;
	break;

      case 2:
	MIOS32_MIDI_SendDebugMessage("Testing linear search with known length in RAM\n");
	benchmark_reset = BENCHMARK_Reset_LinearRAM_KnownLen;
	benchmark_start = BENCHMARK_Start_LinearRAM_KnownLen;
	benchmark_par = 0;
	num_loops = 100;
	break;

      case 3:
	MIOS32_MIDI_SendDebugMessage("Testing linear search with known length in AHB RAM\n");
	benchmark_reset = BENCHMARK_Reset_LinearRAM_KnownLen;
	benchmark_start = BENCHMARK_Start_LinearRAM_KnownLen;
	benchmark_par = 1;
	num_loops = 100;
	break;

      default:
	MIOS32_MIDI_SendDebugMessage("This note isn't mapped to a test function.\n");
	return;
    }

    // add some delay to ensure that there a no USB background traffic caused by the debug message
    MIOS32_DELAY_Wait_uS(50000);

    // reset benchmark
    benchmark_reset(benchmark_par);

    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)

    // turn on LED (e.g. for measurements with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    {
      int i;

      for(i=0; i<num_loops; ++i)
	benchmark_start(benchmark_par);
    }

    // capture counter value
    benchmark_cycles = MIOS32_STOPWATCH_ValueGet();

    // turn off LED
    MIOS32_BOARD_LED_Set(0xffffffff, 0);

    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable tasks (nested)

    // print result on MIOS terminal
    if( benchmark_cycles == 0xffffffff )
      MIOS32_MIDI_SendDebugMessage("Time: overrun!\n");
    else
      MIOS32_MIDI_SendDebugMessage("Time: %5d.%d mS\n", benchmark_cycles/(10*num_loops), benchmark_cycles%(10*num_loops));
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
