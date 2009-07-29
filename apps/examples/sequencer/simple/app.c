// $Id$
/*
 * Simple Sequencer
 * See README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_SEQ		( tskIDLE_PRIORITY + 3 ) // higher priority than MIDI receive task!


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_SEQ(void *pvParameters);
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize MIDI handler
  SEQ_MIDI_OUT_Init(0);

  // initialize sequencer
  SEQ_Init(0);

  // install MIDI Rx callback function
  MIOS32_MIDI_DirectRxCallback_Init(NOTIFY_MIDI_Rx);

  // install sequencer task
  xTaskCreate(TASK_SEQ, (signed portCHAR *)"SEQ", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_SEQ, NULL);

  // print first message
  print_msg = PRINT_MSG_INIT;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD screen
  MIOS32_LCD_Clear();

  // endless loop: print status information on LCD
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // new message requested?
    // TODO: add FreeRTOS specific queue handling!
    u8 new_msg = PRINT_MSG_NONE;
    portENTER_CRITICAL(); // port specific FreeRTOS function to disable IRQs (nested)
    if( print_msg ) {
      new_msg = print_msg;
      print_msg = PRINT_MSG_NONE; // clear request
    }
    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable IRQs (nested)

    switch( new_msg ) {
      case PRINT_MSG_INIT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintString("see README.txt   ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintString("for details     ");

	// wait 5 seconds
	int i;
	for(i=0; i<10; ++i)
	  MIOS32_DELAY_Wait_uS(50000);

	// start sequencer
	SEQ_BPM_Start();

	// and print status screen
	print_msg = PRINT_MSG_SEQ_STATUS;
	break;

      case PRINT_MSG_SEQ_STATUS:
      {
	u32 bpm_tick = SEQ_BPM_TickGet();
	u32 bpm_sub = bpm_tick % (SEQ_BPM_PPQN_Get()/4);
	u32 bpm_16th = (bpm_tick / (SEQ_BPM_PPQN_Get()/4)) % 16;
	u32 bpm_qn = (bpm_tick / SEQ_BPM_PPQN_Get()) % 4;
	u32 bpm_n = bpm_tick / (4*SEQ_BPM_PPQN_Get());
	MIOS32_LCD_CursorSet(0, 0);
	MIOS32_LCD_PrintFormattedString("%3d.%2d.%2d.%2d %s", 
					bpm_n+1, bpm_qn+1, (bpm_16th%4)+1, bpm_sub, 
					SEQ_BPM_IsMaster() ? "Mst" : "Slv");
        MIOS32_LCD_CursorSet(0, 1);
	if( seq_pause ) {
	  MIOS32_LCD_PrintFormattedString("SeqStatus: Pause");
	} else if( SEQ_BPM_IsRunning() ) {
	  MIOS32_LCD_PrintFormattedString("SeqStatus: Play ");
	} else {
	  MIOS32_LCD_PrintFormattedString("SeqStatus: Stop ");
	}

	// request status screen again (will stop once a new screen is requested by another task)
	print_msg = PRINT_MSG_SEQ_STATUS;
      }
      break;
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
  s32 error;

  // ignore if button has been depressed
  if( pin_value )
    return;

  // call function depending on button number
  switch( pin ) {
    case DIN_NUMBER_EXEC: // used as Play button
      // if in auto mode and BPM generator is clocked in slave mode:
      // change to master mode
      SEQ_BPM_CheckAutoMaster();

      // start sequencer
      SEQ_BPM_Start();
      break;

    case DIN_NUMBER_INC: // used as Stop button
      if( SEQ_BPM_IsRunning() )
	SEQ_BPM_Stop();          // stop sequencer
      else
	SEQ_Reset();             // reset sequencer
      break;

    case DIN_NUMBER_DEC: // used as Pause button
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

    case DIN_NUMBER_SNAPSHOT: // Snapshot button
      break;
  }
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

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // execute sequencer handler
    SEQ_Handler();

    // send timestamped MIDI events
    SEQ_MIDI_OUT_Handler();
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
