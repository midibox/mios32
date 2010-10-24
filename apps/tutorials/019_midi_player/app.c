// $Id$
/*
 * MIOS32 Tutorial #019: A MIDI Player plays from SD Card
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

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "seq.h"
#include "mid_file.h"
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_SEQ		( tskIDLE_PRIORITY + 4 ) // higher priority than MIDI receive task!


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_SEQ(void *pvParameters);
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // turn off gate LED
  MIOS32_BOARD_LED_Set(1, 0);

  // initialize MIDI handler
  SEQ_MIDI_OUT_Init(0);

  // initialize sequencer
  SEQ_Init(0);

  // install MIDI Rx callback function
  MIOS32_MIDI_DirectRxCallback_Init(NOTIFY_MIDI_Rx);

  // install sequencer task
  xTaskCreate(TASK_SEQ, (signed portCHAR *)"SEQ", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_SEQ, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // set LED depending on sequencer run state
  MIOS32_BOARD_LED_Set(1, SEQ_BPM_IsRunning() ? 1 : 0);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // Note On received?
  if( midi_package.type == NoteOn &&
      midi_package.chn == Chn1 &&
      midi_package.velocity > 0 ) {

    // determine base key number (0=C, 1=C#, 2=D, ...)
    u8 base_key = midi_package.note % 12;

    // branch depending on note
    switch( base_key ) {
      case 0: // "C" starts the sequencer
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();
	// start sequencer
	SEQ_BPM_Start();
	break;

      case 2: // "D" stops the sequencer. If pressed twice, the sequencer will be reset
	if( SEQ_BPM_IsRunning() )
	  SEQ_BPM_Stop();          // stop sequencer
	else
	  SEQ_Reset();             // reset sequencer
	break;

      case 4: // "E" pauses the sequencer
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();

	// toggle pause mode
	seq_pause ^= 1;

	// execute stop/continue depending on new mode
	if( seq_pause )
	  SEQ_BPM_Stop();         // stop sequencer
	else
	  SEQ_BPM_Cont();         // continue sequencer
	break;

      case 5: // "F" switches to next file
	SEQ_PlayFileReq(1);
	break;
    }
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
// This task is called periodically each mS to handle sequencer requests
/////////////////////////////////////////////////////////////////////////////
static void TASK_SEQ(void *pvParameters)
{
  portTickType xLastExecutionTime;
  u8 sdcard_available = 0;
  int sdcard_check_ctr = 0;

  // initialize SD Card
  MIOS32_SDCARD_Init(0);

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // execute sequencer handler
    SEQ_Handler();

    // send timestamped MIDI events
    SEQ_MIDI_OUT_Handler();

    // each second: check if SD card (still) available
    if( ++sdcard_check_ctr >= 1000 ) {
      sdcard_check_ctr = 0;

      // check if SD card is available
      // High-speed access if SD card was previously available
      u8 prev_sdcard_available = sdcard_available;
      sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

      if( sdcard_available && !prev_sdcard_available ) {
	MIOS32_BOARD_LED_Set(0x1, 0x1); // turn on LED	
	MIOS32_MIDI_SendDebugMessage("SD Card has been connected!\n");

	s32 status;
	if( (status=MID_FILE_mount_fs()) < 0 ) {
	  MIOS32_MIDI_SendDebugMessage("File system cannot be mounted, status: %d\n", status);
	} else {
	  // if in auto mode and BPM generator is clocked in slave mode:
	  // change to master mode
	  SEQ_BPM_CheckAutoMaster();
	  // reset sequencer
	  SEQ_Reset();
	  // request to play the first file
	  SEQ_PlayFileReq(0);
	  // start sequencer
	  SEQ_BPM_Start();
	}
      } else if( !sdcard_available && prev_sdcard_available ) {
	MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
	MIOS32_MIDI_SendDebugMessage("SD Card has been disconnected!\n");

	// stop sequencer
	SEQ_BPM_Stop();
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // here we could filter a certain port
  // The BPM generator will deliver inaccurate results if MIDI clock 
  // is received from multiple ports
  SEQ_BPM_NotifyMIDIRx(midi_byte);

  return 0; // no error, no filtering
}
