$Id$

MIOS32 Tutorial #006: RTOS Tasks
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar
   o 12 Jumpers for J5A/J5B/J5C, or a cable to ground

===============================================================================

In the previous lesson we learnt how to poll J5 pins to trigger MIDI notes.
Now we want to debounce the pins by polling the pins with a defined period, and
delaying the polling for a given time whenever the pin state has been changed
(low-pass filter function)

The proposed method for such frequently repeated "jobs" is the usage of
FreeRTOS tasks. They are explained in great detail under http://www.FreeRTOS.org,
in this example I will only explain the usage in a MIOS32 application.


Two tasks are already running in background, they are defined under
$MIOS32_PATH/programming_models/traditional/main.c

One task polls for MIDI input, and the other scans for SRIO, AIN and COM
activity (part of some following tutorial lessons)


Both tasks are running with a priority of 3.
The allowed priority range is between 0 and 5, where 0 is the lowest
priority (equal to the APP_Background task), and 5 the highest priority.

Tasks with higher priority can always interrupt tasks with lower priority.
Accordingly, tasks with lower priority will only run when no higher priority
task is executed.
If --- and this is the big advantage of a RTOS scheduler --- tasks at
the same priority level are running for more than 1 mS (the system tick),
the current task will be interrupted automatically and the next waiting 
task with the same priority is started.
This is called preemption -> http://en.wikipedia.org/wiki/Preemption_(computing)

You will love this feature, because it allows to run time consuming
loops quasi-parallel without taking care for compute time sharing (so
long tasks are running at the same priority level).

However, usually one one task would run endless: the idle task APP_Background().
All other tasks would wait for a certain number of system ticks (multiple
of 1 ms), would be woken up by the schedule, and do some quick
processing before waiting for the next cycle.

This is what we will do in our J5 input scanning routine.


First let's include some FreeRTOS specific header files, and let's
define a priority for our task at the top of app.c (to improve the
oversight):


-------------------------------------------------------------------------------
// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for J5_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_J5_SCAN	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_J5_Scan(void *pvParameters);
-------------------------------------------------------------------------------


Inside the APP_Init() hook we start the scan task the following way:
-------------------------------------------------------------------------------
  // start task
  xTaskCreate(TASK_J5_Scan, "J5_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_J5_SCAN, NULL);
-------------------------------------------------------------------------------


The J5_Scan task can be found at the bottom of app.c:
-------------------------------------------------------------------------------
static void TASK_J5_Scan(void *pvParameters)
{
  u8 old_state[12]; // to store the state of 12 pins
  portTickType xLastExecutionTime;

  // initialize pin state to inactive value
  int pin;
  for(pin=0; pin<12; ++pin)
    old_state[pin] = 1;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

    // check each J5 pin for changes
    int pin;
    for(pin=0; pin<12; ++pin) {
      u8 new_state = MIOS32_BOARD_J5_PinGet(pin);

      // pin changed?
      if( new_state != old_state[pin] ) {
	// store new state
	old_state[pin] = new_state;

	// send Note On/Off Event depending on new pin state
	if( new_state == 0 ) // Jumper closed -> 0V
	  MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, 0x3c + pin, 127);
	else                 // Jumper opened -> ca. 3.3V
	  MIOS32_MIDI_SendNoteOff(DEFAULT, Chn1, 0x3c + pin, 0);
      }
    }
  }
}
-------------------------------------------------------------------------------


What is the big difference compared to the variant in 005_polling_j5_pins:

  - this task loops each millisecond (1 / portTICK_RATE_MS)

  - it is *guaranteed* that this task will get CPU time within 1 mS
    (assumed, that no higher prio task blocks for more than 1 mS)

  - now we have a time base which allows proper debouncing.


Debouncing J5 pins: each pin gets a dedicated counter which counts
down on each pin state change until zero is reached again, before
checking the pin state again.

Since the task loops each mS, initialising the counter with 20
*guarantees* a delay of 20 mS - thats quite handy, isn't it?


The code variant with debounce counters can be found in app.c

===============================================================================
