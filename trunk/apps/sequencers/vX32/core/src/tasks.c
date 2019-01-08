/* $Id$ */
/*
 * FreeRTOS Tasks
 * only used by MIOS32 build, as pthread based Task handling is used on x86
 *
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <tasks.h>

#include <FreeRTOS.h>
#include <task.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_RACK          ( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_MIDI          ( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_UI            ( tskIDLE_PRIORITY + 1 )


/////////////////////////////////////////////////////////////////////////////
// Global Prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_UI(void *pvParameters);
static void TASK_MIDI(void *pvParameters);
static void TASK_Rack(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

xSemaphoreHandle xGraphSemaphore;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

xTaskHandle xRackHandle;
xTaskHandle xMIDIHandle;
xTaskHandle xUIHandle;


/////////////////////////////////////////////////////////////////////////////
// Initialize all tasks
/////////////////////////////////////////////////////////////////////////////

s32 TASKS_Init(u32 mode) {
    xTaskCreate(TASK_UI, (signed portCHAR *)"UI", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_UI, &xUIHandle);
    xTaskCreate(TASK_MIDI, (signed portCHAR *)"MIDI", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI, &xMIDIHandle);
    xTaskCreate(TASK_Rack, (signed portCHAR *)"Rack", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_RACK, &xRackHandle);

    xGraphSemaphore = xSemaphoreCreateMutex();

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This task handles User Interfacing to the Rack every 2mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_UI(void *pvParameters) {
    portTickType xLastExecutionTime;

    // Initialise the xLastExecutionTime variable on task entry
    xLastExecutionTime = xTaskGetTickCount();

    do {
        vTaskDelayUntil(&xLastExecutionTime, 2 / portTICK_RATE_MS);
        // continue in application hook
        vX_Task_UI();
    } while ( ENDLESSLOOP );
      
}

// use this function to resume the task
void vX_Task_UI_Resume(void) {
    vTaskResume(xUIHandle);
}


/////////////////////////////////////////////////////////////////////////////
// This task handles sequencer and MIDI events every mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_MIDI(void *pvParameters) {
    portTickType xLastExecutionTime;

    // Initialise the xLastExecutionTime variable on task entry
    xLastExecutionTime = xTaskGetTickCount();

    do {
        vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
        
        // continue in application hook
        vX_Task_MIDI();
    } while ( ENDLESSLOOP );
  
}

// use this function to resume the task
void vX_Task_MIDI_Resume(void) {
    vTaskResume(xMIDIHandle);
}



/////////////////////////////////////////////////////////////////////////////
// This task handles the Rack every mS
/////////////////////////////////////////////////////////////////////////////
static void TASK_Rack(void *pvParameters) {
    portTickType xLastExecutionTime;

    // Initialise the xLastExecutionTime variable on task entry
    xLastExecutionTime = xTaskGetTickCount();

    do {
        vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);
        
        vX_Task_Rack_Tick();
    } while( ENDLESSLOOP );
  
}

// use this function to resume the task
void vX_Task_Rack_Tick_Resume(void) {
    vTaskResume(xRackHandle);
}
