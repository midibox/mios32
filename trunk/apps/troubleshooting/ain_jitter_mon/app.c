// $Id$
/*
 * AIN Jitter Monitor
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

#include "app.h"


// include everything FreeRTOS related
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for AIN_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_AIN_SCAN ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_AIN_Scan(void *pvParameters);


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
static u8 check_ain = 1; // check enabled by default
static u8 send_cc = 0;   // sending CCs?

// captured min/max values
#define AIN_NUM_PINS_MAX 16
static u16 ain_value_min[AIN_NUM_PINS_MAX];
static u16 ain_value_max[AIN_NUM_PINS_MAX];


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

  // Jitter Mon requires that deadband function is disabled
  MIOS32_AIN_DeadbandSet(0);

  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  // start task
  xTaskCreate(TASK_AIN_Scan, (signed portCHAR *)"AIN_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_AIN_SCAN, NULL);

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
  if( pin >= AIN_NUM_PINS_MAX )
    return;

  // capture min/max values
  if( pin_value < ain_value_min[pin] )
    ain_value_min[pin] = pin_value;
  if( pin_value > ain_value_max[pin] )
    ain_value_max[pin] = pin_value;

  if( send_cc ) {
    // convert 12bit value to 7bit value
    u8 value_7bit = pin_value >> 5;

    // send MIDI event
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, pin + 0x10, value_7bit);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task scans AIN pins and checks for updates
/////////////////////////////////////////////////////////////////////////////
static void TASK_AIN_Scan(void *pvParameters)
{
  u32 check_ctr = 0;
  portTickType xLastExecutionTime;

  // clear min/max values
  int pin;
  for(pin=0; pin<AIN_NUM_PINS_MAX; ++pin) {
    ain_value_min[pin] = 0xffff;
    ain_value_max[pin] = 0x0000;
  }

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // toggle Status LED
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // print message each second
    if( ++check_ctr >= 1000 ) {
      check_ctr = 0;

      if( check_ain ) {
	int num_pins = AIN_NUM_PINS_MAX;
	char buffer[200];
	int i;

	MIOS32_MIDI_SendDebugMessage(".");

	sprintf(buffer, "Pin ");
	for(i=0; i<num_pins; ++i) {
	  sprintf(buffer, "%s %4d", buffer, i);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);

	sprintf(buffer, "Min ");
	for(i=0; i<num_pins; ++i) {
	  u16 min = ain_value_min[i];
	  if( min == 0xffff )
	    sprintf(buffer, "%s   --", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, min);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);

	sprintf(buffer, "Max ");
	for(i=0; i<num_pins; ++i) {
	  u16 max = ain_value_max[i];
	  if( max == 0x0000 )
	    sprintf(buffer, "%s   --", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, max);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);

	sprintf(buffer, "Diff");
	for(i=0; i<num_pins; ++i) {
	  u16 min = ain_value_min[i];
	  u16 max = ain_value_max[i];
	  if( min > max )
	    sprintf(buffer, "%s   --", buffer);
	  else
	    sprintf(buffer, "%s %4d", buffer, max-min);
	}
	MIOS32_MIDI_SendDebugMessage(buffer);
      }

      // clear min/max values
      for(pin=0; pin<AIN_NUM_PINS_MAX; ++pin) {
	ain_value_min[pin] = 0xffff;
	ain_value_max[pin] = 0x0000;
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
      out("  check <on|off>:           enables/disables the monitor (current: %s)\n", check_ain ? "on" : "off");
      out("  cc <on|off>:              send CC on AIN pin changes (currently: %s)\n", send_cc ? "on" : "off");
      out("  deadband <0..255>:        sets the AIN deadband (currently: %d)\n", MIOS32_AIN_DeadbandGet());
      out("  reset:                    resets the MIDIbox (!)\n");
      out("  help:                     this page");
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();

    } else if( strcmp(parameter, "check") == 0 ) {
      s32 on_off = -1;
      if( (parameter = strtok_r(NULL, separators, &brkt)) )
	on_off = get_on_off(parameter);

      if( on_off < 0 ) {
	out("Expecting 'on' or 'off'!");
      } else {
	check_ain = on_off;
	out("Jitter Monitor %s", check_ain ? "enabled" : "disabled");
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
	MIOS32_AIN_DeadbandSet(deadband);
	out("Deadband set to %d", MIOS32_AIN_DeadbandGet());
      }
      
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}
