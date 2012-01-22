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

#ifndef _MB_CV_MOD_H
#define _MB_CV_MOD_H

#include <mios32.h>
#include "MbCvStructs.h"


// number of MOD nodes
#define MBCV_NUM_MOD 4

// Modulation source assignments
#define MBCV_MOD_SRC_ENV1      0
#define MBCV_MOD_SRC_ENV2      1
#define MBCV_MOD_SRC_LFO1      2
#define MBCV_MOD_SRC_LFO2      3
#define MBCV_MOD_SRC_MOD1      4
#define MBCV_MOD_SRC_MOD2      5
#define MBCV_MOD_SRC_MOD3      6
#define MBCV_MOD_SRC_MOD4      7
#define MBCV_MOD_SRC_KEY       8
#define MBCV_MOD_SRC_VEL       9
#define MBCV_MOD_SRC_MDW      10
#define MBCV_MOD_SRC_PBN      11
#define MBCV_MOD_SRC_ATH      12
#define MBCV_MOD_SRC_KNOB1    13
#define MBCV_MOD_SRC_KNOB2    14
#define MBCV_MOD_SRC_KNOB3    15
#define MBCV_MOD_SRC_KNOB4    16
#define MBCV_MOD_SRC_KNOB5    17
#define MBCV_MOD_SRC_KNOB6    18
#define MBCV_MOD_SRC_KNOB7    19
#define MBCV_MOD_SRC_KNOB8    20
#define MBCV_MOD_SRC_AIN1     21
#define MBCV_MOD_SRC_AIN2     22
#define MBCV_MOD_SRC_AIN3     23
#define MBCV_MOD_SRC_AIN4     24
#define MBCV_MOD_SRC_AIN5     25
#define MBCV_MOD_SRC_AIN6     26
#define MBCV_MOD_SRC_AIN7     27
#define MBCV_MOD_SRC_AIN8     28
#define MBCV_MOD_SRC_SEQ_ENVMOD 29
#define MBCV_MOD_SRC_SEQ_ACCENT 30

#define MBCV_NUM_MOD_SRC      31


// Modulation destination assignments
#define MBCV_MOD_DST_PITCH     0
#define MBCV_MOD_DST_LFO1_A    1
#define MBCV_MOD_DST_LFO2_A    2
#define MBCV_MOD_DST_LFO1_R    3
#define MBCV_MOD_DST_LFO2_R    4
#define MBCV_MOD_DST_ENV1_A    5
#define MBCV_MOD_DST_ENV2_A    6
#define MBCV_MOD_DST_ENV1_R    7
#define MBCV_MOD_DST_ENV2_R    8
// maybe we should also control the ENV2 step? Too complicated?

#define MBCV_NUM_MOD_DST       9


class MbCvMod
{
public:

    // Constructor
    MbCvMod();

    // Destructor
    ~MbCvMod();

    // MOD init function
    void init(u8 _modNum);

    // Modulation Matrix handler
    void tick(void);

    // modulation parmeters
    typedef struct modPatchT {
        s8 depth;
        u8 src1;
        u8 src1_chn;
        u8 src2;
        u8 src2_chn;
        u8 op;
        u8 dst1;
        u8 dst2;
    } ModPatchT;

    ModPatchT modPatch[MBCV_NUM_MOD];

    // Values of modulation sources
    s32 modSrc[MBCV_NUM_MOD_SRC];

    // Values of modulation destinations
    s32 modDst[MBCV_NUM_MOD_DST];

    s32 takeDstValue(const u8& ix);

protected:
    // flags modulation transitions
    u8 modTransition;
};

#endif /* _MB_CV_MOD_H */
