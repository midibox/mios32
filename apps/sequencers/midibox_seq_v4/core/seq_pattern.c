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
#include "seq_core.h"
#include "seq_ui.h"
#include "seq_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// set this to 1 if performance of pattern handler should be measured with a scope
// (LED toggling in APP_Background() has to be disabled!)
#define LED_PERFORMANCE_MEASURING 1


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_PATTERN_Load(u8 group, seq_pattern_t pattern);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_pattern_t seq_pattern[SEQ_CORE_NUM_GROUPS];


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
    seq_pattern[group].group = 2*group; // A/C/E/G
    seq_pattern_req[group].ALL = 0;
  }

  return 0; // no error
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
    MIOS32_IRQ_Disable();
    pattern.REQ = 1;
    seq_pattern_req[group] = pattern;
    MIOS32_IRQ_Enable();

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
      MIOS32_IRQ_Disable();
      seq_pattern_req[group].REQ = 0;
      MIOS32_IRQ_Enable();

      SEQ_PATTERN_Load(group, seq_pattern_req[group]);
    }
  }

#if LED_PERFORMANCE_MEASURING
  MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Load a pattern from BankStick/SD Card
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PATTERN_Load(u8 group, seq_pattern_t pattern)
{
  s32 status;

  seq_pattern[group] = pattern;

  status = SEQ_FILE_B_PatternRead(pattern.bank, pattern.pattern, group);

  return status;
}

