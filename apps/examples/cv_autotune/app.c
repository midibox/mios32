// $Id$
/*
 * CV Autotune
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
#include "app.h"
#include "frq_meter.h"

#include <notestack.h>
#include <aout.h>

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for FRQ_MEASURE task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_FRQ_MEASURE	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_Frq_Measure(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16

#define AUTOTUNE_STATE_NOP        0
#define AUTOTUNE_STATE_START      1
#define AUTOTUNE_STATE_SWEEP      2
#define AUTOTUNE_STATE_PASSED     3
#define AUTOTUNE_STATE_FAILED     4



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];

static volatile u8 autotune_state;
static u32 autotune_last_aout_value;
static u8 autotune_current_note;
static u16 autotune_note_aout_value[128];
static u32 autotune_note_frq[128];

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize frequency meter
  FRQ_METER_Init(0);

  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);

  // initialize AOUT module
  AOUT_Init(0);

  // configure interface
  // see AOUT module documentation for available interfaces and options
  aout_config_t config;
  config = AOUT_ConfigGet();
  config.if_type = AOUT_IF_TLV5630;
  config.if_option = 0;
  config.num_channels = 8;
  config.chn_inverted = 0;
  AOUT_ConfigSet(config);
  AOUT_IF_Init(0);

  // clear autotune state
  autotune_state = AUTOTUNE_STATE_NOP;

  // calculate note frequencies
  float fact = 1.059463;
  float a_freq = 13.75;
  int i;
  for(i=0; i<128; i+=12, a_freq *= 2) {
    int j;
    float note_frq[12];

    note_frq[9] = a_freq;
    for(j=10; j<=11; ++j)
      note_frq[j] = note_frq[j-1] * fact;
    for(j=8; j>=0; --j)
      note_frq[j] = note_frq[j+1] / fact;

    for(j=0; j<12 && (j+i)<128; ++j) {
      autotune_note_frq[i+j] = (int)(note_frq[j] * 1000);
      autotune_note_aout_value[i+j] = (i+j)*0x200; // default for V/Oct
    }
  }

  // start task
  xTaskCreate(TASK_Frq_Measure, "Frq_Measure", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_FRQ_MEASURE, NULL);
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
  // Note On received?
  if( midi_package.chn == Chn1 && 
      (midi_package.type == NoteOn || midi_package.type == NoteOff) ) {

    // skip if autotune running
    if( midi_package.type == NoteOn && midi_package.velocity > 0 ) {
      if( autotune_state != AUTOTUNE_STATE_NOP ) {
	MIOS32_MIDI_SendDebugMessage("Autotune running - Note ignored!\n");
	return;
      }

      // if Note c-2 is played: activate autotune
      if( midi_package.note == 0x00 ) {
	autotune_state = AUTOTUNE_STATE_START;
	return;
      }
    }


    // branch depending on Note On/Off event
    if( midi_package.event == NoteOn && midi_package.velocity > 0 ) {
      // push note into note stack
      NOTESTACK_Push(&notestack, midi_package.note, midi_package.velocity);
    } else {
      // remove note from note stack
      NOTESTACK_Pop(&notestack, midi_package.note);
    }


    // still a note in stack?
    if( notestack.len ) {
      // take first note of stack
      // we have to convert the 7bit value to a 16bit value
      //u16 note_cv = notestack_items[0].note << 9;
      u16 note_cv = autotune_note_aout_value[notestack_items[0].note];
      u16 velocity_cv = notestack_items[0].tag << 9;

      // change voltages
      AOUT_PinSet(0, note_cv);
      AOUT_PinSet(1, velocity_cv);

      // set board LED
      MIOS32_BOARD_LED_Set(1, 1);

      // set AOUT digital output (if available)
      AOUT_DigitalPinSet(0, 1);
    } else {
      // turn off LED (can also be used as a gate output!)
      MIOS32_BOARD_LED_Set(1, 0);

      // clear AOUT digital output (if available)
      AOUT_DigitalPinSet(0, 0);
    }

    // update AOUT channel(s)
    AOUT_Update();

#if 1
    // optional debug messages
    NOTESTACK_SendDebugMessage(&notestack);
#endif

  // CC received?
  } else if( midi_package.chn == Chn1 && midi_package.type == CC ) {

    // we have to convert the 7bit value to a 16bit value
    u16 cc_cv = midi_package.value << 9;

    switch( midi_package.cc_number ) {
      case 16: AOUT_PinSet(2, cc_cv); break;
      case 17: AOUT_PinSet(3, cc_cv); break;
      case 18: AOUT_PinSet(4, cc_cv); break;
      case 19: AOUT_PinSet(5, cc_cv); break;
      case 20: AOUT_PinSet(6, cc_cv); break;
      case 21: AOUT_PinSet(7, cc_cv); break;
    }

    // update AOUT channel(s)
    // register upload will be omitted automatically if no channel value has changed
    AOUT_Update();
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
// This task measures the frequency
/////////////////////////////////////////////////////////////////////////////
static void TASK_Frq_Measure(void *pvParameters)
{
  portTickType xLastExecutionTime;
  u8 frq_was_too_low = 0;
  u8 frq_was_too_high = 0;
  u32 last_frq = 0;
  u32 last_displayed_frq = 0;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    u32 frq = FRQ_METER_Tick();

    if( frq > FRQ_METER_TOO_LOW && frq < FRQ_METER_TOO_HIGH ) {

      // print new frequency if it changed more than 0.5%
      u32 frq_change = abs(frq-last_displayed_frq);
      u32 permill = (1000*frq_change) / frq;
      if( permill >= 5 ) {
	if( autotune_state == AUTOTUNE_STATE_NOP )
	  MIOS32_MIDI_SendDebugMessage("-> %3d.%02d Hz\n", frq / 1000, (frq % 1000) / 10);
	last_displayed_frq = frq;
      }

      // toggle Status LED to as a sign of live
      MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());
    } else {      
      if( frq != FRQ_METER_NO_UPDATE ) {
	if( frq == FRQ_METER_TOO_LOW )
	  ++frq_was_too_low;
	else
	  ++frq_was_too_high;

	if( frq_was_too_low >= 2 || frq_was_too_high >= 10 ) {
	  if( autotune_state == AUTOTUNE_STATE_NOP ) {
	    if( frq_was_too_low && !frq_was_too_high )
	      MIOS32_MIDI_SendDebugMessage("-> frequency too low!\n");
	    else if( frq_was_too_high && !frq_was_too_low )
	      MIOS32_MIDI_SendDebugMessage("-> frequency too high!\n");
	    else
	      MIOS32_MIDI_SendDebugMessage("-> frequency very unstable!\n");
	  }

	  frq_was_too_low = 0;
	  frq_was_too_high = 0;

	  last_displayed_frq = frq;
	}
      }
    }

    // autotune
    if( frq != FRQ_METER_NO_UPDATE ) {
      switch( autotune_state ) {
      case AUTOTUNE_STATE_START:
	MIOS32_MIDI_SendDebugMessage("Autotune started.\n");
	autotune_last_aout_value = 0;
	autotune_current_note = 0;
	AOUT_PinSet(0, autotune_last_aout_value);
	AOUT_Update();
	autotune_state = AUTOTUNE_STATE_SWEEP;
	break;

      case AUTOTUNE_STATE_SWEEP: {
	u32 search_frq = autotune_note_frq[autotune_current_note];
	MIOS32_MIDI_SendDebugMessage("0x%03x: %3d.%02d Hz (searching for #%d: %3d.%02d Hz)\n",
				     autotune_last_aout_value,
				     frq / 1000, (frq % 1000) / 10,
				     autotune_current_note,
				     search_frq / 1000, (search_frq % 1000) / 10);

	if( frq >= search_frq ) {
	  autotune_note_aout_value[autotune_current_note] = autotune_last_aout_value;
	  if( ++autotune_current_note >= 0x80 ) {
	    autotune_state = AUTOTUNE_STATE_PASSED;
	  }
	} else {
	  if( autotune_last_aout_value >= 0xfff0 ) {
	    autotune_state = AUTOTUNE_STATE_FAILED;
	  } else {
	    autotune_last_aout_value += 0x10;
	    AOUT_PinSet(0, autotune_last_aout_value);
	    AOUT_Update();
	  }
	}
      } break;

      case AUTOTUNE_STATE_PASSED:
	AOUT_PinSet(0, 0x0000);
	AOUT_Update();
	autotune_state = AUTOTUNE_STATE_NOP;
	MIOS32_MIDI_SendDebugMessage("Autotune finished!\n");
	break;

      case AUTOTUNE_STATE_FAILED:
	AOUT_PinSet(0, 0x0000);
	AOUT_Update();
	autotune_state = AUTOTUNE_STATE_NOP;
	MIOS32_MIDI_SendDebugMessage("Autotune failed, frequency for note >= %d not assigned!\n", autotune_current_note);
	break;
      }
    }


    // store current frequency
    if( frq != FRQ_METER_NO_UPDATE )
      last_frq = frq;
  }
}
