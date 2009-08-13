// $Id$
/*
 * MIOS32 Tutorial #023: Exclusive access to LCD device (using a Mutex)
 * see README.txt for details
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
#include "app.h"


// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority levels for LCD tasks:
// use a lower priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_LCD1	( tskIDLE_PRIORITY + 2 )
// use the same priority like MIOS32 specific tasks (3)
#define PRIORITY_TASK_LCD2	( tskIDLE_PRIORITY + 3 )

// local prototype of the task functions
static void TASK_LCD1(void *pvParameters);
static void TASK_LCD2(void *pvParameters);


// define a Mutex for LCD access
xSemaphoreHandle xLCDSemaphore;
#define MUTEX_LCD_TAKE { while( xSemaphoreTakeRecursive(xLCDSemaphore, (portTickType)1) != pdTRUE ); }
#define MUTEX_LCD_GIVE { xSemaphoreGiveRecursive(xLCDSemaphore); }



/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // create Mutex for LCD access
  xLCDSemaphore = xSemaphoreCreateRecursiveMutex();

  // start tasks
  xTaskCreate(TASK_LCD1, (signed portCHAR *)"LCD1", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_LCD1, NULL);
  xTaskCreate(TASK_LCD2, (signed portCHAR *)"LCD2", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_LCD2, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle Status LED on each call of this function
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());
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
// The first task which accesses the LCD
// (running at lower priority than MIOS32 tasks)
/////////////////////////////////////////////////////////////////////////////
static void TASK_LCD1(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    // called again after 500 mS
    vTaskDelayUntil(&xLastExecutionTime, 500 / portTICK_RATE_MS);

    // print an animated vertical bar
    // we print characters multiple times to *ensure*
    // many conflicts with TASK_LCD2()
    // (this isn't a realistic usecase, but perfectly demonstrates
    // the need for a Mutex)

    int repeat, i, j;
    for(i=0; i<16; ++i) {
      for(repeat=0; repeat<100; ++repeat) {
	// request LCD access
	MUTEX_LCD_TAKE;

	// print my part of the LCD screen
	MIOS32_LCD_CursorSet(0, 0); // X, Y
	for(j=0; j<16; ++j)
	  MIOS32_LCD_PrintChar((j < i) ? '#' : ' ');

	// release LCD access for other tasks
	MUTEX_LCD_GIVE;
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// The second task which accesses the LCD
// (running at same priority as MIOS32 tasks)
/////////////////////////////////////////////////////////////////////////////
static void TASK_LCD2(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  // clear LCD
  MIOS32_LCD_Clear();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // print system time

    // request LCD access
    MUTEX_LCD_TAKE;

    // print my part of the LCD screen
    MIOS32_LCD_CursorSet(0, 1); // X, Y
    mios32_sys_time_t t = MIOS32_SYS_TimeGet();
    int hours = t.seconds / 3600;
    int minutes = (t.seconds % 3600) / 60;
    int seconds = (t.seconds % 3600) % 60;
    int milliseconds = t.fraction_ms;
    MIOS32_LCD_PrintFormattedString("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);

    // release LCD access for other tasks
    MUTEX_LCD_GIVE;
  }
}
