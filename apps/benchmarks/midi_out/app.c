// $Id$
/*
 * Benchmark for MIDI Out ports (USB/UART/IIC) routines
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
#include "uip_task.h"

#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 benchmark_cycles;
static u8 tested_port;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // start uIP task
  UIP_TASK_Init(0);

  // initialize stopwatch for measuring delays
  MIOS32_STOPWATCH_Init(100);

  // initialize benchmark
  BENCHMARK_Init(0);

  // install MIDI Tx callback function
  MIOS32_MIDI_DirectTxCallback_Init(NOTIFY_MIDI_Tx);

  // init benchmark result
  benchmark_cycles = 0;
  tested_port = 0;

  // print first message
  print_msg = PRINT_MSG_INIT;

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
	MIOS32_LCD_PrintFormattedString("Port: 0x%02x     ", tested_port);
        MIOS32_LCD_CursorSet(0, 1);
	if( benchmark_cycles == 0xffffffff )
	  MIOS32_LCD_PrintFormattedString("Delay: overrun  ");
	else
	  MIOS32_LCD_PrintFormattedString("Delay: %4d.%d mS  ", benchmark_cycles/10, benchmark_cycles%10);

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

    // determine test number (use note number, remove octave)
    u8 test_number = midi_package.note % 12;

    // set the tested port and RS optimisation
    switch( test_number ) {
      case 0:
	tested_port = USB0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 0);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (USB0) with RS disabled\n", tested_port);
	break;

      case 1:
	tested_port = USB0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 1);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (USB0) with RS enabled\n", tested_port);
	break;

      case 2:
	tested_port = UART0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 0);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (UART0) with RS disabled\n", tested_port);
	break;

      case 3:
	tested_port = UART0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 1);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (UART0) with RS enabled\n", tested_port);
	break;

      case 4:
	tested_port = IIC0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 0);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (IIC0) with RS disabled\n", tested_port);
	break;

      case 5:
	tested_port = IIC0;
	MIOS32_MIDI_RS_OptimisationSet(tested_port, 1);
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (IIC0) with RS enabled\n", tested_port);
	break;

      case 6:
	tested_port = 0xf0; // forwarded to OSC, see NOTIFY_MIDI_Tx()
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (OSC), one datagram per event\n", tested_port);
	break;

      case 7:
	tested_port = 0xf1; // forwarded to OSC, see NOTIFY_MIDI_Tx()
	MIOS32_MIDI_SendDebugMessage("Testing Port 0x%02x (OSC), 8 events bundled in a datagram\n", tested_port);
	break;

      default:
	MIOS32_MIDI_SendDebugMessage("This note isn't mapped to a test function.\n", tested_port);
	return;
    }

    // add some delay to ensure that there a no USB background traffic caused by the debug message
    MIOS32_DELAY_Wait_uS(50000);

    // reset benchmark
    BENCHMARK_Reset();

    portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)

    // turn on LED (e.g. for measurements with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    // reset stopwatch
    MIOS32_STOPWATCH_Reset();

    // start benchmark
    BENCHMARK_Start(tested_port);

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


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // forward port 0xf0 messages to OSC client
  if( port == 0xf0 ) {
    mios32_osc_timetag_t timetag;
    timetag.seconds = 0;
    timetag.fraction = 0;
    OSC_CLIENT_SendMIDIEvent(timetag, package);
  } else if( port == 0xf1 && (package.note & 0x07) == 0 ) { // send 8 events bundled
    mios32_osc_timetag_t timetag;
    mios32_midi_package_t p[8];
    timetag.seconds = 0;
    timetag.fraction = 0;

    int i;
    for(i=0; i<8; ++i) {
      p[i] = package;
      ++package.note;
    }
    OSC_CLIENT_SendMIDIEventBundled(timetag, p, 8);
  }

  return 0; // no error, no filtering
}


