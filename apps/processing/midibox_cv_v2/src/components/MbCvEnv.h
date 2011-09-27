/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ENV_H
#define _MB_CV_ENV_H

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


class MbCvEnv
{
public:
    // Constructor
    MbCvEnv();

    // Destructor
    ~MbCvEnv();

    // ENV init function
    virtual void init(void);

    // ENV handler (returns true when sustain phase reached)
    virtual bool tick(const u8 &updateSpeedFactor);

    // input parameters
    cv_se_env_mode_t envMode;
    s8 envDepth;
    u8 envDelay;
    u8 envAttack;
    u8 envDecay;
    u8 envDecayAccented;
    u8 envSustain;
    u8 envRelease;
    u8 envCurve;

    // used by Multi and Bassline engine - too lazy to create a new class for these three variables
    s8 envDepthPitch;
    s8 envDepthPulsewidth;
    s8 envDepthFilter;

    // output waveform
    s16 envOut;

    // requests a restart and release phase
    bool restartReq;
    bool releaseReq;

    // set by each 6th MIDI clock (if ENV in SYNC mode)
    bool syncClockReq;

    // requests to use accented parameters
    bool accentReq;


protected:
    bool step(const u16 &target, const u8 &rate, const u8 &curve, const u8 &updateSpeedFactor);

    // internal variables
    mbcv_env_state_t envState;
    u16 envCtr;
    u16 envDelayCtr;

};

#endif /* _MB_CV_ENV_H */
