// $Id$
/*
 * MBSEQ Statistics functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "seq_statistics.h"

#include "tasks.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static s32 cpu_load_in_percent;

static u32 stopwatch_value;
static u32 stopwatch_value_max;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_Init(u32 mode)
{
  return SEQ_STATISTICS_Reset();
}


/////////////////////////////////////////////////////////////////////////////
// Resets the statistic values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_Reset(void)
{
  cpu_load_in_percent = 0;
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  return 0; // no error;
}


/////////////////////////////////////////////////////////////////////////////
// Handles Idle Counter (frequently called from Background task)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_Idle(void)
{
  static u32 idle_ctr = 0;
  static u32 last_seconds = 0;

  // determine the CPU load
  ++idle_ctr;
  mios32_sys_time_t t = MIOS32_SYS_TimeGet();
  if( t.seconds != last_seconds ) {
    last_seconds = t.seconds;

    // MAX_IDLE_CTR defined in mios32_config.h
    // CPU Load is printed in main menu screen
    cpu_load_in_percent = 100 - ((100 * idle_ctr) / MAX_IDLE_CTR);

#if 0
    DEBUG_MSG("Load: %d%% (Ctr: %d)\n", cpu_load_in_percent, idle_ctr);
#endif
    
    idle_ctr = 0;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns current CPU load in percent
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_STATISTICS_CurrentCPULoad(void)
{
  return cpu_load_in_percent;
}



/////////////////////////////////////////////////////////////////////////////
// Optional stopwatch for measuring performance
// Will be displayed on menu page once stopwatch_max > 0
// Value can be reseted by pressing GP button below the max number
// Usage example: see seq_core.c
// Only one task should control the stopwatch!
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  MIOS32_STOPWATCH_Init(1); // 1 uS resolution

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets stopwatch
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_StopwatchReset(void)
{
  return MIOS32_STOPWATCH_Reset();
}


/////////////////////////////////////////////////////////////////////////////
// Captures value of stopwatch and displays on screen
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_STATISTICS_StopwatchCapture(void)
{
  u32 value = MIOS32_STOPWATCH_ValueGet();
  portENTER_CRITICAL();
  stopwatch_value = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  portEXIT_CRITICAL();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns current value
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_STATISTICS_StopwatchGetValue(void)
{
  return stopwatch_value;
}

/////////////////////////////////////////////////////////////////////////////
// Returns current max value
/////////////////////////////////////////////////////////////////////////////
extern u32 SEQ_STATISTICS_StopwatchGetValueMax(void)
{
  return stopwatch_value_max;
}

