// $Id$
/*
 * Header file for FreeRTOS Utility Functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _FREERTOS_UTILS_H
#define _FREERTOS_UTILS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// which MIOS32 timer should be used for performance measurements (0..2)
#ifndef FREERTOS_UTILS_PERF_TIMER
#define FREERTOS_UTILS_PERF_TIMER 2
#endif

// at which interval should it be called (interval should be much less than 1 mS!)
#ifndef FREERTOS_UTILS_PERF_TIMER_PERIOD
#define FREERTOS_UTILS_PERF_TIMER_PERIOD 10 // uS
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 FREERTOS_UTILS_PerfCounterInit(void);
extern u32 FREERTOS_UTILS_PerfCounterGet(void);
extern s32 FREERTOS_UTILS_RunTimeStats(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _FREERTOS_UTILS_H */
