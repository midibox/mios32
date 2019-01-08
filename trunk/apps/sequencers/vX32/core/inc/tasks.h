/* $Id$ */
/*
 * FreeRTOS Tasks
 * Header file for tasks which have to be serviced by FreeRTOS/x86
 *
 * For MIOS32, the appr. tasks.c file is located in ../mios32/tasks.c
 * For x86, the code is implemented in tasks.cpp
 *
 */

#ifndef _TASKS_H
#define _TASKS_H


#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef ENDLESSLOOP
#define ENDLESSLOOP 1
#endif

# define MUTEX_GRAPH_TAKE xSemaphoreTake(xGraphSemaphore, (portTickType)1)
# define MUTEX_GRAPH_GIVE xSemaphoreGive(xGraphSemaphore)

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


extern xSemaphoreHandle xGraphSemaphore;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// called from tasks.c
extern s32 TASKS_Init(u32 mode);                                                // add the tasks to FreeRTOS

extern void vX_Task_Rack_Tick(void);                                            // task handles the vX rack
extern void vX_Task_Rack_Tick_Resume(void);                                     // resume the above task if it is stopped
extern void vX_Task_MIDI(void);                                                 // task handles MIDI I/O
extern void vX_Task_MIDI_Resume(void);                                          // resume the above task if it is stopped
extern void vX_Task_UI(void);                                                   // task handles user interfacing
extern void vX_Task_UI_Resume(void);                                            // resume the above task if it is stopped


#endif /* _TASKS_H */
