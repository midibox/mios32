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

#include "tasks.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "seq.h"
#include "file.h"
#include "mid_file.h"
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_SEQ		( tskIDLE_PRIORITY + 4 ) // higher priority than MIDI receive task!


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;


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

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();

  // initialize file functions
  FILE_Init(0);
  
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
  // set LED depending on sequencer run state
  MIOS32_BOARD_LED_Set(1, SEQ_BPM_IsRunning() ? 1 : 0);
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
  // Note On received?
  if( midi_package.type == NoteOn &&
      midi_package.chn == Chn1 &&
      midi_package.velocity > 0 ) {

    // determine base key number (0=C, 1=C#, 2=D, ...)
    u8 base_key = midi_package.note % 12;

    // branch depending on note
    switch( base_key ) {
      case 0: // "C" starts the sequencer
	MIOS32_MIDI_SendDebugMessage("Start\n");
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();
	// start sequencer
	SEQ_BPM_Start();
	break;

      case 2: // "D" stops the sequencer. If pressed twice, the sequencer will be reset
	MIOS32_MIDI_SendDebugMessage("Stop\n");
	if( SEQ_BPM_IsRunning() )
	  SEQ_BPM_Stop();          // stop sequencer
	else
	  SEQ_Reset(1);            // reset sequencer
	break;

      case 4: // "E" pauses the sequencer
	// if in auto mode and BPM generator is clocked in slave mode:
	// change to master mode
	SEQ_BPM_CheckAutoMaster();

	// toggle pause mode
	seq_pause ^= 1;

	MIOS32_MIDI_SendDebugMessage("Pause %s\n", seq_pause ? "on" : "off");

	// execute stop/continue depending on new mode
	if( seq_pause )
	  SEQ_BPM_Stop();         // stop sequencer
	else
	  SEQ_BPM_Cont();         // continue sequencer
	break;

      case 5: // "F" switches to next file
	MIOS32_MIDI_SendDebugMessage("Next File\n");
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
  u16 sdcard_check_ctr = 0;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // execute sequencer handler
    SEQ_Handler();

    // send timestamped MIDI events
    SEQ_MIDI_OUT_Handler();

    // each second: check if SD Card (still) available
    if( ++sdcard_check_ctr >= 1000 ) {
      sdcard_check_ctr = 0;

      // use a mutex if multiple tasks access the SD Card!
      MUTEX_SDCARD_TAKE;
      s32 status = FILE_CheckSDCard();

      if( status == 1 ) {
	MIOS32_MIDI_SendDebugMessage("SD Card connected: %s\n", FILE_VolumeLabel());
      } else if( status == 2 ) {
	MIOS32_MIDI_SendDebugMessage("SD Card disconnected\n");

	// stop sequencer
	SEQ_BPM_Stop();

	// change filename
	sprintf(MID_FILE_UI_NameGet(), "No SD Card");
      } else if( status == 3 ) {
	if( !FILE_SDCardAvailable() ) {
	  MIOS32_MIDI_SendDebugMessage("SD Card not found\n");
	  // change filename
	  sprintf(MID_FILE_UI_NameGet(), "No SD Card");
	} else if( !FILE_VolumeAvailable() ) {
	  MIOS32_MIDI_SendDebugMessage("ERROR: SD Card contains invalid FAT!\n");
	  MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
	  // change filename
	  sprintf(MID_FILE_UI_NameGet(), "No FAT");
	  // stop sequencer
	  SEQ_BPM_Stop();
	} else {
	  // change filename
	  sprintf(MID_FILE_UI_NameGet(), "SDCard found");
	  // if in auto mode and BPM generator is clocked in slave mode:
	  // change to master mode
	  SEQ_BPM_CheckAutoMaster();
	  // reset sequencer
	  SEQ_Reset(1);
	  // request to play the first file
	  SEQ_PlayFileReq(0);
	  // start sequencer
	  SEQ_BPM_Start();
	}
      }
      MUTEX_SDCARD_GIVE;
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
