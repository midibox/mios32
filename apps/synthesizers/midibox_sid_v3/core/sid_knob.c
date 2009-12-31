// $Id$
/*
 * MBSID Patch Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "sid_patch.h"
#include "sid_se.h"
#include "sid_knob.h"
#include "sid_par.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_KNOB_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
s32 SID_KNOB_SetValue(u8 sid, sid_knob_num_t knob_num, u8 value)
{
  if( knob_num >= 8 )
    return -1; // invalid knob number

  // store new value into patch
  sid_knob_t *knob = (sid_knob_t *)&sid_patch[sid].knob[knob_num];
  knob->value = value;

  // copy it also into shadow buffer
  sid_knob_t *knob_shadow = (sid_knob_t *)&sid_patch_shadow[sid].knob[knob_num];
  knob_shadow->value = value;

  // get scaled value between min/max boundaries
  s32 factor = ((knob->max > knob->min) ? (knob->max - knob->min) : (knob->min - knob->max)) + 1;
  s32 scaled_value16 = knob->min + (value * factor); // 16bit

  // copy as signed value into modulation source array
  sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_KNOB1 + knob_num] = (value << 8) - 0x8000;

  if( knob_num == SID_KNOB_1 ) {
    // copy knob1 to MDW source
    // in distance to KNOB1, this one goes only into positive direction
    sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_MDW] = value << 7;
  }

  // determine sidl/r and instrument selection depending on engine
  u8 sidlr;
  u8 ins;
  sid_se_engine_t engine = sid_patch[sid].engine;
  switch( engine ) {
    case SID_SE_LEAD:
      sidlr = 3; // always modify both SIDs
      ins = 0; // instrument n/a
      break;

    case SID_SE_BASSLINE:
      sidlr = 3; // always modify both SIDs
      ins = 3; // TODO: for "current" use this selection! - value has to be taken from CS later
      break;

    case SID_SE_DRUM:
      sidlr = 3; // always modify both SIDs
      ins = 0; // TODO: for "current" use this selection! - value has to be taken from CS later
      break;

    case SID_SE_MULTI:
      sidlr = 3; // always modify both SIDs
      ins = 0; // TODO: for "current" use this selection! - value has to be taken from CS later
      break;

    default:
      // unsupported engine, use default values
      sidlr = 3;
      ins = 0;
  }

  // forward to parameter handler
  // interrupts should be disabled to ensure atomic access
  MIOS32_IRQ_Disable();
  SID_PAR_Set16(sid, knob->assign1, scaled_value16, sidlr, ins);
  SID_PAR_Set16(sid, knob->assign2, scaled_value16, sidlr, ins);
  MIOS32_IRQ_Enable();

  return 0; // no error
}
