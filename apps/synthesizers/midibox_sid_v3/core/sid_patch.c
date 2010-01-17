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

#include <sid.h>

#include "sid_se.h"
#include "sid_patch.h"
#include "sid_voice.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

sid_patch_t sid_patch[SID_PATCH_NUM];
sid_patch_t sid_patch_shadow[SID_PATCH_NUM];

sid_patch_ref_t sid_patch_ref[SID_PATCH_NUM];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#include "sid_patch_preset_lead.inc"
#include "sid_patch_preset_bassline.inc"
#include "sid_patch_preset_drum.inc"
#include "sid_patch_preset_multi.inc"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_PATCH_Init(u32 mode)
{
  int i;

  for(i=0; i<SID_PATCH_NUM; ++i) {
    SID_PATCH_Preset((sid_patch_t *)&sid_patch[i], SID_SE_LEAD);
    SID_PATCH_Preset((sid_patch_t *)&sid_patch_shadow[i], SID_SE_LEAD);

    sid_patch_ref[i].sid = i;
    sid_patch_ref[i].bank = 0;
    sid_patch_ref[i].patch = 0;
    sid_patch_ref[i].p = &sid_patch[i];
  }

  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Has to be called whenever a patch has changed
/////////////////////////////////////////////////////////////////////////////
s32 SID_PATCH_Changed(u8 sid)
{
  // disable interrupts to ensure atomic change
  MIOS32_IRQ_Disable();

  // remember previous engine and get new engine
  sid_se_engine_t prev_engine = sid_patch_shadow[sid].engine;
  sid_se_engine_t engine = sid_patch[sid].engine;

  // copy patch into shadow buffer
  memcpy((u8 *)&sid_patch_shadow[sid].ALL[0], (u8 *)&sid_patch[sid].ALL[0], 512);

  // re-initialize structures if engine has changed
  if( prev_engine != engine )
    SID_SE_InitStructs(sid);

  // clear voice queue
  SID_VOICE_QueueInit(sid);

  // enable interrupts again
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Preloads a patch
/////////////////////////////////////////////////////////////////////////////
s32 SID_PATCH_Preset(sid_patch_t *patch, sid_se_engine_t engine)
{
  switch( engine ) {
    case SID_SE_LEAD:
      if( sizeof(sid_patch_t) != sizeof(sid_patch_preset_lead) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SID_PATCH_Preset:ERROR] lead patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(sid_patch_preset_lead));
#endif
	return -2; // unexpected error: structure doesn't fit
      }
      memcpy(patch, sid_patch_preset_lead, sizeof(sid_patch_preset_lead));
#if 0
      // this code helps to check if the patch structure is aligned correctly
      DEBUG_MSG("chk1: %08x\n", (u8 *)&patch->L.v_flags - (u8 *)&patch->engine);
      DEBUG_MSG("chk2: %08x\n", (u8 *)&patch->L.osc_detune - (u8 *)&patch->engine);
      DEBUG_MSG("chk3: %08x\n", (u8 *)&patch->L.filter[0] - (u8 *)&patch->engine);
      DEBUG_MSG("chk4: %08x\n", (u8 *)&patch->L.voice[0] - (u8 *)&patch->engine);
      DEBUG_MSG("chk5: %08x\n", (u8 *)&patch->L.lfo[0] - (u8 *)&patch->engine);
      DEBUG_MSG("chk6: %08x\n", (u8 *)&patch->L.env[0] - (u8 *)&patch->engine);
      DEBUG_MSG("chk7: %08x\n", (u8 *)&patch->L.mod[0] - (u8 *)&patch->engine);
      DEBUG_MSG("chk8: %08x\n", (u8 *)&patch->L.wt_memory[127] - (u8 *)&patch->engine);

      DEBUG_MSG("sz1: %08x\n", sizeof(sid_se_filter_patch_t));
      DEBUG_MSG("sz2: %08x\n", sizeof(sid_se_voice_patch_t));
      DEBUG_MSG("sz3: %08x\n", sizeof(sid_se_lfo_patch_t));
      DEBUG_MSG("sz4: %08x\n", sizeof(sid_se_env_patch_t));
      DEBUG_MSG("sz5: %08x\n", sizeof(sid_se_mod_patch_t));
      DEBUG_MSG("sz6: %08x\n", sizeof(sid_se_trg_t));
      DEBUG_MSG("sz7: %08x\n", sizeof(sid_se_wt_patch_t));
#endif
      break;

    case SID_SE_BASSLINE:
      if( sizeof(sid_patch_t) != sizeof(sid_patch_preset_bassline) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SID_PATCH_Preset:ERROR] bassline patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(sid_patch_preset_bassline));
#endif
	return -2; // unexpected error: structure doesn't fit
      }
      memcpy(patch, sid_patch_preset_bassline, sizeof(sid_patch_preset_bassline));
      break;

    case SID_SE_DRUM:
      if( sizeof(sid_patch_t) != sizeof(sid_patch_preset_drum) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SID_PATCH_Preset:ERROR] drum patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(sid_patch_preset_drum));
#endif
	return -2; // unexpected error: structure doesn't fit
      }
      memcpy(patch, sid_patch_preset_drum, sizeof(sid_patch_preset_drum));
      break;

    case SID_SE_MULTI:
      if( sizeof(sid_patch_t) != sizeof(sid_patch_preset_multi) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SID_PATCH_Preset:ERROR] multi patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(sid_patch_preset_multi));
#endif
	return -2; // unexpected error: structure doesn't fit
      }
      memcpy(patch, sid_patch_preset_multi, sizeof(sid_patch_preset_multi));
      break;

    default:
      return -1; // invalid engine
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the patch name as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
s32 SID_PATCH_NameGet(sid_patch_t *patch, char *buffer)
{
  int i;

  for(i=0; i<16; ++i)
    buffer[i] = patch->name[i] >= 0x20 ? patch->name[i] : ' ';
  buffer[i] = 0;

  return 0; // no error
}
