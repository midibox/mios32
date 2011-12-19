// $Id$
/*
 * AINSER Jitter Monitor
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include <ainser.h>
#include "app.h"


// include everything FreeRTOS related
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for AINSER_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_AINSER_SCAN ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_AINSER_Scan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static char line_buffer[STRING_MAX];
static u16 line_ix;


// changed via terminal
static u8 send_cc = 0;   // sending CCs?
static u8 check_ain_module = 1; // AINSER module which should be checked (0=off, 1)
static u8 check_ain_mux = 1;    // AINSER multiplexer which should be checked (0=off, 1..8)

// captured min/max values
static u16 ain_value_min[AINSER_NUM_MODULES][AINSER_NUM_PINS];
static u16 ain_value_max[AINSER_NUM_MODULES][AINSER_NUM_PINS];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_Parse(mios32_midi_port_t port, char byte);
static s32 TERMINAL_ParseLine(char *input, void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize the AINSER module(s)
  AINSER_Init(0);

  // Jitter Mon requires that deadband function is disabled
  AINSER_DeadbandSet(0);

  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  // start task
  xTaskCreate(TASK_AINSER_Scan, (signed portCHAR *)"AINSER_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_AINSER_SCAN, NULL);

  // print welcome message
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("============================\n");
  MIOS32_MIDI_SendDebugMessage("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
  MIOS32_MIDI_SendDebugMessage("============================\n");
  MIOS32_MIDI_SendDebugMessage("Enter 'help' for available commands!");
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
  // MIOS32_AIN not used here!!!
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value)
{
  // capture min/max values
  if( pin_value < ain_value_min[module][pin] )
    ain_value_min[module][pin] = pin_value;
  if( pin_value > ain_value_max[module][pin] )
    ain_value_max[module][pin] = pin_value;

  if( send_cc ) {
    // convert 12bit value to 7bit value
    u8 value_7bit = pin_value >> 5;

    // send MIDI event
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, pin + 0x10, value_7bit);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task scans AINSER pins and checks for updates
/////////////////////////////////////////////////////////////////////////////
static void TASK_AINSER_Scan(void *pvParameters)
{
  u32 check_ctr = 0;
  portTickType xLastExecutionTime;

  // clear min/max values
  int module, pin;
  for(module=0; module<AINSER_NUM_MODULES; ++module) {
    for(pin=0; pin<AINSER_NUM_PINS; ++pin) {
      ain_value_min[module][pin] = 0xffff;
      ain_value_max[module][pin] = 0x0000;
    }
  }

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // toggle Status LED
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // scan pins
    AINSER_Handler(APP_AINSER_NotifyChange);

    // print message each second
    if( ++check_ctr >= 1000 ) {
      check_ctr = 0;

      if( check_ain_module && check_ain_mux ) {
	char buffer[100];
	u32 module = check_ain_module - 1;
	u32 pin0 = (check_ain_mux - 1) * 8;
	int i;

	MIOS32_MIDI_SendDebugMessage(".");
	MIOS32_MIDI_SendDebugMessage("Module %d Pin %4d %4d %4d %4d %4d %4d %4d %4d\n",
				     check_ain_module,
				     pin0+0, pin0+1, pin0+2, pin0+3, pin0+4, pin0+5, pin0+6, pin0+7);

	sprintf(buffer, "         Min ");
	for(i=0; i<8; ++i) {
	  u16 min = ain_value_min[module][pin0+i];
	  if( min == 0xffff )
	    sprintf(buffer, "%s  -- ", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, min);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);

	sprintf(buffer, "         Max ");
	for(i=0; i<8; ++i) {
	  u16 max = ain_value_max[module][pin0+i];
	  if( max == 0x0000 )
	    sprintf(buffer, "%s  -- ", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, max);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);

	sprintf(buffer, "         Diff");
	for(i=0; i<8; ++i) {
	  u16 min = ain_value_min[module][pin0+i];
	  u16 max = ain_value_max[module][pin0+i];
	  if( min > max )
	    sprintf(buffer, "%s  -- ", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, max-min);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);
      }

      // clear min/max values
      for(module=0; module<AINSER_NUM_MODULES; ++module) {
	for(pin=0; pin<AINSER_NUM_PINS; ++pin) {
	  ain_value_min[module][pin] = 0xffff;
	  ain_value_max[module][pin] = 0x0000;
	}
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
// help function which parses for on or off
// returns 0 if 'off', 1 if 'on', -1 if invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_on_off(char *word)
{
  if( strcmp(word, "on") == 0 )
    return 1;

  if( strcmp(word, "off") == 0 )
    return 0;

  return -1;
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  check_module <off|1..%d>: which AIN module should be checked (current: %d)\n", AINSER_NumModulesGet(), check_ain_module);
      out("  check_mux <off|1..8>:     which AINSER multiplexer should be checked (current: %d)\n", check_ain_mux);
      out("  cc <on|off>:              send CC on AIN pin changes (currently: %s)\n", send_cc ? "on" : "off");
      out("  deadband <0..255>:        sets the AIN deadband (currently: %d)\n", AINSER_DeadbandGet());
      out("  reset:                    resets the MIDIbox (!)\n");
      out("  help:                     this page");
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();

    } else if( strcmp(parameter, "check_module") == 0 ) {
      s32 on_off = -1;
      s32 module = -1;
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	on_off = get_on_off(parameter);
	module = get_dec(parameter);
      }

      if( on_off == 0 || module == 0 ) {
	check_ain_module = 0;
	out("Jitter Monitor has been disabled");
      } else if( module >= 1 && module <= AINSER_NumModulesGet() ) {
	check_ain_module = module;
	out("Jitter Monitor checks AINSER module %d", check_ain_module);
      } else {
	out("ERROR: please specifiy AINSER module 1..%d or 'off' to disable checks!", AINSER_NumModulesGet());
      }

    } else if( strcmp(parameter, "check_mux") == 0 ) {
      s32 on_off = -1;
      s32 mux = -1;
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	on_off = get_on_off(parameter);
	mux = get_dec(parameter);
      }

      if( on_off == 0 || mux == 0 ) {
	check_ain_mux = 0;
	out("Jitter Monitor has been disabled");
      } else if( mux >= 1 && mux <= 8 ) {
	check_ain_mux = mux;
	out("Jitter Monitor checks AINSER mux %d", check_ain_mux);
      } else {
	out("ERROR: please specifiy AINSER mux 1..8 or 'off' to disable checks!");
      }

    } else if( strcmp(parameter, "cc") == 0 ) {
      s32 on_off = -1;
      if( (parameter = strtok_r(NULL, separators, &brkt)) )
	on_off = get_on_off(parameter);

      if( on_off < 0 ) {
	out("Expecting 'on' or 'off'!");
      } else {
	send_cc = on_off;
	if( send_cc ) {
	  out("CCs will be sent in AIN pin changes.");
	} else {
	  out("CCs won't be sent in AIN pin changes.");
	}
      }

    } else if( strcmp(parameter, "deadband") == 0 ) {
      s32 deadband = -1;
      if( (parameter = strtok_r(NULL, separators, &brkt)) )
	deadband = get_dec(parameter);

      if( deadband < 0 || deadband > 255 ) {
	out("ERROR: deadband must be between 0..255");
      } else {
	AINSER_DeadbandSet(deadband);
	out("Deadband set to %d", AINSER_DeadbandGet());
      }
      
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}
