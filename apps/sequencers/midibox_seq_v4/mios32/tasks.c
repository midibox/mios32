// $Id$
/*
 * FreeRTOS Tasks
 * only used by MIOS32 build, as a Cocoa based Task handling is used on MacOS
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

#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN handler
xSemaphoreHandle xMIDIINSemaphore;


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_MIDI		( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_PERIOD1MS		( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_PERIOD1S		( tskIDLE_PRIORITY + 1 )
#define PRIORITY_TASK_PATTERN           ( tskIDLE_PRIORITY + 1 )


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters);
static void TASK_Period1mS(void *pvParameters);
static void TASK_Period1S(void *pvParameters);
static void TASK_Pattern(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static xTaskHandle xPatternHandle;

 
/////////////////////////////////////////////////////////////////////////////
// Initialize all tasks
/////////////////////////////////////////////////////////////////////////////
s32 TASKS_Init(u32 mode)
{
  // start tasks
  xTaskCreate(TASK_MIDI,      (signed portCHAR *)"MIDI",      configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI, NULL);
  xTaskCreate(TASK_Period1mS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);
  xTaskCreate(TASK_Period1S,  (signed portCHAR *)"Period1S",  configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1S, NULL);
  xTaskCreate(TASK_Pattern,   (signed portCHAR *)"Pattern",   configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PATTERN, &xPatternHandle);

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateMutex();
  xMIDIINSemaphore = xSemaphoreCreateMutex();
  // TODO: here we could check for NULL and bring MBSEQ into halt state

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // continue in application hook
    SEQ_TASK_MIDI();
  }
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

    // continue in application hook
    SEQ_TASK_Period1mS();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1S(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1000 / portTICK_RATE_MS);

    // continue in application hook
    SEQ_TASK_Period1S();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is triggered from SEQ_PATTERN_Change to transport the new patch
// into RAM
/////////////////////////////////////////////////////////////////////////////
static void TASK_Pattern(void *pvParameters)
{
  do {
    // suspend task - will be resumed from SEQ_PATTERN_Change()
    vTaskSuspend(NULL);

    SEQ_TASK_Pattern();
  } while( 1 );
}

// use this function to resume the task
void SEQ_TASK_PatternResume(void)
{
    vTaskResume(xPatternHandle);
}
