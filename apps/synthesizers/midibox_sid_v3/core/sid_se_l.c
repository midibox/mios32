// $Id$
/*
 * MBSID Lead Engine
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
#include "sid_se_l.h"
#include "sid_patch.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SID_SE_L_CalcModulation(u8 sid);


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_L_Update(u8 sid)
{
  int i;
  sid_se_vars_t *vars = &sid_se_vars[sid];


  ///////////////////////////////////////////////////////////////////////////
  // Clock
  ///////////////////////////////////////////////////////////////////////////

  if( sid_se_clk.event.MIDI_START ) {
    // propagate LFO/ENV/WT sync via trigger matrix
    u8 *trg_mask_midi_start = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_MST][0];
    u8 *triggers = (u8 *)&vars->triggers.ALL[0];
    *triggers++ |= *trg_mask_midi_start++;
    *triggers++ |= *trg_mask_midi_start++;
    *triggers++ |= *trg_mask_midi_start++;
  }

  if( sid_se_clk.event.CLK ) {
    // clock propagation
    u8 *trg_mask_clk = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_CLK][0];
    u8 *triggers = (u8 *)&vars->triggers.ALL[0];
    *triggers++ |= *trg_mask_clk++;
    *triggers++ |= *trg_mask_clk++;
    *triggers++ |= *trg_mask_clk++;

    // propagate clock/4 event to trigger matrix on each 6th clock
    if( sid_se_clk.global_clk_ctr6 == 0 ) {
      u8 *trg_mask_cl6 = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_CL6][0];
      u8 *triggers = (u8 *)&vars->triggers.ALL[0];
      *triggers++ |= *trg_mask_cl6++;
      *triggers++ |= *trg_mask_cl6++;
      *triggers++ |= *trg_mask_cl6++;
    }

    // propagate clock/16 event to trigger matrix on each 24th clock
    if( sid_se_clk.global_clk_ctr24 == 0 ) {
      u8 *trg_mask_c24 = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_C24][0];
      u8 *triggers = (u8 *)&vars->triggers.ALL[0];
      *triggers++ |= *trg_mask_c24++;
      *triggers++ |= *trg_mask_c24++;
      *triggers++ |= *trg_mask_c24++;
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // LFOs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_lfo_t *l = &sid_se_lfo[sid][0];
  for(i=0; i<6; ++i, ++l)
    SID_SE_LFO(l);


  ///////////////////////////////////////////////////////////////////////////
  // ENVs
  ///////////////////////////////////////////////////////////////////////////

  sid_se_env_t *e = &sid_se_env[sid][0];
  for(i=0; i<2; ++i, ++e)
    SID_SE_ENV_Lead(e);


  ///////////////////////////////////////////////////////////////////////////
  // Modulation Matrix
  ///////////////////////////////////////////////////////////////////////////

  // since this isn't done anywhere else:
  // convert linear frequency of voice1 to 15bit signed value (only positive direction)
  vars->mod_src[SID_SE_MOD_SRC_KEY] = sid_se_voice[sid][0].linear_frq >> 1;

  // do calculations
  SID_SE_L_CalcModulation(sid);


  ///////////////////////////////////////////////////////////////////////////
  // Wavetables
  ///////////////////////////////////////////////////////////////////////////

  sid_se_wt_t *w = &sid_se_wt[sid][0];
  for(i=0; i<4; ++i, ++w)
    SID_SE_WT(w);


  ///////////////////////////////////////////////////////////////////////////
  // Arps
  ///////////////////////////////////////////////////////////////////////////
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v)
    SID_SE_Arp(v);


  ///////////////////////////////////////////////////////////////////////////
  // Syncs via Trigger Matrix
  ///////////////////////////////////////////////////////////////////////////
  sid_se_trg_t *trg = &vars->triggers;
  if( trg->ALL[0] ) {
    if( trg->O1L ) SID_SE_NoteRestart(&sid_se_voice[sid][0]);
    if( trg->O2L ) SID_SE_NoteRestart(&sid_se_voice[sid][1]);
    if( trg->O3L ) SID_SE_NoteRestart(&sid_se_voice[sid][2]);
    if( trg->O1R ) SID_SE_NoteRestart(&sid_se_voice[sid][3]);
    if( trg->O2R ) SID_SE_NoteRestart(&sid_se_voice[sid][4]);
    if( trg->O3R ) SID_SE_NoteRestart(&sid_se_voice[sid][5]);
    if( trg->E1A ) SID_SE_ENV_Restart(&sid_se_env[sid][0]);
    if( trg->E2A ) SID_SE_ENV_Restart(&sid_se_env[sid][1]);
    trg->ALL[0] = 0;
  }

  if( trg->ALL[1] ) {
    if( trg->E1R ) SID_SE_ENV_Release(&sid_se_env[sid][0]);
    if( trg->E2R ) SID_SE_ENV_Release(&sid_se_env[sid][1]);
    if( trg->L1  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][0]);
    if( trg->L2  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][1]);
    if( trg->L3  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][2]);
    if( trg->L4  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][3]);
    if( trg->L5  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][4]);
    if( trg->L6  ) SID_SE_LFO_Restart(&sid_se_lfo[sid][5]);
    trg->ALL[1] = 0;
  }

  // trg->ALL[2] already handled by WT function
  trg->ALL[2] = 0;


  ///////////////////////////////////////////////////////////////////////////
  // Voices
  ///////////////////////////////////////////////////////////////////////////
  v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    if( SID_SE_Gate(v) > 0 )
      SID_SE_Pitch(v);
    SID_SE_PW(v);
  }


  ///////////////////////////////////////////////////////////////////////////
  // Filters and Volume
  ///////////////////////////////////////////////////////////////////////////
  sid_se_filter_t *f = &sid_se_filter[sid][0];
  SID_SE_FilterAndVolume(f++);
  SID_SE_FilterAndVolume(f);


  ///////////////////////////////////////////////////////////////////////////
  // Tmp: copy register values directly into SID registers
  ///////////////////////////////////////////////////////////////////////////
  v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    sid_se_voice_waveform_t waveform = (sid_se_voice_waveform_t)v->voice_patch->waveform;
    v->phys_sid_voice->waveform = waveform.WAVEFORM;
    v->phys_sid_voice->sync = waveform.SYNC;
    v->phys_sid_voice->ringmod = waveform.RINGMOD;

    // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
    if( !v->set_delay_ctr ) {
      v->phys_sid_voice->ad = v->voice_patch->ad;
      v->phys_sid_voice->sr = v->voice_patch->sr;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Calculate Modulation Path
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_L_CalcModulation(u8 sid)
{
  int i;

  s16 *mod_src_array = sid_se_vars[sid].mod_src;
  s32 *mod_dst_array = sid_se_vars[sid].mod_dst;

  // clear all destinations
  s32 *mod_dst_array_clr = mod_dst_array;
  for(i=0; i<SID_SE_NUM_MOD_DST; ++i)
    *mod_dst_array_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise

  // calculate modulation pathes
  sid_se_mod_patch_t *mp = (sid_se_mod_patch_t *)&sid_patch[sid].L.mod[0];
  for(i=0; i<8; ++i, ++mp) {
    if( mp->depth != 0 ) {

      // first source
      s32 mod_src1_value;
      if( !mp->src1 ) {
	mod_src1_value = 0;
      } else {
	if( mp->src1 & (1 << 7) ) {
	  // constant range 0x00..0x7f -> +0x0000..0x38f0
	  mod_src1_value = (mp->src1 & 0x7f) << 7;
	} else {
	  // modulation range +/- 0x3fff
	  mod_src1_value = mod_src_array[mp->src1-1] / 2;
	}
      }

      // second source
      s32 mod_src2_value;
      if( !mp->src2 ) {
	mod_src2_value = 0;
      } else {
	if( mp->src2 & (1 << 7) ) {
	  // constant range 0x00..0x7f -> +0x0000..0x38f0
	  mod_src2_value = (mp->src2 & 0x7f) << 7;
	} else {
	  // modulation range +/- 0x3fff
	  mod_src2_value = mod_src_array[mp->src2-1] / 2;
	}
      }

      // apply operator
      s16 mod_result;
      switch( mp->op & 0x0f ) {
        case 0: // disabled
	  mod_result = 0;
	  break;

        case 1: // SRC1 only
	  mod_result = mod_src1_value;
	  break;

        case 2: // SRC2 only
	  mod_result = mod_src2_value;
	  break;

        case 3: // SRC1+SRC2
	  mod_result = mod_src1_value + mod_src2_value;
	  break;

        case 4: // SRC1-SRC2
	  mod_result = mod_src1_value - mod_src2_value;
	  break;

        case 5: // SRC1*SRC2 / 8192 (to avoid overrun)
	  mod_result = (mod_src1_value * mod_src2_value) / 8192;
	  break;

        case 6: // XOR
	  mod_result = mod_src1_value ^ mod_src2_value;
	  break;

        case 7: // OR
	  mod_result = mod_src1_value | mod_src2_value;
	  break;

        case 8: // AND
	  mod_result = mod_src1_value & mod_src2_value;
	  break;

        case 9: // Min
	  mod_result = (mod_src1_value < mod_src2_value) ? mod_src1_value : mod_src2_value;
	  break;

        case 10: // Max
	  mod_result = (mod_src1_value > mod_src2_value) ? mod_src1_value : mod_src2_value;
	  break;

        case 11: // SRC1 < SRC2
	  mod_result = (mod_src1_value < mod_src2_value) ? 0x7fff : 0x0000;
	  break;

        case 12: // SRC1 > SRC2
	  mod_result = (mod_src1_value > mod_src2_value) ? 0x7fff : 0x0000;
	  break;

        case 13: { // SRC1 == SRC2 (with tolarance of +/- 64
	  s32 diff = mod_src1_value - mod_src2_value;
	  mod_result = (diff > -64 && diff < 64) ? 0x7fff : 0x0000;
	} break;

        case 14: { // S&H - SRC1 will be sampled whenever SRC2 changes from a negative to a positive value
	  // check for SRC2 transition
	  u8 old_mod_transition = sid_se_vars[sid].mod_transition;
	  if( mod_src2_value < 0 )
	    sid_se_vars[sid].mod_transition &= ~(1 << i);
	  else
	    sid_se_vars[sid].mod_transition |= (1 << i);

	  if( sid_se_vars[sid].mod_transition != old_mod_transition && mod_src2_value >= 0 ) // only on positive transition
	    mod_result = mod_src1_value; // sample: take new mod value
	  else
	    mod_result = mod_src_array[SID_SE_MOD_SRC_MOD1 + i]; // hold: take old mod value
	} break;

        default:
	  mod_result = 0;
      }

      // store in modulator source array for feedbacks
      // use value w/o depth, this has two advantages:
      // - maximum resolution when forwarding the data value
      // - original MOD value can be taken for sample&hold feature
      // bit it also has disadvantage:
      // - the user could think it is a bug when depth doesn't affect the feedback MOD value...
      mod_src_array[SID_SE_MOD_SRC_MOD1 + i] = mod_result;

      // forward to destinations
      if( mod_result ) {
	s32 scaled_mod_result = ((s32)mp->depth-128) * mod_result / 64; // (+/- 0x7fff * +/- 0x7f) / 128
      
	// invert result if requested
	s32 mod_dst1 = (mp->op & (1 << 6)) ? -scaled_mod_result : scaled_mod_result;
	s32 mod_dst2 = (mp->op & (1 << 7)) ? -scaled_mod_result : scaled_mod_result;

	// add result to modulation target array
	u8 x_target1 = mp->x_target[0];
	if( x_target1 && x_target1 <= SID_SE_NUM_MOD_DST )
	  mod_dst_array[x_target1 - 1] += mod_dst1;
	
	u8 x_target2 = mp->x_target[1];
	if( x_target2 && x_target2 <= SID_SE_NUM_MOD_DST )
	  mod_dst_array[x_target2 - 1] += mod_dst2;

	// add to additional SIDL/R targets
	u8 direct_target_l = mp->direct_target[0];
	if( direct_target_l ) {
	  if( direct_target_l & (1 << 0) ) mod_dst_array[SID_SE_MOD_DST_PITCH1] += mod_dst1;
	  if( direct_target_l & (1 << 1) ) mod_dst_array[SID_SE_MOD_DST_PITCH2] += mod_dst1;
	  if( direct_target_l & (1 << 2) ) mod_dst_array[SID_SE_MOD_DST_PITCH3] += mod_dst1;
	  if( direct_target_l & (1 << 3) ) mod_dst_array[SID_SE_MOD_DST_PW1] += mod_dst1;
	  if( direct_target_l & (1 << 4) ) mod_dst_array[SID_SE_MOD_DST_PW2] += mod_dst1;
	  if( direct_target_l & (1 << 5) ) mod_dst_array[SID_SE_MOD_DST_PW3] += mod_dst1;
	  if( direct_target_l & (1 << 6) ) mod_dst_array[SID_SE_MOD_DST_FIL1] += mod_dst1;
	  if( direct_target_l & (1 << 7) ) mod_dst_array[SID_SE_MOD_DST_VOL1] += mod_dst1;
	}

	u8 direct_target_r = mp->direct_target[1];
	if( direct_target_r ) {
	  if( direct_target_r & (1 << 0) ) mod_dst_array[SID_SE_MOD_DST_PITCH4] += mod_dst2;
	  if( direct_target_r & (1 << 1) ) mod_dst_array[SID_SE_MOD_DST_PITCH5] += mod_dst2;
	  if( direct_target_r & (1 << 2) ) mod_dst_array[SID_SE_MOD_DST_PITCH6] += mod_dst2;
	  if( direct_target_r & (1 << 3) ) mod_dst_array[SID_SE_MOD_DST_PW4] += mod_dst2;
	  if( direct_target_r & (1 << 4) ) mod_dst_array[SID_SE_MOD_DST_PW5] += mod_dst2;
	  if( direct_target_r & (1 << 5) ) mod_dst_array[SID_SE_MOD_DST_PW6] += mod_dst2;
	  if( direct_target_r & (1 << 6) ) mod_dst_array[SID_SE_MOD_DST_FIL2] += mod_dst2;
	  if( direct_target_r & (1 << 7) ) mod_dst_array[SID_SE_MOD_DST_VOL2] += mod_dst2;
	}
      }
    }
  }

  return 0; // no error
}
