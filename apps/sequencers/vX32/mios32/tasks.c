/*
 * FreeRTOS Tasks
 * only used by MIOS32 build, as pthread based Task handling is used on x86
 *
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "tasks.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_MIDI 			( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_RACK 			( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_PERIOD1MS		( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1mS(void *pvParameters);
static void TASK_MIDI(void *pvParameters);
static void TASK_Rack(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

xTaskHandle xRackHandle;
xTaskHandle xMIDIHandle;
xTaskHandle xPeriod1msHandle;


/////////////////////////////////////////////////////////////////////////////
// Initialize all tasks
/////////////////////////////////////////////////////////////////////////////
s32 TASKS_Init(u32 mode) {
  xTaskCreate(TASK_Period1mS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, &xPeriod1msHandle);
  xTaskCreate(TASK_MIDI, (signed portCHAR *)"MIDI", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI, &xMIDIHandle);
  xTaskCreate(TASK_Rack, (signed portCHAR *)"Rack", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_RACK, &xRackHandle);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period1mS(void *pvParameters) {
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

	while( 1 ) {
		vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
		// continue in application hook
		vx_task_period1ms();
	}
  
}

// use this function to resume the task
void vx_task_period1ms_resume(void) {
    vTaskResume(xPeriod1msHandle);
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles sequencer and MIDI events
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters) {
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

	while( 1 ) {
		vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
		
		// continue in application hook
		vx_task_midi();
	}
  
}

// use this function to resume the task
void vx_task_midi_resume(void) {
    vTaskResume(xMIDIHandle);
}


/////////////////////////////////////////////////////////////////////////////
// This task is triggered from SEQ_PATTERN_Change to transport the new patch
// into RAM
/////////////////////////////////////////////////////////////////////////////
static void TASK_Rack(void *pvParameters) {
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

	while( 1 ) {
		vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
		
		// continue in application hook
		vx_task_rack_tick();
	}
  
}

// use this function to resume the task
void vx_task_rack_resume(void) {
    vTaskResume(xRackHandle);
}
