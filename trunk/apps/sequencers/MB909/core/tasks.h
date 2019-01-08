// $Id: tasks.h 1868 2013-11-25 20:32:00Z tk $
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
# include <FreeRTOS.h>
# include <portmacro.h>
# include <task.h>
# include <queue.h>
# include <semphr.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

// this mutex should be used by all tasks which are accessing the SD Card
#ifdef MIOS32_FAMILY_EMULATION
  extern void TASKS_SDCardSemaphoreTake(void);
  extern void TASKS_SDCardSemaphoreGive(void);
# define MUTEX_SDCARD_TAKE { TASKS_SDCardSemaphoreTake(); }
# define MUTEX_SDCARD_GIVE { TASKS_SDCardSemaphoreGive(); }
#else
  extern xSemaphoreHandle xSDCardSemaphore;
# define MUTEX_SDCARD_TAKE { while( xSemaphoreTakeRecursive(xSDCardSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_SDCARD_GIVE { xSemaphoreGiveRecursive(xSDCardSemaphore); }
#endif


// this mutex should be used by all tasks which access J16 (ENC28J60 and SD Card)
#ifdef MIOS32_FAMILY_EMULATION
# define MUTEX_J16_TAKE { }
# define MUTEX_J16_GIVE { }
#else
  extern xSemaphoreHandle xJ16Semaphore;
# define MUTEX_J16_TAKE { while( xSemaphoreTakeRecursive(xJ16Semaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_J16_GIVE { xSemaphoreGiveRecursive(xJ16Semaphore); }
#endif


// MIDI IN handler
#ifdef MIOS32_FAMILY_EMULATION
  extern void TASKS_MIDIINSemaphoreTake(void);
  extern void TASKS_MIDIINSemaphoreGive(void);
# define MUTEX_MIDIIN_TAKE { TASKS_MIDIINSemaphoreTake(); }
# define MUTEX_MIDIIN_GIVE { TASKS_MIDIINSemaphoreGive(); }
#else
  extern xSemaphoreHandle xMIDIINSemaphore;
# define MUTEX_MIDIIN_TAKE { while( xSemaphoreTakeRecursive(xMIDIINSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIIN_GIVE { xSemaphoreGiveRecursive(xMIDIINSemaphore); }
#endif


// MIDI OUT handler
#ifdef MIOS32_FAMILY_EMULATION
  extern void TASKS_MIDIOUTSemaphoreTake(void);
  extern void TASKS_MIDIOUTSemaphoreGive(void);
# define MUTEX_MIDIOUT_TAKE { TASKS_MIDIOUTSemaphoreTake(); }
# define MUTEX_MIDIOUT_GIVE { TASKS_MIDIOUTSemaphoreGive(); }
#else
  extern xSemaphoreHandle xMIDIOUTSemaphore;
# define MUTEX_MIDIOUT_TAKE { if( xMIDIOUTSemaphore ) while( xSemaphoreTakeRecursive(xMIDIOUTSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIOUT_GIVE { if( xMIDIOUTSemaphore ) xSemaphoreGiveRecursive(xMIDIOUTSemaphore); }
#endif


// LCD access
#ifdef MIOS32_FAMILY_EMULATION
  extern void TASKS_LCDSemaphoreTake(void);
  extern void TASKS_LCDSemaphoreGive(void);
# define MUTEX_LCD_TAKE { TASKS_LCDSemaphoreTake(); }
# define MUTEX_LCD_GIVE { TASKS_LCDSemaphoreGive(); }
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
extern void SEQ_TASK_Period1mS_LowPrio(void);
extern void SEQ_TASK_Period1S(void);
extern void SEQ_TASK_Pattern(void);

// located in tasks.c
extern void SEQ_TASK_PatternResume(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _TASKS_H */
