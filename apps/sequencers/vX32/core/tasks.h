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


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// called from tasks.c
extern s32 tasks_init(u32 mode);												// add the tasks to FreeRTOS

extern void vx_task_rack_tick(void);											// task handles the vX rack
extern void vx_task_rack_resume(void);											// resume the above task if it is stopped
extern void vx_task_midi(void);													// task handles MIDI I/O
extern void vx_task_midi_resume(void);											// resume the above task if it is stopped
extern void vx_task_ui(void);													// task handles user interfacing
extern void vx_task_ui_resume(void);											// resume the above task if it is stopped

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TASKS_H */
