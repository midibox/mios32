/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV LFO
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_LFO_H
#define _MB_CV_LFO_H

#include <mios32.h>
#include "MbCvStructs.h"
#include "MbCvRandomGen.h"


class MbCvLfo
{
public:
    // Constructor
    MbCvLfo();

    // Destructor
    ~MbCvLfo();

    // LFO init function
    void init();

    // LFO handler (returns true on overrun)
    bool tick(const u8 &updateSpeedFactor);

    // input parameters
    cv_se_lfo_mode_t lfoMode;
    s8 lfoAmplitude;
    u8 lfoRate;
    u8 lfoDelay;
    u8 lfoPhase;

    s8 lfoDepthPitch;
    s8 lfoDepthLfoAmplitude;
    s8 lfoDepthLfoRate;
    s8 lfoDepthEnvAmplitude;
    s8 lfoDepthEnvDecay;

    s32 lfoAmplitudeModulation;
    s32 lfoRateModulation;

    // output waveform
    s16 lfoOut;

    // requests a restart
    bool restartReq;

    // set by each 6th MIDI clock (if LFO in SYNC mode)
    bool syncClockReq;

    // random generator
    MbCvRandomGen randomGen;

protected:
    // internal variables
    u16 lfoCtr;
    u16 lfoDelayCtr;
};

#endif /* _MB_CV_LFO_H */
