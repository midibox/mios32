// $Id$
/*
 * Routines for relative track position matrix LED display (by hawkeye)
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

#include "seq_hwcfg.h"
#include "seq_core.h"
#include "seq_ui.h"
#include "seq_bpm.h"
#include "seq_led.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// LED update routine
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_TPD_LED_Update(void)
{
  static u8 cycle_ctr = 0;
  
  if (cycle_ctr++ > 8)
     cycle_ctr = 0;

  // Power the corresponding column anode of this cycle
  MIOS32_DOUT_SRSet(seq_hwcfg_tpd.columns_sr - 1, (1 << cycle_ctr));         

  // Determine upper relative track position (tracks 1-8, "fall upward")
  s32 steps = SEQ_CC_Get(cycle_ctr, SEQ_CC_LENGTH);
  s32 curstep = seq_core_trk[cycle_ctr].step;
  u8 relpos_upper = 3 - (s32)(curstep << 2) / steps;
  
  // Determine lower relative track position (tracks 9-16, "fall downward")
  steps = SEQ_CC_Get(cycle_ctr + 8, SEQ_CC_LENGTH);
  curstep = seq_core_trk[cycle_ctr + 8].step;
  u8 relpos_lower = 4 + (s32)(curstep << 2)/ steps;
    
  // Gate the corresponding cathodes of this cycle (if the tracks are not muted)
  u8 setbyte = 0xFF;
  if (!(seq_core_trk_muted & (1 << cycle_ctr)))
     setbyte ^= (1 << relpos_upper);
  if (!(seq_core_trk_muted & (1 << (cycle_ctr + 8))))
     setbyte ^= (1 << relpos_lower);
  MIOS32_DOUT_SRSet(seq_hwcfg_tpd.rows_sr - 1, setbyte);
  
  return 0; // no error
}
