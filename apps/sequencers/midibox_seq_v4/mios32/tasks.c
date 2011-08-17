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

#include <msd.h>
#include "uip_task.h"

#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// for mutual exclusive SD Card access between different tasks
// The mutex is handled with MUTEX_SDCARD_TAKE and MUTEX_SDCARD_GIVE
// macros inside the application, which contain a different implementation 
// for emulation
xSemaphoreHandle xSDCardSemaphore;

// Mutex for MIDI IN/OUT handler
xSemaphoreHandle xMIDIINSemaphore;
xSemaphoreHandle xMIDIOUTSemaphore;

// Mutex for LCD access
xSemaphoreHandle xLCDSemaphore;

// Mutex for J16 access (SDCard/Ethernet)
xSemaphoreHandle xJ16Semaphore;


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MSD_DISABLED,
  MSD_INIT,
  MSD_READY,
  MSD_SHUTDOWN
} msd_state_t;


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_MIDI		 ( tskIDLE_PRIORITY + 4 )
#define PRIORITY_TASK_PERIOD1MS		 ( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_PERIOD1MS_LOW_PRIO ( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_PATTERN            ( tskIDLE_PRIORITY + 2 )

// priority of uIP task defined in uip_task.c (-> using 3)


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters);
static void TASK_Period1mS(void *pvParameters);
static void TASK_Period1mS_LowPrio(void *pvParameters);
static void TASK_Pattern(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static xTaskHandle xPatternHandle;

static volatile msd_state_t msd_state;

 
/////////////////////////////////////////////////////////////////////////////
// Initialize all tasks
/////////////////////////////////////////////////////////////////////////////
s32 TASKS_Init(u32 mode)
{
  // disable MSD by default (has to be enabled in SEQ_UI_FILE menu)
  msd_state = MSD_DISABLED;

  // create semaphores
  xSDCardSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIINSemaphore = xSemaphoreCreateRecursiveMutex();
  xMIDIOUTSemaphore = xSemaphoreCreateRecursiveMutex();
  xLCDSemaphore = xSemaphoreCreateRecursiveMutex();
  xJ16Semaphore = xSemaphoreCreateRecursiveMutex();
  // TODO: here we could check for NULL and bring MBSEQ into halt state

  // start tasks
  xTaskCreate(TASK_MIDI,              (signed portCHAR *)"MIDI",         configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI, NULL);
  xTaskCreate(TASK_Period1mS,         (signed portCHAR *)"Period1mS",    configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);
  xTaskCreate(TASK_Period1mS_LowPrio, (signed portCHAR *)"Period1mS_LP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS_LOW_PRIO, NULL);
  xTaskCreate(TASK_Pattern,           (signed portCHAR *)"Pattern",      configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PATTERN, &xPatternHandle);

  // finally init the uIP task
  UIP_TASK_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters)
{
  portTickType xLastExecutionTime;
  u8 lun_available = 0;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // skip delay gap if we had to wait for more than 5 ticks to avoid 
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

    // MSD driver handling overrules MIDI
    if( msd_state == MSD_DISABLED ) {
      // continue in application hook
      SEQ_TASK_MIDI();
    } else {
      MUTEX_SDCARD_TAKE;

      switch( msd_state ) {
      case MSD_SHUTDOWN:
	// switch back to USB MIDI
	MIOS32_USB_Init(1);
	msd_state = MSD_DISABLED;
	break;

      case MSD_INIT:
	// LUN not mounted yet
	lun_available = 0;

	// enable MSD USB driver
	MUTEX_J16_TAKE;
	if( MSD_Init(0) >= 0 )
	  msd_state = MSD_READY;
	else
	  msd_state = MSD_SHUTDOWN;
	MUTEX_J16_GIVE;
	break;

      case MSD_READY:
	// service MSD USB driver
	MSD_Periodic_mS();

	// this mechanism shuts down the MSD driver if SD card has been unmounted by OS
	if( lun_available && !MSD_LUN_AvailableGet(0) )
	  msd_state = MSD_SHUTDOWN;
	else if( !lun_available && MSD_LUN_AvailableGet(0) )
	  lun_available = 1;
	break;
      }

      MUTEX_SDCARD_GIVE;
    }
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

    // skip delay gap if we had to wait for more than 5 ticks to avoid 
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

    // continue in application hook
    SEQ_TASK_Period1mS();
  }
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS with low priority
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1mS_LowPrio(void *pvParameters)
{
  u16 ms_ctr = 0;

  while( 1 ) {
    // using vTaskDelay instead of vTaskDelayUntil, since a periodical execution
    // isn't required, and this task could be invoked too often if it was blocked
    // for a long time
    vTaskDelay(1 / portTICK_RATE_MS);

    // continue in application hook
    SEQ_TASK_Period1mS_LowPrio();

    // 1 second task
    if( ++ms_ctr >= 1000 ) {
      ms_ctr = 0;
      // continue in application hook
      SEQ_TASK_Period1S();
    }
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


/////////////////////////////////////////////////////////////////////////////
// MSD access functions
/////////////////////////////////////////////////////////////////////////////
s32 TASK_MSD_EnableSet(u8 enable)
{
  MIOS32_IRQ_Disable();
  if( msd_state == MSD_DISABLED && enable ) {
    msd_state = MSD_INIT;
  } else if( msd_state == MSD_READY && !enable )
    msd_state = MSD_SHUTDOWN;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 TASK_MSD_EnableGet()
{
  return (msd_state == MSD_READY) ? 1 : 0;
}

s32 TASK_MSD_FlagStrGet(char str[5])
{
  str[0] = MSD_CheckAvailable() ? 'U' : '-';
  str[1] = MSD_LUN_AvailableGet(0) ? 'M' : '-';
  str[2] = MSD_RdLEDGet(250) ? 'R' : '-';
  str[3] = MSD_WrLEDGet(250) ? 'W' : '-';
  str[4] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// functions to access J16 semaphore
// see also mios32_config.h
/////////////////////////////////////////////////////////////////////////////
void TASKS_J16SemaphoreTake(void)
{
  if( xJ16Semaphore != NULL && msd_state == MSD_DISABLED )
    MUTEX_J16_TAKE;
}

void TASKS_J16SemaphoreGive(void)
{
  if( xJ16Semaphore != NULL && msd_state == MSD_DISABLED )
    MUTEX_J16_GIVE;
}
