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
#include "MbCvTables.h"


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
            envLevel[i] = 128 + i * (256/MBCV_ENV_MULTI_NUM_STEPS);
        else
            envLevel[i] = 255 - (i-(MBCV_ENV_MULTI_NUM_STEPS/2)) * (256/MBCV_ENV_MULTI_NUM_STEPS);
    }

    envOffset = 0;
    envRate = 32;

    envLoopAttack = 0;
    envSustainStep = 9;
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
    bool sustainPhase = false; // will be the return value

    if( restartReq ) {
        restartReq = false;
        envState = MBCV_ENV_STATE_ATTACK;
        envCurrentStep = 0;
        envCtr = 0;
        recalc(updateSpeedFactor);
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBCV_ENV_STATE_RELEASE;

        if( envSustainStep && envSustainStep <= MBCV_ENV_MULTI_NUM_STEPS ) {
            envCurrentStep = envSustainStep - 1;
            envCtr = 0;
            recalc(updateSpeedFactor);
        }
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

            envWaveOut = envLevel[sustainStep] << 8;
        } else {
            // the rate can be modulated
            s32 rate = envRate + (envRateModulation / 128);
            if( rate > 255 ) rate = 255; else if( rate < 0 ) rate = 0;

            u16 incrementer = mbCvEnvTable[rate] / (envModeFast ? 1 : updateSpeedFactor);
            if( step(envLevel[envCurrentStep] << 8, envLevel[envNextStep] << 8, incrementer, true) ) {
                envCurrentStep = envNextStep;

                if( (envSustainStep && envCurrentStep == (envSustainStep-1)) ) {
                    if( envLoopAttack ) {
                        envState = MBCV_ENV_STATE_ATTACK;
                        envCurrentStep = envLoopAttack;
                        recalc(updateSpeedFactor);
                    } else {
                        envState = MBCV_ENV_STATE_SUSTAIN;
  
                        // propagate sustain phase to trigger matrix
                        sustainPhase = true;
                    }
                } else {
                    recalc(updateSpeedFactor);
                }
            }
        }
    }

    // the amplitude can be modulated
    s32 amplitude = envAmplitude + (envAmplitudeModulation / 512);
    if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

    // final output value
#if 0
    // unidirectional (for typical ENV behaviour)
    envOut = ((s32)envWaveOut * (s32)amplitude) / 128;
#else
    // better:
    // bidirectional (allows to realize a waveform based LFO)
    envOut = (((s32)envWaveOut - 32768 + ((s32)envOffset*512)) * (s32)amplitude) / 64;
#endif

    return sustainPhase;
}


// recalculates envNextStep
void MbCvEnvMulti::recalc(const u8 &updateSpeedFactor)
{
    envNextStep = envCurrentStep + 1;

    if( envNextStep >= MBCV_ENV_MULTI_NUM_STEPS || envNextStep > envLastStep ) {
        if( envLoopRelease ) {
            envNextStep = envLoopRelease - 1;

            if( envNextStep >= MBCV_ENV_MULTI_NUM_STEPS || envNextStep > envLastStep )
                envNextStep = 0;
        } else {
            if( envModeOneshot )
                envNextStep = envCurrentStep; // stop...
            else
                envNextStep = 0;
        }
    }
}
