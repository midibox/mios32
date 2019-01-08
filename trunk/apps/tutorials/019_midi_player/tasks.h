// $Id: tasks.h 1247 2011-07-10 21:29:37Z tk $
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

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _TASKS_H */
