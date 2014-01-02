// $Id$
/*
 * MIDIbox KB
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <keyboard.h>
#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "tasks.h"
#include "uip_task.h"
#include "osc_client.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// define priority level for sequencer
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;


/////////////////////////////////////////////////////////////////////////////
//! global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

// local prototype of the task function
static void TASK_Period_1mS(void *pvParameters);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // create semaphores
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(&NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(&NOTIFY_MIDI_TimeOut);

  // limit the number of DIN/DOUT SRs which will be scanned for faster scan rate
  MIOS32_SRIO_ScanNumSet(2);

  // init keyboard functions
  KEYBOARD_Init(0);

  // read EEPROM content
  PRESETS_Init(0);

  // init MIDI port/router handling
  MIDI_PORT_Init(0);
  MIDI_ROUTER_Init(0);

  // init terminal
  TERMINAL_Init(0);

  // init MIDImon
  MIDIMON_Init(0);

  // start uIP task
  UIP_TASK_Init(0);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("=================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("=================\n");
  MIOS32_MIDI_SendDebugMessage("\n");

  // speed up SPI transfer rate (was MIOS32_SPI_PRESCALER_128, initialized by MIOS32_SRIO_Init())
  MIOS32_SPI_TransferModeInit(MIOS32_SRIO_SPI, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);
  // prescaler 64 results into a transfer rate of 0.64 uS per bit
  // when 2 SRs are transfered, we are able to scan the whole 16x8 matrix in 300 uS

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);

  // start tasks
  xTaskCreate(TASK_Period_1mS, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

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
  // -> MIDI Router
  MIDI_ROUTER_Receive(port, midi_package);

  // -> MIDI Port Handler (used for MIDI monitor function)
  MIDI_PORT_NotifyMIDIRx(port, midi_package);

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, filter_sysex_message);
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // -> MIDI Router
  MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // -> keyboard handler
  KEYBOARD_SRIO_ServicePrepare();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
  // -> keyboard handler
  KEYBOARD_SRIO_ServiceFinish();

  // standard SRIO scan has been disabled in programming_models/traditional/main.c via MIOS32_DONT_SERVICE_SRIO_SCAN in mios32_config.h
  // start the scan here - and retrigger it whenever it's finished
  APP_SRIO_ServicePrepare();
  MIOS32_SRIO_ScanStart(APP_SRIO_ServiceFinish);
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
  // -> keyboard
  KEYBOARD_AIN_NotifyChange(pin, pin_value);
#if 0
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x10 + pin, pin_value >> 5);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to check for keyboard MIDI events
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // skip delay gap if we had to wait for more than 5 ticks to avoid
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

    // -> keyboard handler
    KEYBOARD_Periodic_1mS();

    // MIDI In/Out monitor
    // TODO: call from low-prio task
    MIDI_PORT_Period1mS();

    // update AINs with current value
    // the keyboard driver will only send events on value changes
    {
      int pin;

      for(pin=0; pin<6; ++pin) {
	KEYBOARD_AIN_NotifyChange(pin, MIOS32_AIN_PinGet(pin));
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // filter MIDI In port which controls the MIDI clock
  if( MIDI_ROUTER_MIDIClockInGet(port) == 1 ) {
   // SEQ_BPM_NotifyMIDIRx(midi_byte);
  }

  return 0; // no error, no filtering
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return MIDI_PORT_NotifyMIDITx(port, package);
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{
  // forward to SysEx parser
  // MBKB_SYSEX_TimeOut(port);

  // print message on screen
  //SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}
