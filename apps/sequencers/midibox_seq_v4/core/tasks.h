// $Id$
/*
 * Header file for tasks which have to be serviced by FreeRTOS/MacOS
 *
 * For MIOS32, the appr. tasks.c file is located in ../mios32/tasks.c
 * For MacOS, the code is implemented in ui.m
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _TASKS_H
#define _TASKS_H


#ifndef MIOS32_FAMILY_EMULATION
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// this mutex should be used by all tasks which are accessing the SD Card
#ifdef MIOS32_FAMILY_EMULATION
# define MUTEX_SDCARD_TAKE { }
# define MUTEX_SDCARD_GIVE { }
#else
  extern xSemaphoreHandle xSDCardSemaphore;
# define MUTEX_SDCARD_TAKE { while( xSemaphoreTakeRecursive(xSDCardSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_SDCARD_GIVE { xSemaphoreGiveRecursive(xSDCardSemaphore); }
#endif


// MIDI IN handler
#ifdef MIOS32_FAMILY_EMULATION
# define MUTEX_MIDIIN_TAKE { }
# define MUTEX_MIDIIN_GIVE { }
#else
  extern xSemaphoreHandle xMIDIINSemaphore;
# define MUTEX_MIDIIN_TAKE { while( xSemaphoreTakeRecursive(xMIDIINSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIIN_GIVE { xSemaphoreGiveRecursive(xMIDIINSemaphore); }
#endif


// MIDI OUT handler
#ifdef MIOS32_FAMILY_EMULATION
# define MUTEX_MIDIOUT_TAKE { }
# define MUTEX_MIDIOUT_GIVE { }
#else
  extern xSemaphoreHandle xMIDIOUTSemaphore;
# define MUTEX_MIDIOUT_TAKE { while( xSemaphoreTakeRecursive(xMIDIOUTSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIOUT_GIVE { xSemaphoreGiveRecursive(xMIDIOUTSemaphore); }
#endif


// LCD access
#ifdef MIOS32_FAMILY_EMULATION
# define MUTEX_LCD_TAKE { }
# define MUTEX_LCD_GIVE { }
#else
  extern xSemaphoreHandle xLCDSemaphore;
# define MUTEX_LCD_TAKE { while( xSemaphoreTakeRecursive(xLCDSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_LCD_GIVE { xSemaphoreGiveRecursive(xLCDSemaphore); }
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// called from tasks.c
extern s32 TASKS_Init(u32 mode);

extern void SEQ_TASK_MIDI(void);
extern void SEQ_TASK_Period1mS(void);
extern void SEQ_TASK_Period1S(void);
extern void SEQ_TASK_Pattern(void);

// located in tasks.c
extern void SEQ_TASK_PatternResume(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _TASKS_H */
