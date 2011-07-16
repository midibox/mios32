// $Id$
/*
 * Header file for tasks which have to be serviced
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
# define MUTEX_MIDIOUT_TAKE { while( xSemaphoreTakeRecursive(xMIDIOUTSemaphore, (portTickType)1) != pdTRUE ); }
# define MUTEX_MIDIOUT_GIVE { xSemaphoreGiveRecursive(xMIDIOUTSemaphore); }
#endif


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _TASKS_H */
