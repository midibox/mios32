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
#define MBCV_MOD_SRC_NONE      0
#define MBCV_MOD_SRC_ENV1      1
#define MBCV_MOD_SRC_ENV2      2
#define MBCV_MOD_SRC_LFO1      3
#define MBCV_MOD_SRC_LFO2      4
#define MBCV_MOD_SRC_MOD1      5
#define MBCV_MOD_SRC_MOD2      6
#define MBCV_MOD_SRC_MOD3      7
#define MBCV_MOD_SRC_MOD4      8
#define MBCV_MOD_SRC_KEY       9
#define MBCV_MOD_SRC_VEL      10
#define MBCV_MOD_SRC_MDW      11
#define MBCV_MOD_SRC_PBN      12
#define MBCV_MOD_SRC_ATH      13
#define MBCV_MOD_SRC_KNOB1    14
#define MBCV_MOD_SRC_KNOB2    15
#define MBCV_MOD_SRC_KNOB3    16
#define MBCV_MOD_SRC_KNOB4    17
#define MBCV_MOD_SRC_KNOB5    18
#define MBCV_MOD_SRC_KNOB6    19
#define MBCV_MOD_SRC_KNOB7    20
#define MBCV_MOD_SRC_KNOB8    21
#define MBCV_MOD_SRC_AIN1     22
#define MBCV_MOD_SRC_AIN2     23
#define MBCV_MOD_SRC_AIN3     24
#define MBCV_MOD_SRC_AIN4     25
#define MBCV_MOD_SRC_AIN5     26
#define MBCV_MOD_SRC_AIN6     27
#define MBCV_MOD_SRC_AIN7     28
#define MBCV_MOD_SRC_AIN8     29
#define MBCV_MOD_SRC_SEQ_ENVMOD 30
#define MBCV_MOD_SRC_SEQ_ACCENT 31

#define MBCV_NUM_MOD_SRC      32


// Modulation Operators
#define MBCV_MOD_OP_NONE       0
#define MBCV_MOD_OP_SRC1_ONLY  1
#define MBCV_MOD_OP_SRC2_ONLY  2
#define MBCV_MOD_OP_PLUS       3
#define MBCV_MOD_OP_MINUS      4
#define MBCV_MOD_OP_MULTIPLY   5
#define MBCV_MOD_OP_XOR        6
#define MBCV_MOD_OP_OR         7
#define MBCV_MOD_OP_AND        8
#define MBCV_MOD_OP_MIN        9
#define MBCV_MOD_OP_MAX        10
#define MBCV_MOD_OP_LT         11
#define MBCV_MOD_OP_GT         12
#define MBCV_MOD_OP_EQ         13
#define MBCV_MOD_OP_S_AND_H    14
#define MBCV_MOD_OP_FTS        15

#define MBCV_NUM_MOD_OP        16


// Modulation destination assignments
#define MBCV_MOD_DST_NONE      0
#define MBCV_MOD_DST_CV        1
#define MBCV_MOD_DST_LFO1_A    2
#define MBCV_MOD_DST_LFO2_A    3
#define MBCV_MOD_DST_LFO1_R    4
#define MBCV_MOD_DST_LFO2_R    5
#define MBCV_MOD_DST_ENV1_A    6
#define MBCV_MOD_DST_ENV2_A    7
#define MBCV_MOD_DST_ENV1_R    8
#define MBCV_MOD_DST_ENV2_R    9
// maybe we should also control the ENV2 step? Too complicated?

#define MBCV_NUM_MOD_DST       10


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
        s8 offset;
        u8 src1;
        u8 src1_chn;
        u8 src2;
        u8 src2_chn;
        u8 op;
        u8 dst1;
        u8 dst2;
    } ModPatchT;

    ModPatchT modPatch[MBCV_NUM_MOD];

    // Output values of modulation paths
    s16 modOut[MBCV_NUM_MOD];

    // Values of modulation destinations
    s32 modDst[MBCV_NUM_MOD_DST];

    s32 takeDstValue(const u8& ix);

    // flags modulation transitions
    u8 modTransition;
};

#endif /* _MB_CV_MOD_H */
