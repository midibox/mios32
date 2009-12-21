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
  s32 scaled_value = knob->min + (value * factor)/256;

  // copy as signed value into modulation source array
  sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_KNOB1 + knob_num] = (value << 8) - 0x8000;

  if( knob_num == SID_KNOB_1 ) {
    // copy knob1 to MDW source
    // in distance to KNOB1, this one goes only into positive direction
    sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_MDW] = value << 7;
  } else if( knob_num == SID_KNOB_PITCHBENDER ) {
    // TMP solution for pitchbender
    sid_se_voice_t *v = &sid_se_voice[sid][0];
    int voice;
    for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v)
      v->pitchbender = value;
  }

  // TODO: forward to parameter handler

  return 0; // no error
}
