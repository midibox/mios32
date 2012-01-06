/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Base Class for Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ENV_BASE_H
#define _MB_CV_ENV_BASE_H

#include <mios32.h>
#include "MbCvStructs.h"

typedef enum {
    MBCV_ENV_STATE_IDLE = 0,
    MBCV_ENV_STATE_ATTACK,
    MBCV_ENV_STATE_ATTACK2,
    MBCV_ENV_STATE_DECAY,
    MBCV_ENV_STATE_DECAY2,
    MBCV_ENV_STATE_SUSTAIN,
    MBCV_ENV_STATE_RELEASE,
    MBCV_ENV_STATE_RELEASE2
} mbcv_env_state_t;

class MbCvEnvBase
{
public:
    // Constructor
    MbCvEnvBase();

    // Destructor
    ~MbCvEnvBase();

    // ENV init function
    virtual void init(void);

    // ENV handler (returns true when sustain phase reached)
    virtual bool tick(const u8 &updateSpeedFactor);

    // input parameters
    bool envModeClkSync;
    bool envModeKeySync;
    bool envModeOneshot;
    bool envModeFast;

    s8 envAmplitude;
    u8 envCurve;

    s8 envDepthPitch;
    s8 envDepthLfo1Amplitude;
    s8 envDepthLfo1Rate;
    s8 envDepthLfo2Amplitude;
    s8 envDepthLfo2Rate;

    s32 envAmplitudeModulation;
    s32 envDecayModulation;

    // output waveform
    s32 envOut;

    // requests a restart and release phase
    bool restartReq;
    bool releaseReq;

    // set by each 6th MIDI clock (if ENV in SYNC mode)
    bool syncClockReq;


protected:
    bool step(const u16 &target, const u16 &rate, const s8 &curve, const u8 &updateSpeedFactor, const bool& rateFromEnvTable);

    // internal variables
    mbcv_env_state_t envState;

    u16 envCtr;
    u32 envDelayCtr;

};

#endif /* _MB_CV_ENV_BASE_H */
