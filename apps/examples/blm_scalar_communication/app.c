// $Id$
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
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
#include <blm_scalar_master.h>

// FreeRTOS
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


/////////////////////////////////////////////////////////////////////////////
// FreeRTOS tasks and semaphores
/////////////////////////////////////////////////////////////////////////////

// this semaphore (resp. mutex) ensures exclusive access to the MIDI Out port
xSemaphoreHandle xMIDIOUTSemaphore;

// simplify access to this mutex
#define MUTEX_MIDIOUT_TAKE { TASKS_MUTEX_MIDIOUT_Take(); }
#define MUTEX_MIDIOUT_GIVE { TASKS_MUTEX_MIDIOUT_Give(); }

// periodic task
static void TASK_Period1mS(void *pvParameters);
#define PRIORITY_TASK_PERIOD1MS          ( tskIDLE_PRIORITY + 2 ) // lower than MIDI


/////////////////////////////////////////////////////////////////////////////
// Local functions
/////////////////////////////////////////////////////////////////////////////
static s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
static s32 APP_MIDI_TimeOut(mios32_midi_port_t port);

static s32 APP_BLM_ButtonCallback(u8 blm, blm_scalar_master_element_t element_type, u8 button_x, u8 button_y, u8 button_depressed);
static s32 APP_BLM_FaderCallback(u8 blm, u8 fader, u8 value);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  u8 blm = 0; // currently only a single blm device is supported

  // create semaphores
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize BLM_SCALAR_MASTER communication handler
  BLM_SCALAR_MASTER_Init(0);

  // install BLM callbacks
  BLM_SCALAR_MASTER_ButtonCallback_Init(APP_BLM_ButtonCallback);
  BLM_SCALAR_MASTER_FaderCallback_Init(APP_BLM_FaderCallback);

  // set MIDI port (change here if your MBHP_BLM_SCALAR is connected to a different port)
  BLM_SCALAR_MASTER_MIDI_PortSet(blm, UART2); // == IN3/OUT3

  // install SysEx callback
  MIOS32_MIDI_SysExCallback_Init(APP_SYSEX_Parser);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(APP_MIDI_TimeOut);

  // start tasks
  xTaskCreate(TASK_Period1mS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);

  // send layout request to MBHP_BLM_SCALAR
  BLM_SCALAR_MASTER_SendRequest(blm, 0x00);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
  u8 blm = 0; // currently only a single blm device is supported
  static blm_scalar_master_connection_state_t prev_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE;
  blm_scalar_master_connection_state_t connection_state = BLM_SCALAR_MASTER_ConnectionStateGet(blm);

  // print message on connection change and turn on/off the status LED
  if( connection_state != prev_connection_state ) {
    prev_connection_state = connection_state;

    if( connection_state == BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE ) {
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendDebugMessage("BLM has been disconnected."); // will appear ca. 10 seconds after communication is broken
      MIOS32_BOARD_LED_Set(1, 0);
      MUTEX_MIDIOUT_GIVE;
    } else {
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendDebugMessage("BLM has been connected."); // will appear ca. 5 seconds after BLM has been connected
      MIOS32_MIDI_SendDebugMessage("Reported grid layout: %dx%d with %d colours.", BLM_SCALAR_MASTER_NumColumnsGet(blm), BLM_SCALAR_MASTER_NumRowsGet(blm), BLM_SCALAR_MASTER_NumColoursGet(blm));
      MIOS32_BOARD_LED_Set(1, 1);
      MUTEX_MIDIOUT_GIVE;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // forward to BLM master
  BLM_SCALAR_MASTER_MIDI_Receive(port, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
static s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  // forward SysEx to BLM MASTER
  BLM_SCALAR_MASTER_SYSEX_Parser(port, midi_in);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function parses an incoming sysex stream for MIOS32 commands
/////////////////////////////////////////////////////////////////////////////
static s32 APP_MIDI_TimeOut(mios32_midi_port_t port)
{
  // forward timeout to BLM MASTER
  BLM_SCALAR_MASTER_MIDI_TimeOut(port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a BLM button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
static s32 APP_BLM_ButtonCallback(u8 blm, blm_scalar_master_element_t element_id, u8 button_x, u8 button_y, u8 button_depressed)
{
  // for demo purposes we implemented following:
  // - message will be print whenever a button has been pressed
  //   later you want to remove these messages from your application, because they consume time!
  // - button toggles between green and off
  // - if SHIFT button is pressed, the button toggles between red and off
  // - the SHIFT button toggles between yellow and off

  // the BLM_SCALAR driver doesn't store the button state, but we can retrieve it with the colour
  u8 shift_button_pressed = BLM_SCALAR_MASTER_LED_Get(blm, BLM_SCALAR_MASTER_ELEMENT_SHIFT, 0, 0) == BLM_SCALAR_MASTER_COLOUR_YELLOW;

  // current colour
  blm_scalar_master_colour_t colour = BLM_SCALAR_MASTER_LED_Get(blm, element_id, button_x, button_y);

  // set new colour
  if( element_id == BLM_SCALAR_MASTER_ELEMENT_SHIFT ) {
    blm_scalar_master_colour_t new_colour = button_depressed ? BLM_SCALAR_MASTER_COLOUR_OFF : BLM_SCALAR_MASTER_COLOUR_YELLOW;
    BLM_SCALAR_MASTER_LED_Set(blm, element_id, button_x, button_y, new_colour);
  } else {
    if( !button_depressed ) {
      blm_scalar_master_colour_t new_colour = BLM_SCALAR_MASTER_COLOUR_OFF;
      if( colour != BLM_SCALAR_MASTER_COLOUR_OFF ) {
	new_colour = BLM_SCALAR_MASTER_COLOUR_OFF;
      } else {
	new_colour = shift_button_pressed ? BLM_SCALAR_MASTER_COLOUR_RED : BLM_SCALAR_MASTER_COLOUR_GREEN;
      }

      BLM_SCALAR_MASTER_LED_Set(blm, element_id, button_x, button_y, new_colour);
    }
  }


  // print messages for debugging purposes
  switch( element_id ) {
  case BLM_SCALAR_MASTER_ELEMENT_GRID: {
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendDebugMessage("BLM Grid Button x=%d y=%d %s\n", button_x, button_y, button_depressed ? "depressed" : "pressed");
    MUTEX_MIDIOUT_GIVE;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW: {
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendDebugMessage("BLM Extra Row Button: x=%d y=%d %s\n", button_x, button_y, button_depressed ? "depressed" : "pressed");
    MUTEX_MIDIOUT_GIVE;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN: {
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendDebugMessage("BLM Extra Column Button: x=%d y=%d %s\n", button_x, button_y, button_depressed ? "depressed" : "pressed");
    MUTEX_MIDIOUT_GIVE;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_SHIFT: {
    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendDebugMessage("BLM Shift Button: x=%d y=%d %s\n", button_x, button_y, button_depressed ? "depressed" : "pressed");
    MUTEX_MIDIOUT_GIVE;
  } break;

  default:
    return -1; // unexpected element_id
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called when a BLM fader has been moved
/////////////////////////////////////////////////////////////////////////////
static s32 APP_BLM_FaderCallback(u8 blm, u8 fader, u8 value)
{
  // just forward as CC event
  MUTEX_MIDIOUT_TAKE;
  MIOS32_MIDI_SendCC(DEFAULT, fader, 1, value);
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
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
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1mS(void *pvParameters)
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

    // call BLM handler, e.g. to send LED updates
    BLM_SCALAR_MASTER_Periodic_mS();
  }
}


/////////////////////////////////////////////////////////////////////////////
// Function to take/give MIDI OUT Semaphore
// Note: mios32_defines.h provides special macros which call this function,
// so that it can also be used by drivers (such as BLM_SCALAR_MASTER)
/////////////////////////////////////////////////////////////////////////////

void TASKS_MUTEX_MIDIOUT_Take(void)
{
  if( xMIDIOUTSemaphore ) {
    while( xSemaphoreTakeRecursive(xMIDIOUTSemaphore, (portTickType)1) != pdTRUE );
  }
}

void TASKS_MUTEX_MIDIOUT_Give(void)
{
  if( xMIDIOUTSemaphore ) {
    xSemaphoreGiveRecursive(xMIDIOUTSemaphore);
  }
}
