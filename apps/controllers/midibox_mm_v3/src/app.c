// $Id$
/*
 * MIDIbox MM V3
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

#include <eeprom.h>

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <midi_port.h>
#include <midi_router.h>
#include <midimon.h>

#include "app.h"
#include "presets.h"
#include "terminal.h"
#include "tasks.h"
#include "uip_task.h"
#include "osc_client.h"

#include "mm_hwcfg.h"
#include "mm_lcd.h"
#include "mm_sysex.h"
#include "mm_midi.h"
#include "mm_mf.h"
#include "mm_vpot.h"
#include "mm_dio.h"
#include "mm_gpc.h"
#include "mm_leddigits.h"


/////////////////////////////////////////////////////////////////////////////
// RTOS tasks
/////////////////////////////////////////////////////////////////////////////

// define priority level for periodic handler
// use same priority as MIOS32 specific tasks
#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )

// low-prio thread for LCD output
#define PRIORITY_TASK_PERIOD_1mS_LP ( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// status of LC emulation
mm_flags_t mm_flags;

// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;

// ms accurate counter
static u32 counter_ms;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

// local prototype of the task functions
static void TASK_Period_1mS(void *pvParameters);
static void TASK_Period_1mS_LP(void *pvParameters);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);


/////////////////////////////////////////////////////////////////////////////
// Default MIDI Router configuration
/////////////////////////////////////////////////////////////////////////////

midi_router_node_entry_t midi_router_cfg[MIDI_ROUTER_NUM_NODES] = {
  // src  chn   dst  chn
  { USB0,  17, UART0, 17 }, // 17 == all
  { UART0, 17, USB0,  17 },

  { USB1,  17, UART1, 17 },
  { UART1, 17, USB1,  17 },

  { USB2,  17, UART2, 17 },
  { UART2, 17, USB2,  17 },

  { USB3,  17, UART3, 17 },
  { UART3, 17, USB3,  17 },

  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
  { USB0,   0, UART0, 17 },
};


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // clear mS counter
  counter_ms = 0;

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

  // initialize application specific variables
  MM_HWCFG_Init(0);
  MM_SYSEX_Init(0);
  MM_VPOT_Init(0);
  MM_MF_Init(0);
  MM_GPC_Init(0);
  MM_LEDDIGITS_Init(0);
  MM_DIO_SFBLEDUpdate();

  // initialize the shift registers
  //  MIOS32_SRIO_TS_SensitivitySet(TOUCHSENSOR_SENSITIVITY); // TODO
  MIOS32_SRIO_DebounceSet(20);

  // init MIDI port/router handling
  MIDI_PORT_Init(0);
  MIDI_ROUTER_Init(0);

  // create default router configuration
  int node;
  midi_router_node_entry_t *ncfg = (midi_router_node_entry_t *)&midi_router_cfg[0];
  midi_router_node_entry_t *n = (midi_router_node_entry_t *)&midi_router_node[0];
  for(node=0; node<MIDI_ROUTER_NUM_NODES; ++node, ++n, ++ncfg) {
	n->src_port = ncfg->src_port;
	n->src_chn = ncfg->src_chn;
	n->dst_port = ncfg->dst_port;
	n->dst_chn = ncfg->dst_chn;
  }

  // init terminal
  TERMINAL_Init(0);

  // init MIDImon
  MIDIMON_Init(0);

  // read EEPROM content
  PRESETS_Init(0);

  // start uIP task
  UIP_TASK_Init(0);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("=================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("=================\n");
  MIOS32_MIDI_SendDebugMessage("\n");

  // start tasks
  xTaskCreate(TASK_Period_1mS, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
  xTaskCreate(TASK_Period_1mS_LP, (signed portCHAR *)"1mS_LP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
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
  // -> MIDI Router
  MIDI_ROUTER_Receive(port, midi_package);

  // -> MIDI Port Handler (used for MIDI monitor function)
  MIDI_PORT_NotifyMIDIRx(port, midi_package);

  // forward to MIDI Monitor
  // SysEx messages have to be filtered for USB0 and UART0 to avoid data corruption
  // (the SysEx stream would interfere with monitor messages)
  u8 filter_sysex_message = (port == USB0) || (port == UART0);
  MIDIMON_Receive(port, midi_package, counter_ms, filter_sysex_message);

  if( port == MIOS32_MIDI_DefaultPortGet() ) {
    // forward MIDI event to MIDI handler
    MM_MIDI_Received(midi_package);

    // forward MIDI event to GPC handler
    MM_GPC_Received(midi_package);

    // CC events are also forwarded to MBHP_MF_NG module
    if( midi_package.type == CC ) {
      MIOS32_MIDI_SendPackage(UART2, midi_package); // STM32 and LPC17
      MIOS32_MIDI_SendPackage(UART3, midi_package); // LPC17
    }
  }


  // extra for MBHP_MF_NG: CC Events are also forwarded to default port
  if( port == UART2 || port == UART3 ) {
    if( midi_package.type == CC ) {
      MIOS32_MIDI_SendPackage(USB0, midi_package);

      if( mm_hwcfg_verbose_level >= 2 ) {
	MIOS32_MIDI_SendDebugMessage("MBHP_MF_NG event forward: %02X %02X %02X",
				     midi_package.evnt0,
				     midi_package.evnt1,
				     midi_package.evnt2);
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // -> MIDI Router
  MIDI_ROUTER_ReceiveSysEx(port, midi_in);

  // -> LC SysEx parser
  MM_SYSEX_Parser(port, midi_in);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
  // sets the LEDring (and Meter) patterns
  MM_VPOT_LEDRing_SRHandler();
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
  if( mm_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("APP_DIN_NotifyToggle(%3d, %d) -> DIN SR%d.D%d %s\n",
				 pin, pin_value,
				 (pin/8)+1, pin % 8, pin_value ? "depressed" : "pressed");
  }

  // branch to button handler
  MM_DIO_ButtonHandler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  if( mm_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("APP_ENC_NotifyChange(%3d, %d)\n", encoder, incrementer);
  }

  // encoder number 0: Jogwheel
  if( encoder == 0 )
    MM_VPOT_SendJogWheelEvent(incrementer);
  // encoder number 1..8: VPOT
  else if( encoder < 9 )
    MM_VPOT_SendENCEvent(encoder-1, incrementer);
  else {
    // here you could handle additional encoders
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  MM_MF_FaderEvent(pin, pin_value << 4); // convert 12bit to 16bit
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to handle LC functions
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

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // increment "global" ms counter
    ++counter_ms;

    // handles the update requests for VPOT LEDrings
    MM_VPOT_LEDRing_CheckUpdates();

    // handles the update requests for LEDs
    MM_DIO_LED_CheckUpdate();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This low-prio task handles the LCD
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  // init LCD
  MM_LCD_Init(0);

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call LCD screen handler
    MM_LCD_Update(0);
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
  MM_SYSEX_TimeOut(port);

  // print message on screen
  //SCS_Msg(SCS_MSG_L, 2000, "MIDI Protocol", "TIMEOUT !!!");

  return 0;
}
