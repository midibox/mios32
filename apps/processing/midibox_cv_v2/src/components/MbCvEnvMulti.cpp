/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Envelope Generator with multiple stages
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
#include "MbCvEnvMulti.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvMulti::MbCvEnvMulti()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvMulti::~MbCvEnvMulti()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvMulti::init(void)
{
    MbCvEnvBase::init();

    // clear variables
    for(int i=0; i<MBCV_ENV_MULTI_NUM_STEPS; ++i) {
        if( i < (MBCV_ENV_MULTI_NUM_STEPS/2) )
            envLevel[i] = i * (2*256/MBCV_ENV_MULTI_NUM_STEPS);
        else
            envLevel[i] = 255 - (i-(MBCV_ENV_MULTI_NUM_STEPS/2)) * (2*256/MBCV_ENV_MULTI_NUM_STEPS);
        envDelay[i] = 20;
    }

    envOffset = 0;
    envRate = 0;

    envLoopAttack = 0;
    envSustainStep = 8;
    envLoopRelease = 0;
    envLastStep = 15;

    envCurrentStep = 0;

    recalc(2);
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnvMulti::tick(const u8 &updateSpeedFactor)
{
    const bool rateNotFromEnvTable = false;
    bool sustainPhase = false; // will be the return value

    if( restartReq ) {
        restartReq = false;
        envState = MBCV_ENV_STATE_ATTACK;
        envCurrentStep = 0;
        recalc(updateSpeedFactor);
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBCV_ENV_STATE_RELEASE;
        if( envSustainStep && envSustainStep <= MBCV_ENV_MULTI_NUM_STEPS )
            envCurrentStep = envSustainStep - 1;
        recalc(updateSpeedFactor);
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envModeClkSync && !syncClockReq ) {
        if( envState == MBCV_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        if( envState == MBCV_ENV_STATE_SUSTAIN ) {
            // always update sustain level
            u8 sustainStep = envSustainStep - 1;
            if( sustainStep >= MBCV_ENV_MULTI_NUM_STEPS )
                sustainStep = MBCV_ENV_MULTI_NUM_STEPS - 1;

            envCtr = envLevel[sustainStep] << 8;
        } else {
            if( ++envDelayCtr >= envDelayCtrMax ) {
                envCurrentStep = envNextStep;
                envCtr = envLevel[envCurrentStep] << 8;

                if( (envSustainStep && envCurrentStep == (envSustainStep-1)) ||
                    (envLastStep < (MBCV_ENV_MULTI_NUM_STEPS-1) && envCurrentStep == envLastStep) ) {
                    envState = MBCV_ENV_STATE_SUSTAIN;
  
                    // propagate sustain phase to trigger matrix
                    sustainPhase = true;
                } else {
                    recalc(updateSpeedFactor);
                }
            } else {
                u16 nextLevel = envLevel[envNextStep] << 8;

                if( envCurve ) { // tmp. hack to disable curve (will be add as an option later)
                    envCtr = nextLevel;
                } else {
                    if( envCtr != nextLevel ) {
                        s32 newCtr = envCtr + envIncrementer;
                        if( envIncrementer > 0 ) {
                            if( newCtr > nextLevel )
                                newCtr = nextLevel;
                        } else {
                            if( newCtr < nextLevel )
                                newCtr = nextLevel;
                        }
                        envCtr = newCtr;
                    }
                }
            }
        }
    }

    // the amplitude can be modulated
    s32 amplitude = envAmplitude + (envAmplitudeModulation / 512);
    if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

    // final output value
    //envOut = ((s32)(envCtr / 2) * (s32)amplitude) / 128;
    // multiplied by *2 to allow envelopes over full range
    envOut = ((s32)envCtr * (s32)amplitude) / 128;

    return sustainPhase;
}


// recalculates envNextStep and envIncrementer based on current settings
void MbCvEnvMulti::recalc(const u8 &updateSpeedFactor)
{
    bool releasePhase = envState == MBCV_ENV_STATE_RELEASE;

    envNextStep = envCurrentStep + 1;
    if( envNextStep >= MBCV_ENV_MULTI_NUM_STEPS || envNextStep > envLastStep ) {
        if( envModeOneshot )
            envNextStep = envCurrentStep; // stop...
        else
            envNextStep = 0;
    }
    u16 nextLevel = envLevel[envNextStep] << 8;

    // the rate can be modulated
    s32 delay = envDelay[envCurrentStep];
    delay += -envRate;
    delay = delay + (envRateModulation / 128);
    if( delay > 1024 ) delay = 1024; else if( delay < 1 ) delay = 1;

    envDelayCtr = 0;
    envDelayCtrMax = envModeFast ? delay : ((delay * updateSpeedFactor) / 2);

    s32 diff = (s32)nextLevel - (s32)envCtr;
    envIncrementer = diff / (s32)envDelayCtrMax;
    if( envIncrementer == 0 )
        envIncrementer = (diff > 0) ? 1 : -1;
}
