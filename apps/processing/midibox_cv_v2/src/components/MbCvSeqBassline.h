/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Bassline Sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_SEQ_BASSLINE_H
#define _MB_CV_SEQ_BASSLINE_H

#include <mios32.h>
#include "MbCvStructs.h"
#include "MbCvSeq.h"
#include "MbCvVoice.h"

class MbCv; // forward declaration

#define MBCV_SEQ_BASSLINE_NUM_STEPS    32
#define MBCV_SEQ_BASSLINE_NUM_PATTERNS  8


class MbCvSeqBassline : public MbCvSeq
{
public:

    typedef union {
        u8 ALL;

        struct {
            u8 gate:1;
            u8 glide:1;
            u8 accent:1;
        };
    } BasslineArgsT;
    

    // Constructor
    MbCvSeqBassline();

    // Destructor
    ~MbCvSeqBassline();

    // sequencer init function
    void init(void);

    // sequencer handler
    void tick(MbCvVoice *v, MbCv *cv);

    // input parameters: see also MbCvSeq.h

    // additional parameters for bassline sequencer
    u8 seqGateLength;
    u8 seqEnvMod;
    u8 seqAccent;
    u8 seqAccentEffective;

    // sequencer memory
    u8            seqBasslineKey[MBCV_SEQ_BASSLINE_NUM_PATTERNS][MBCV_SEQ_BASSLINE_NUM_STEPS];
    BasslineArgsT seqBasslineArgs[MBCV_SEQ_BASSLINE_NUM_PATTERNS][MBCV_SEQ_BASSLINE_NUM_STEPS];

};

#endif /* _MB_CV_SEQ_BASSLINE_H */
