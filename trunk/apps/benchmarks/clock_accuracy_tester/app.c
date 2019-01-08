// $Id$
/*
 * Clock Accuracy Tester
 * See README.txt for details
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
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////
#define STRING_MAX 80


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u32 timestamp_last;
  u32 delay_last;
  u32 delay_min;
  u32 delay_max;
} delay_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 DelayInit(delay_t* d, u8 including_min_max);
static s32 DelayUpdate(delay_t* d, u32 timestamp);

static s32 CONSOLE_Parse(mios32_midi_port_t port, u8 byte);

static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 timestamp_midi_start;

static delay_t d_tick;
static delay_t d_beat;
static delay_t d_measure;

static u32 total_delay;
static u32 midi_clock_ctr;

static volatile u8 print_message; // notifier

static char line_buffer[STRING_MAX];
static u16 line_ix;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // install MIDI Rx callback function
  MIOS32_MIDI_DirectRxCallback_Init(&NOTIFY_MIDI_Rx);

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("Measurement results will be displayed once a MIDI clock is received.");
  MIOS32_MIDI_SendDebugMessage("Type \"reset\" in MIOS terminal to reset the current measurements!");

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;

  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(CONSOLE_Parse);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD screen
  MIOS32_LCD_Clear();

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // send delay min/max changes to MIOS terminal
  while( 1 ) {
    if( print_message ) {
      MIOS32_IRQ_Disable();
      u32 c_total_delay = total_delay;
      u32 c_midi_clock_ctr = midi_clock_ctr;
      delay_t c_d_tick = d_tick;
      delay_t c_d_beat = d_beat;
      print_message = 0;
      MIOS32_IRQ_Enable();

      u32 bpm = 60000000 / c_d_beat.delay_last;
      u32 avg = c_midi_clock_ctr ? (c_total_delay / c_midi_clock_ctr) : 0;

      MIOS32_MIDI_SendDebugMessage("BPM %d.%d  -  tick min/avg/max = %d.%03d/%d.%03d/%d.%03d\n",
				   bpm / 1000, bpm % 1000,
				   c_d_tick.delay_min / 1000, c_d_tick.delay_min % 1000,
				   avg / 1000, avg % 1000,
				   c_d_tick.delay_max / 1000, c_d_tick.delay_max % 1000);

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintFormattedString("  BPM    Min    Avg    Max     ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintFormattedString("%3d.%03d %2d.%03d %2d.%03d %2d.%03d",
				      bpm / 1000, bpm % 1000,
				      c_d_tick.delay_min / 1000, c_d_tick.delay_min % 1000,
				      avg / 1000, avg % 1000,
				      c_d_tick.delay_max / 1000, c_d_tick.delay_max % 1000);
    }
  }
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
}


/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 CONSOLE_Parse(mios32_midi_port_t port, u8 byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    // example for parsing the command:
    char *separators = " \t";
    char *brkt;
    char *parameter;

    if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {
      if( strcmp(parameter, "help") == 0 ) {
	MIOS32_MIDI_SendDebugMessage("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
	MIOS32_MIDI_SendDebugMessage("Following commands are available:");
	MIOS32_MIDI_SendDebugMessage("  reset:          clears the current measurements\n");
	MIOS32_MIDI_SendDebugMessage("  help:           this page\n");
      } else if( strcmp(parameter, "reset") == 0 ) {
	MIOS32_IRQ_Disable();
	u8 including_min_max = 1;
	DelayInit(&d_tick, including_min_max);
	DelayInit(&d_measure, including_min_max);
	DelayInit(&d_beat, including_min_max);
	midi_clock_ctr = 0;
	total_delay = 0;
	MIOS32_IRQ_Enable();

	MIOS32_MIDI_SendDebugMessage("Measurements have been cleared!\n");
      } else {
	MIOS32_MIDI_SendDebugMessage("Unknown command - type 'help' to list available commands!\n");
      }
    }

    line_ix = 0;

  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Delay Handlers
/////////////////////////////////////////////////////////////////////////////
static s32 DelayInit(delay_t* d, u8 including_min_max)
{
  d->timestamp_last = 0;
  d->delay_last = 0;

  if( including_min_max ) {
    d->delay_min = 0;
    d->delay_max = 0;
  }

  print_message = 1;

  return 0; // no error
}

static s32 DelayUpdate(delay_t* d, u32 timestamp)
{
  u32 delay = 0;

  if( d->timestamp_last ) {
    delay = timestamp - d->timestamp_last;

    if( !d->delay_min || delay < d->delay_min ) {
      d->delay_min = delay;
      print_message = 1;
    }

    if( delay > d->delay_max ) {
      d->delay_max = delay;
      print_message = 1;
    }

    d->delay_last = delay;
  }

  d->timestamp_last = timestamp;
  d->delay_last = delay;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // check for MIDI clock
  if( midi_byte == 0xf8 ) {
    u32 timestamp = MIOS32_TIMESTAMP_Get();

    DelayUpdate(&d_tick, timestamp);

    if( (midi_clock_ctr % 24) == 0 )
      DelayUpdate(&d_beat, timestamp);

    if( (midi_clock_ctr % 96) == 0 ) {
      DelayUpdate(&d_measure, timestamp);

      // force print message with each measure
      print_message = 1;
    }

    ++midi_clock_ctr;
    total_delay += d_tick.delay_last;

    return 0; // no error, no filtering
  }

  // check for MIDI start or continue
  if( midi_byte == 0xfa || midi_byte == 0xfb ) {
    u32 timestamp = MIOS32_TIMESTAMP_Get();

    timestamp_midi_start = timestamp;

    u8 including_min_max = 0;
    DelayInit(&d_tick, including_min_max);
    DelayInit(&d_measure, including_min_max);
    DelayInit(&d_beat, including_min_max);

    midi_clock_ctr = 0;
    total_delay = 0;

    return 0; // no error, no filtering
  }

  // check for MIDI stop
  if( midi_byte == 0xfc ) {
    // invalidate measured delays
    u8 including_min_max = 0;
    DelayInit(&d_tick, including_min_max);
    DelayInit(&d_measure, including_min_max);
    DelayInit(&d_beat, including_min_max);

    return 0; // no error, no filtering
  }

  return 0; // no error, no filtering
}
