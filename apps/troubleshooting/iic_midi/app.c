// $Id$
/*
 * Simple MIOS32_IIC_MIDI check
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
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

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include "app.h"


// from mios32_iic
extern volatile u32 MIOS32_IIC_unexpected_event;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 TERMINAL_Parse(mios32_midi_port_t port, char byte);
static s32 TERMINAL_ParseLine(char *input, void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 100
static char line_buffer[STRING_MAX];
static u16 line_ix;

static u8 outtest = 0;


/////////////////////////////////////////////////////////////////////////////
// Tasks
/////////////////////////////////////////////////////////////////////////////
xSemaphoreHandle xMIDIOUTSemaphore;
# define MUTEX_MIDIOUT_TAKE { while( xSemaphoreTakeRecursive(xMIDIOUTSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIOUT_GIVE { xSemaphoreGiveRecursive(xMIDIOUTSemaphore); }


#define PRIORITY_TASK_PERIOD_1mS ( tskIDLE_PRIORITY + 3 )
static void TASK_Period_1mS(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();

  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Enter 'help' for available tests\n");

  // start tasks
  xTaskCreate(TASK_Period_1mS, (signed portCHAR *)"1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS, NULL);
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
  if( (port & 0xf0) == IIC0 ) {
    // toggle Status LED
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    MUTEX_MIDIOUT_TAKE;
    MIOS32_MIDI_SendDebugMessage("IIC%d received %02x %02x %02x\n",
				 (port & 0xf) + 1,
				 midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
    MUTEX_MIDIOUT_GIVE;
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


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Terminal
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

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


static s32 TERMINAL_Ping(u8 iic_midi, void *_output_function, u8 success_message)
{
  s32 status;

  if( (status=MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Non_Blocking)) < 0 )
    MIOS32_MIDI_SendDebugMessage("IIC%d: MIOS32_IIC_TransferBegin failed with %d!\n", iic_midi+1, status);
  else {
    if( (status=MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_midi, NULL, 0)) < 0 )
      MIOS32_MIDI_SendDebugMessage("IIC%d: MIOS32_IIC_Transfer failed with %d!\n", iic_midi+1, status);
    else {
      if( (status=MIOS32_IIC_TransferWait(MIOS32_IIC_MIDI_PORT)) < 0 )
	MIOS32_MIDI_SendDebugMessage("IIC%d: MIOS32_IIC_TransferWait failed with %d (event: %02x)!\n", iic_midi+1, status, MIOS32_IIC_unexpected_event);
      else {
	MIOS32_IIC_TransferFinished(MIOS32_IIC_MIDI_PORT);
	if( success_message ) {
	  MIOS32_MIDI_SendDebugMessage("IIC%d: MBHP_IIC_MIDI module replied! :-)\n", iic_midi+1);
	}
      }
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  scan:                    scan for available IIC_MIDI interfaces");
      out("  ping:                    ping all IIC_MIDI interfaces");
      out("  stress:                  stressing all IIC_MIDI interfaces for ca. 1 second");
      out("  outtest:                 enables/disables MIDI OUT test (sends a CC each 200 mS)");
      out("  reset:                   resets the MIDIbox (!)\n");
      out("  help:                    this page");
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "scan") == 0 ) {
      s32 status;
      if( (status=MIOS32_IIC_MIDI_ScanInterfaces()) < 0 )
	MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_MIDI_ScanInterfaces() failed with %d\n", status);

      int i;
      for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i)
	MIOS32_MIDI_SendDebugMessage("IIC%d %savailable\n", i+1, MIOS32_IIC_MIDI_CheckAvailable(i) ? "" : "NOT ");
    } else if( strcmp(parameter, "ping") == 0 ) {
      int i;
      for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i)
	TERMINAL_Ping(i, _output_function, 1);
    } else if( strcmp(parameter, "stress") == 0 ) {
      int i;
      for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i) {
	if( MIOS32_IIC_MIDI_CheckAvailable(i) ) {
	  int j;
	  s32 num_loops = 100;
	  s32 error_count = 0;
	  for(j=0; j<num_loops; ++j) {
	    s32 status;
	    if( (status=TERMINAL_Ping(i, _output_function, 0)) < 0 ) {
	      ++error_count;
	    }
	  }

	  MIOS32_MIDI_SendDebugMessage("IIC%d: %d errors after %d transfers\n", i+1, error_count, num_loops);
	}
      }
    } else if( strcmp(parameter, "outtest") == 0 ) {
      if( outtest ) {
	outtest = 0;
	out("IIC_MIDI OUT test has been disabled.");
      } else {
	outtest = 1;
	out("IIC_MIDI OUT test has been enabled.");
      }
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  MUTEX_MIDIOUT_TAKE;

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

  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}


////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS to periodic IIC scans
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS(void *pvParameters)
{
  u32 iic_available = 0;
  u8 tested_module = 0;
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 200 / portTICK_RATE_MS); // each 200 mS

    MUTEX_MIDIOUT_TAKE;

    s32 status;
    if( (status=MIOS32_IIC_MIDI_ScanInterfaces()) < 0 ) {
      MIOS32_MIDI_SendDebugMessage("MIOS32_IIC_MIDI_ScanInterfaces() failed with %d\n", status);
    } else {
      // check if module availability has changed
      {
	int i;
	u32 mask = 1;
	for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i, mask<<=1) {
	  if( !(iic_available & mask) && MIOS32_IIC_MIDI_CheckAvailable(i) ) {
	    iic_available |= mask;
	    MIOS32_MIDI_SendDebugMessage("MBHP_IIC_MIDI module #%d connected!\n", i+1);
	  } else if( (iic_available & mask) && !MIOS32_IIC_MIDI_CheckAvailable(i) ) {
	    iic_available &= ~mask;
	    MIOS32_MIDI_SendDebugMessage("MBHP_IIC_MIDI module #%d has been disconnected!\n", i+1);
	  }
	}
      }

      // find next available interface and send a CC
      if( outtest ) {
	int i;
	u8 module_found = 0;
	for(i=0; i<MIOS32_IIC_MIDI_NUM; ++i) {
	  int iic = (i + tested_module + 1) % MIOS32_IIC_MIDI_NUM;
	  if( MIOS32_IIC_MIDI_CheckAvailable(iic) ) {
	    tested_module = iic;
	    module_found = 1;
	    break;
	  }
	}
	if( module_found ) {
	  if( (status=MIOS32_MIDI_SendCC(IIC0 + tested_module, Chn1, 0x07, 0x7f)) ) {
	    MIOS32_MIDI_SendDebugMessage("Failed to send CC on IIC%d!\n", tested_module+1);
	  }
	}
      }
    }

    MUTEX_MIDIOUT_GIVE;
  }
}
