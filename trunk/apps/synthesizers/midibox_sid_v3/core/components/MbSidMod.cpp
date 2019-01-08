/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Modulation Matrix
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbSidMod.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidMod::MbSidMod()
{
    init(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidMod::~MbSidMod()
{
}


/////////////////////////////////////////////////////////////////////////////
// MOD init function
/////////////////////////////////////////////////////////////////////////////
void MbSidMod::init(sid_se_mod_patch_t *_modPatch)
{
    modPatch = _modPatch;
}


/////////////////////////////////////////////////////////////////////////////
// clears all destinations
/////////////////////////////////////////////////////////////////////////////
void MbSidMod::clearDestinations(void)
{
    s32 *modDst_clr = (s32 *)&modDst;
    for(int i=0; i<SID_SE_NUM_MOD_DST; ++i)
        *modDst_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise
}


/////////////////////////////////////////////////////////////////////////////
// Modulation Matrix Handler
/////////////////////////////////////////////////////////////////////////////
void MbSidMod::tick(void)
{
    if( !modPatch ) // exit if no patch reference initialized
        return;

    // calculate modulation pathes
    sid_se_mod_patch_t *mp = modPatch;
    for(int i=0; i<8; ++i, ++mp) {
        if( mp->depth != 128 ) {

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
                    mod_src1_value = modSrc[mp->src1-1] / 2;
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
                    mod_src2_value = modSrc[mp->src2-1] / 2;
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
                u8 old_mod_transition = modTransition;
                if( mod_src2_value < 0 )
                    modTransition &= ~(1 << i);
                else
                    modTransition |= (1 << i);

                if( modTransition != old_mod_transition && mod_src2_value >= 0 ) // only on positive transition
                    mod_result = mod_src1_value; // sample: take new mod value
                else
                    mod_result = modSrc[SID_SE_MOD_SRC_MOD1 + i]; // hold: take old mod value
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
            modSrc[SID_SE_MOD_SRC_MOD1 + i] = mod_result;

            // forward to destinations
            if( mod_result ) {
                s32 scaled_mod_result = ((s32)mp->depth-128) * mod_result / 64; // (+/- 0x7fff * +/- 0x7f) / 128
      
                // invert result if requested
                s32 mod_dst1 = (mp->op & (1 << 6)) ? -scaled_mod_result : scaled_mod_result;
                s32 mod_dst2 = (mp->op & (1 << 7)) ? -scaled_mod_result : scaled_mod_result;

                // add result to modulation target array
                u8 x_target1 = mp->x_target[0];
                if( x_target1 && x_target1 <= SID_SE_NUM_MOD_DST )
                    modDst[x_target1 - 1] += mod_dst1;
	
                u8 x_target2 = mp->x_target[1];
                if( x_target2 && x_target2 <= SID_SE_NUM_MOD_DST )
                    modDst[x_target2 - 1] += mod_dst2;

                // add to additional SIDL/R targets
                u8 direct_target_l = mp->direct_target[0];
                if( direct_target_l ) {
                    if( direct_target_l & (1 << 0) ) modDst[SID_SE_MOD_DST_PITCH1] += mod_dst1;
                    if( direct_target_l & (1 << 1) ) modDst[SID_SE_MOD_DST_PITCH2] += mod_dst1;
                    if( direct_target_l & (1 << 2) ) modDst[SID_SE_MOD_DST_PITCH3] += mod_dst1;
                    if( direct_target_l & (1 << 3) ) modDst[SID_SE_MOD_DST_PW1] += mod_dst1;
                    if( direct_target_l & (1 << 4) ) modDst[SID_SE_MOD_DST_PW2] += mod_dst1;
                    if( direct_target_l & (1 << 5) ) modDst[SID_SE_MOD_DST_PW3] += mod_dst1;
                    if( direct_target_l & (1 << 6) ) modDst[SID_SE_MOD_DST_FIL1] += mod_dst1;
                    if( direct_target_l & (1 << 7) ) modDst[SID_SE_MOD_DST_VOL1] += mod_dst1;
                }

                u8 direct_target_r = mp->direct_target[1];
                if( direct_target_r ) {
                    if( direct_target_r & (1 << 0) ) modDst[SID_SE_MOD_DST_PITCH4] += mod_dst2;
                    if( direct_target_r & (1 << 1) ) modDst[SID_SE_MOD_DST_PITCH5] += mod_dst2;
                    if( direct_target_r & (1 << 2) ) modDst[SID_SE_MOD_DST_PITCH6] += mod_dst2;
                    if( direct_target_r & (1 << 3) ) modDst[SID_SE_MOD_DST_PW4] += mod_dst2;
                    if( direct_target_r & (1 << 4) ) modDst[SID_SE_MOD_DST_PW5] += mod_dst2;
                    if( direct_target_r & (1 << 5) ) modDst[SID_SE_MOD_DST_PW6] += mod_dst2;
                    if( direct_target_r & (1 << 6) ) modDst[SID_SE_MOD_DST_FIL2] += mod_dst2;
                    if( direct_target_r & (1 << 7) ) modDst[SID_SE_MOD_DST_VOL2] += mod_dst2;
                }
            }
        }
    }
}
