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
        mp->offset = 0;
        mp->dst1 = 0;
        mp->dst2 = 0;

        modOut[i] = 0;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Modulation Sources
/////////////////////////////////////////////////////////////////////////////

#define CREATE_SRC_FUNCTION(name, str, getCode) \
    static const char name##SrcString[] = str; \
    static s16 getSrc##name(MbCvEnvironment* env, u8 cv) { getCode; }

typedef struct {
    const char *nameString;
    s16 (*getFunct)(MbCvEnvironment *env, u8 cv);
} MbCvModSrcTableEntry_t;

#define SRC_TABLE_ITEM(name) \
    { name##SrcString, getSrc##name }

CREATE_SRC_FUNCTION(None,    "--- ", return 0);
CREATE_SRC_FUNCTION(Env1,    "ENV1", return env->mbCv[cv].mbCvEnv1[0].envOut);
CREATE_SRC_FUNCTION(Env2,    "ENV2", return env->mbCv[cv].mbCvEnv2[0].envOut);
CREATE_SRC_FUNCTION(Lfo1,    "LFO1", return env->mbCv[cv].mbCvLfo[0].lfoOut);
CREATE_SRC_FUNCTION(Lfo2,    "LFO2", return env->mbCv[cv].mbCvLfo[1].lfoOut);
CREATE_SRC_FUNCTION(Mod1,    "MOD1", return env->mbCv[cv].mbCvMod.modOut[0]);
CREATE_SRC_FUNCTION(Mod2,    "MOD2", return env->mbCv[cv].mbCvMod.modOut[1]);
CREATE_SRC_FUNCTION(Mod3,    "MOD3", return env->mbCv[cv].mbCvMod.modOut[2]);
CREATE_SRC_FUNCTION(Mod4,    "MOD4", return env->mbCv[cv].mbCvMod.modOut[3]);
CREATE_SRC_FUNCTION(Key,     "Key ", return env->mbCv[cv].mbCvVoice.voiceLinearFrq >> 1);
CREATE_SRC_FUNCTION(Vel,     "Vel ", return env->mbCv[cv].mbCvVoice.voiceVelocity << 8);
CREATE_SRC_FUNCTION(MdW,     "MdW ", return env->mbCv[cv].mbCvMidiVoice.midivoiceModWheel << 8);
CREATE_SRC_FUNCTION(PBn,     "PBn ", return env->mbCv[cv].mbCvMidiVoice.midivoicePitchBender * 2);
CREATE_SRC_FUNCTION(Aft,     "Aft ", return env->mbCv[cv].mbCvMidiVoice.midivoiceAftertouch << 8);
CREATE_SRC_FUNCTION(Knb1,    "Knb1", return env->knobValue[0] << 7);
CREATE_SRC_FUNCTION(Knb2,    "Knb2", return env->knobValue[1] << 7);
CREATE_SRC_FUNCTION(Knb3,    "Knb3", return env->knobValue[2] << 7);
CREATE_SRC_FUNCTION(Knb4,    "Knb4", return env->knobValue[3] << 7);
CREATE_SRC_FUNCTION(Knb5,    "Knb5", return env->knobValue[4] << 7);
CREATE_SRC_FUNCTION(Knb6,    "Knb6", return env->knobValue[5] << 7);
CREATE_SRC_FUNCTION(Knb7,    "Knb7", return env->knobValue[6] << 7);
CREATE_SRC_FUNCTION(Knb8,    "Knb8", return env->knobValue[7] << 7);
CREATE_SRC_FUNCTION(Ain1,    "AIN1", return MIOS32_AIN_PinGet(0) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain2,    "AIN2", return MIOS32_AIN_PinGet(1) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain3,    "AIN3", return MIOS32_AIN_PinGet(2) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain4,    "AIN4", return MIOS32_AIN_PinGet(3) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain5,    "AIN5", return MIOS32_AIN_PinGet(4) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain6,    "AIN6", return MIOS32_AIN_PinGet(5) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain7,    "AIN7", return MIOS32_AIN_PinGet(6) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(Ain8,    "AIN8", return MIOS32_AIN_PinGet(7) << 3); // 12bit -> 15bit
CREATE_SRC_FUNCTION(SeqEnvM, "EnvM", return env->mbCv[cv].mbCvSeqBassline.seqEnvMod << 7);
CREATE_SRC_FUNCTION(SeqAcc,  "Acc.", return (env->mbCv[cv].mbCvArp.arpEnabled ? env->mbCv[cv].mbCvSeqBassline.seqAccent : env->mbCv[cv].mbCvSeqBassline.seqAccentEffective) << 7);

static const MbCvModSrcTableEntry_t mbCvModSrcTable[MBCV_NUM_MOD_SRC] = {
    SRC_TABLE_ITEM(None),
    SRC_TABLE_ITEM(Env1),
    SRC_TABLE_ITEM(Env2),
    SRC_TABLE_ITEM(Lfo1),
    SRC_TABLE_ITEM(Lfo2),
    SRC_TABLE_ITEM(Mod1),
    SRC_TABLE_ITEM(Mod2),
    SRC_TABLE_ITEM(Mod3),
    SRC_TABLE_ITEM(Mod4),
    SRC_TABLE_ITEM(Key),
    SRC_TABLE_ITEM(Vel),
    SRC_TABLE_ITEM(MdW),
    SRC_TABLE_ITEM(PBn),
    SRC_TABLE_ITEM(Aft),
    SRC_TABLE_ITEM(Knb1),
    SRC_TABLE_ITEM(Knb2),
    SRC_TABLE_ITEM(Knb3),
    SRC_TABLE_ITEM(Knb4),
    SRC_TABLE_ITEM(Knb5),
    SRC_TABLE_ITEM(Knb6),
    SRC_TABLE_ITEM(Knb7),
    SRC_TABLE_ITEM(Knb8),
    SRC_TABLE_ITEM(Ain1),
    SRC_TABLE_ITEM(Ain2),
    SRC_TABLE_ITEM(Ain3),
    SRC_TABLE_ITEM(Ain4),
    SRC_TABLE_ITEM(Ain5),
    SRC_TABLE_ITEM(Ain6),
    SRC_TABLE_ITEM(Ain7),
    SRC_TABLE_ITEM(Ain8),
    SRC_TABLE_ITEM(SeqEnvM),
    SRC_TABLE_ITEM(SeqAcc),
};


/////////////////////////////////////////////////////////////////////////////
// Modulation Operations
/////////////////////////////////////////////////////////////////////////////

#define CREATE_OP_FUNCTION(name, str, modifyCode) \
    static const char name##OpString[] = str; \
    static s16 modifyOp##name(MbCvEnvironment *env, MbCvMod* mod, u8 num, s16 src1, s16 src2) { modifyCode; }

typedef struct {
    const char *nameString;
    s16 (*modifyFunct)(MbCvEnvironment *env, MbCvMod *mod, u8 num, s16 src1, s16 src2);
} MbCvModOpTableEntry_t;

#define OP_TABLE_ITEM(name) \
    { name##OpString, modifyOp##name }

CREATE_OP_FUNCTION(None,     "--- ", return 0);
CREATE_OP_FUNCTION(Src1Only, "Src1", return src1);
CREATE_OP_FUNCTION(Src2Only, "Src2", return src2);
CREATE_OP_FUNCTION(Plus,     "1+2 ", return src1 + src2);
CREATE_OP_FUNCTION(Minus,    "1-2 ", return src1 - src2);
CREATE_OP_FUNCTION(Multiply, "1*2 ", return (src1 * src2) / 8192); // / 8192 to avoid overrun
CREATE_OP_FUNCTION(Xor,      "XOR ", return src1 ^ src2);
CREATE_OP_FUNCTION(Or,       "OR  ", return src1 | src2);
CREATE_OP_FUNCTION(And,      "AND ", return src1 & src2);
CREATE_OP_FUNCTION(Min,      "MIN ", return (src1 < src2) ? src1 : src2);
CREATE_OP_FUNCTION(Max,      "MAX ", return (src1 > src2) ? src1 : src2);
CREATE_OP_FUNCTION(Lt,       "1<2 ", return (src1 < src2) ? 0x7fff : 0x0000);
CREATE_OP_FUNCTION(Gt,       "1>2 ", return (src1 > src2) ? 0x7fff : 0x0000);
CREATE_OP_FUNCTION(Eq,       "1=2 ", s32 diff = src1 - src2; return (diff > -64 && diff < 64) ? 0x7fff : 0x0000);
CREATE_OP_FUNCTION(SandH,    "S&H ", u8 old_mod_transition = mod->modTransition; if( src2 < 0 ) { mod->modTransition &= ~(1 << num); } else { mod->modTransition |= (1 << num); } return (mod->modTransition != old_mod_transition && src2 >= 0) ? src1 : mod->modOut[num]);
CREATE_OP_FUNCTION(Fts,      "FTS ", s32 sum = src1 + src2; if( sum >= 0 ) { return env->scaleValue(sum / 256) * 256; } else { return -(env->scaleValue(-sum / 256) * 256); });


static const MbCvModOpTableEntry_t mbCvModOpTable[MBCV_NUM_MOD_OP] = {
    OP_TABLE_ITEM(None),
    OP_TABLE_ITEM(Src1Only),
    OP_TABLE_ITEM(Src2Only),
    OP_TABLE_ITEM(Plus),
    OP_TABLE_ITEM(Minus),
    OP_TABLE_ITEM(Multiply),
    OP_TABLE_ITEM(Xor),
    OP_TABLE_ITEM(Or),
    OP_TABLE_ITEM(And),
    OP_TABLE_ITEM(Min),
    OP_TABLE_ITEM(Max),
    OP_TABLE_ITEM(Lt),
    OP_TABLE_ITEM(Gt),
    OP_TABLE_ITEM(Eq),
    OP_TABLE_ITEM(SandH),
    OP_TABLE_ITEM(Fts),
};


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
            s32 mod_src1_value = 0;
            if( mp->src1 && mp->src1_chn < CV_SE_NUM ) {
                if( mp->src1 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src1_value = (mp->src1 & 0x7f) << 7;
                } else if( mp->src1 < MBCV_NUM_MOD_SRC ) {
                    // modulation range +/- 0x3fff
                    const MbCvModSrcTableEntry_t *srcItem = &mbCvModSrcTable[mp->src1];
                    mod_src1_value = srcItem->getFunct(env, mp->src1_chn) / 2;
                }
            }

            // second source
            s32 mod_src2_value = 0;
            if( mp->src2 && mp->src2_chn < CV_SE_NUM ) {
                if( mp->src2 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src2_value = (mp->src2 & 0x7f) << 7;
                } else {
                    // modulation range +/- 0x3fff
                    const MbCvModSrcTableEntry_t *srcItem = &mbCvModSrcTable[mp->src2];
                    mod_src2_value = srcItem->getFunct(env, mp->src2_chn) / 2;
                }
            }

            // apply operator
            u8 opNum = mp->op & 0xf;
            s16 mod_result = 0;
            if( opNum < MBCV_NUM_MOD_OP ) {
                const MbCvModOpTableEntry_t *opItem = &mbCvModOpTable[opNum];
                mod_result = opItem->modifyFunct(env, this, i, mod_src1_value, mod_src2_value);
            }

            // store in modulator source array for feedbacks
            // use value w/o depth and offset, this has two advantages:
            // - maximum resolution when forwarding the data value
            // - original MOD value can be taken for sample&hold feature
            // bit it also has disadvantage:
            // - the user could think it is a bug when depth doesn't affect the feedback MOD value...
            modOut[i] = mod_result;

            // forward to destinations
            if( mod_result || mp->offset ) {
                s32 scaled_mod_result = (s32)mp->depth * mod_result / 64; // (+/- 0x7fff * +/- 0x7f) / 128
                // invert result if requested
                s32 mod_dst1 = (mp->op & (1 << 6)) ? -scaled_mod_result : scaled_mod_result;
                s32 mod_dst2 = (mp->op & (1 << 7)) ? -scaled_mod_result : scaled_mod_result;

                // add result + offset to modulation target array
                u8 dst1 = mp->dst1;
                if( dst1 && dst1 <= MBCV_NUM_MOD_DST )
                    modDst[dst1] += mod_dst1 + 512 * mp->offset;
	
                u8 dst2 = mp->dst2;
                if( dst2 && dst2 <= MBCV_NUM_MOD_DST )
                    modDst[dst2] += mod_dst2 + 512 * mp->offset;
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

