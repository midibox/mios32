/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Modulation Matrix
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
#include "app.h"
#include "MbCvMod.h"
#include "MbCvEnvironment.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvMod::MbCvMod()
{
    init(0);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvMod::~MbCvMod()
{
}


/////////////////////////////////////////////////////////////////////////////
// MOD init function
/////////////////////////////////////////////////////////////////////////////
void MbCvMod::init(u8 _modNum)
{
    ModPatchT *mp = modPatch;
    for(int i=0; i<MBCV_NUM_MOD; ++i, ++mp) {
        mp->src1 = 0;
        mp->src1_chn = _modNum;
        mp->src2 = 0;
        mp->src2_chn = _modNum;
        mp->op = 0;
        mp->depth = 64;
        mp->dst1 = 0;
        mp->dst2 = 0;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Modulation Matrix Handler
/////////////////////////////////////////////////////////////////////////////
void MbCvMod::tick(void)
{
    // dirty... we handle MbCvEnvironment like a singleton
    MbCvEnvironment* env = APP_GetEnv();
    if( !env )
        return;

    // calculate modulation pathes
    ModPatchT *mp = modPatch;
    for(int i=0; i<MBCV_NUM_MOD; ++i, ++mp) {
        if( mp->depth != 0 ) {

            // first source
            s32 mod_src1_value;
            if( !mp->src1 || mp->src2_chn >= CV_SE_NUM ) {
                mod_src1_value = 0;
            } else {
                if( mp->src1 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src1_value = (mp->src1 & 0x7f) << 7;
                } else {
                    // modulation range +/- 0x3fff
                    mod_src1_value = env->mbCv[mp->src1_chn].mbCvMod.modSrc[mp->src1-1] / 2;
                }
            }

            // second source
            s32 mod_src2_value;
            if( !mp->src2 || mp->src2_chn >= CV_SE_NUM ) {
                mod_src2_value = 0;
            } else {
                if( mp->src2 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src2_value = (mp->src2 & 0x7f) << 7;
                } else {
                    // modulation range +/- 0x3fff
                    mod_src2_value = env->mbCv[mp->src2_chn].mbCvMod.modSrc[mp->src2-1] / 2;
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
                    mod_result = modSrc[MBCV_MOD_SRC_MOD1 + i]; // hold: take old mod value
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
            modSrc[MBCV_MOD_SRC_MOD1 + i] = mod_result;

            // forward to destinations
            if( mod_result ) {
                s32 scaled_mod_result = (s32)mp->depth * mod_result / 64; // (+/- 0x7fff * +/- 0x7f) / 128
      
                // invert result if requested
                s32 mod_dst1 = (mp->op & (1 << 6)) ? -scaled_mod_result : scaled_mod_result;
                s32 mod_dst2 = (mp->op & (1 << 7)) ? -scaled_mod_result : scaled_mod_result;

                // add result to modulation target array
                u8 dst1 = mp->dst1;
                if( dst1 && dst1 <= MBCV_NUM_MOD_DST )
                    modDst[dst1 - 1] += mod_dst1;
	
                u8 dst2 = mp->dst2;
                if( dst2 && dst2 <= MBCV_NUM_MOD_DST )
                    modDst[dst2 - 1] += mod_dst2;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Takes and clears destination value
/////////////////////////////////////////////////////////////////////////////
s32 MbCvMod::takeDstValue(const u8& ix)
{
    if( ix >= MBCV_NUM_MOD_DST )
        return 0;

    s32 *dst = (s32 *)&modDst[ix];
    s32 ret = *dst;
    *dst = 0;
    return ret;
}

