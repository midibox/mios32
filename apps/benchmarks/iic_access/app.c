// $Id$
/*
 * Benchmark for MIOS32_IIC routines
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

volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 benchmark_cycles_reads;
static u32 benchmark_cycles_block_reads;
static u32 benchmark_cycles_writes;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize stopwatch for measuring delays
  MIOS32_STOPWATCH_Init(100);

  // initialize benchmark
  BENCHMARK_Init(0);

  // init benchmark result
  benchmark_cycles_reads = 0;
  benchmark_cycles_block_reads = 0;
  benchmark_cycles_writes = 0;

  // print first message
  print_msg = PRINT_MSG_INIT;

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Settings:\n");
  MIOS32_MIDI_SendDebugMessage("#define MIOS32_IIC_BUS_FREQUENCY %u\n", MIOS32_IIC_BUS_FREQUENCY);
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
	if( benchmark_cycles_reads == 0xffffffff )
	  MIOS32_LCD_PrintFormattedString("Reads: overrun  ");
	else
	  MIOS32_LCD_PrintFormattedString("Reads: %4d.%d mS  ", benchmark_cycles_reads/10, benchmark_cycles_reads%10);

        MIOS32_LCD_CursorSet(0, 1);
	if( benchmark_cycles_writes == 0xffffffff )
	  MIOS32_LCD_PrintFormattedString("Write: overrun ");
	else
	  MIOS32_LCD_PrintFormattedString("Write: %4d.%d mS  ", benchmark_cycles_writes/10, benchmark_cycles_writes%10);

	// request status screen again (will stop once a new screen is requested by another task)
	print_msg = PRINT_MSG_STATUS;
      }
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {

    // reset benchmark (returns < 0 on errors)
    s32 status;
    if( (status=BENCHMARK_Reset()) < 0 ) {
      benchmark_cycles_reads = 0xffffffff;
      benchmark_cycles_block_reads = 0xffffffff;
      benchmark_cycles_writes = 0xffffffff;

      MIOS32_MIDI_SendDebugMessage("No IIC EEPROM connected - error code %d!\n", status);
      return;
    }

    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)

    // turn on LED (e.g. for measurements with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, 1);


    /////////////////////////////////////////////////////////////////////////
    // IIC Reads
    /////////////////////////////////////////////////////////////////////////

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    s32 status_reads = BENCHMARK_Start_Reads();

    // capture counter value
    benchmark_cycles_reads = MIOS32_STOPWATCH_ValueGet();


    /////////////////////////////////////////////////////////////////////////
    // IIC Block Reads
    /////////////////////////////////////////////////////////////////////////

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    s32 status_block_reads = BENCHMARK_Start_BlockReads();

    // capture counter value
    benchmark_cycles_block_reads = MIOS32_STOPWATCH_ValueGet();


    /////////////////////////////////////////////////////////////////////////
    // IIC Writes
    /////////////////////////////////////////////////////////////////////////

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    s32 status_writes = BENCHMARK_Start_Writes();

    // capture counter value
    benchmark_cycles_writes = MIOS32_STOPWATCH_ValueGet();


    /////////////////////////////////////////////////////////////////////////
    // Finished...
    /////////////////////////////////////////////////////////////////////////

    // turn off LED
    MIOS32_BOARD_LED_Set(0xffffffff, 0);

    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable tasks (nested)

    // print result on MIOS terminal
    if( status_reads < 0 ) {
      MIOS32_MIDI_SendDebugMessage("Reads: error status %d!\n", status_reads);
      benchmark_cycles_reads = 0xffffffff;
    } else {
      if( benchmark_cycles_reads == 0xffffffff )
	MIOS32_MIDI_SendDebugMessage("Reads: overrun!\n");
      else {
	MIOS32_MIDI_SendDebugMessage("Reads: %5d.%d mS / 1000\n", benchmark_cycles_reads/10, benchmark_cycles_reads%10);
	MIOS32_MIDI_SendDebugMessage("-> %d read accesses per second\n", 10000000 / benchmark_cycles_reads);
	MIOS32_MIDI_SendDebugMessage("-> transfered %d bytes per second\n", 4 * (10000000 / benchmark_cycles_reads));
      }
    }

    if( status_block_reads < 0 ) {
      MIOS32_MIDI_SendDebugMessage("Reads: error status %d!\n", status_reads);
      benchmark_cycles_block_reads = 0xffffffff;
    } else {
      if( benchmark_cycles_block_reads == 0xffffffff )
	MIOS32_MIDI_SendDebugMessage("Block Reads: overrun!\n");
      else {
	MIOS32_MIDI_SendDebugMessage("Block Reads: %5d.%d mS / 1000\n", benchmark_cycles_block_reads/10, benchmark_cycles_block_reads%10);
	MIOS32_MIDI_SendDebugMessage("-> %d block read accesses per second\n", 10000000 / benchmark_cycles_block_reads);
	MIOS32_MIDI_SendDebugMessage("-> transfered %d bytes per second\n", 67 * (10000000 / benchmark_cycles_block_reads));
      }
    }

    if( status_reads < 0 ) {
      MIOS32_MIDI_SendDebugMessage("Writes: error status %d!\n", status_writes);
      benchmark_cycles_writes = 0xffffffff;
    } else {
      if( benchmark_cycles_writes == 0xffffffff )
	MIOS32_MIDI_SendDebugMessage("Writes: overrun!\n");
      else {
	MIOS32_MIDI_SendDebugMessage("Writes:%5d.%d mS / 1000\n", benchmark_cycles_writes/10, benchmark_cycles_writes%10);
	MIOS32_MIDI_SendDebugMessage("-> %d write accesses per second\n", 10000000 / benchmark_cycles_writes);
	MIOS32_MIDI_SendDebugMessage("-> transfered %d bytes per second\n", 3 * (10000000 / benchmark_cycles_writes));
      }
    }

    // print status screen
    print_msg = PRINT_MSG_STATUS;
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
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
