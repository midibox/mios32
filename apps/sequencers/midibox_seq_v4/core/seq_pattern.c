// $Id$
/*
 * Pattern Routines
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

#include "tasks.h"

#include "seq_pattern.h"
#include "seq_cc.h"
#include "seq_core.h"
#include "seq_ui.h"
#include "seq_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// set this to 1 if performance of pattern handler should be measured with a scope
// (LED toggling in APP_Background() has to be disabled!)
#define LED_PERFORMANCE_MEASURING 0

// same for measuring with the stopwatch
// value is visible in menu (-> press exit button)
#define STOPWATCH_PERFORMANCE_MEASURING 0


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_pattern_t seq_pattern[SEQ_CORE_NUM_GROUPS];
char seq_pattern_name[SEQ_CORE_NUM_GROUPS][21];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_pattern_t seq_pattern_req[SEQ_CORE_NUM_GROUPS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Init(u32 mode)
{
  // pre-init pattern numbers
  u8 group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    seq_pattern[group].ALL = 0;
#if 0
    seq_pattern[group].group = 2*group; // A/C/E/G
#else
    seq_pattern[group].bank = group; // each group has it's own bank
#endif
    seq_pattern_req[group].ALL = 0;

#if 0
    sprintf((char *)seq_pattern_name[group], "Pattern %c%d          ", ('A'+((pattern>>3)&7)), (pattern&7)+1);
#else
    // if pattern name only contains spaces, the UI will print 
    // the pattern number instead of an empty message
    // this ensures highest flexibility (e.g. any pattern can be copied to another slot w/o name inconsistencies)
    // -> see SEQ_LCD_PrintPatternName()
    int i;
    for(i=0; i<20; ++i)
      seq_pattern_name[group][i] = ' ';
    seq_pattern_name[group][20] = 0;
#endif

  }

#if STOPWATCH_PERFORMANCE_MEASURING
  SEQ_UI_MENU_StopwatchInit();
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of a pattern (20 characters)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_PATTERN_NameGet(u8 group)
{
  if( group >= SEQ_CORE_NUM_GROUPS )
    return "<invalid group>     ";
  return seq_pattern_name[group];
}


/////////////////////////////////////////////////////////////////////////////
// Requests a pattern change
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Change(u8 group, seq_pattern_t pattern)
{
  if( group >= SEQ_CORE_NUM_GROUPS )
    return -1; // invalid group

  // change immediately if sequencer not running
  if( !SEQ_BPM_IsRunning() || ui_seq_pause ) {
#if LED_PERFORMANCE_MEASURING
    MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif
    SEQ_PATTERN_Load(group, pattern);
#if LED_PERFORMANCE_MEASURING
    MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif
  } else {

    // TODO: stall here if previous pattern change hasn't been finished yet!

    // else request change
    portENTER_CRITICAL();
    pattern.REQ = 1;
    seq_pattern_req[group] = pattern;
    portEXIT_CRITICAL();

    // pregenerate bpm ticks
    SEQ_CORE_AddForwardDelay(50); // mS

    // resume low-prio pattern handler
    SEQ_TASK_PatternResume();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from a separate task to handle pattern
// change requests
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Handler(void)
{
  u8 group;

#if LED_PERFORMANCE_MEASURING
  MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif

  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
    if( seq_pattern_req[group].REQ ) {
      portENTER_CRITICAL();
      seq_pattern_req[group].REQ = 0;
      portEXIT_CRITICAL();

      SEQ_PATTERN_Load(group, seq_pattern_req[group]);
    }
  }

#if LED_PERFORMANCE_MEASURING
  MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Load a pattern from SD Card
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Load(u8 group, seq_pattern_t pattern)
{
  s32 status;

  seq_pattern[group] = pattern;

  MUTEX_SDCARD_TAKE;

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  SEQ_UI_MENU_StopwatchReset();
#endif

  if( (status=SEQ_FILE_B_PatternRead(pattern.bank, pattern.pattern, group)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  SEQ_UI_MENU_StopwatchCapture();
#endif

  MUTEX_SDCARD_GIVE;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// Stores a pattern into SD Card
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_PATTERN_Save(u8 group, seq_pattern_t pattern)
{
  s32 status;

  MUTEX_SDCARD_TAKE;

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  SEQ_UI_MENU_StopwatchReset();
#endif

  if( (status=SEQ_FILE_B_PatternWrite(pattern.bank, pattern.pattern, group)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  SEQ_UI_MENU_StopwatchCapture();
#endif

  MUTEX_SDCARD_GIVE;

  return status;
}

