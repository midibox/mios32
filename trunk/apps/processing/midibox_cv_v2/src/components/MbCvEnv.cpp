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

#include <string.h>
#include "MbCvEnv.h"
#include "MbCvTables.h"
#include "CapChargeCurve.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnv::MbCvEnv()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnv::~MbCvEnv()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbCvEnv::init(void)
{
    MbCvEnvBase::init();

    // clear additional flags
    accentReq = false;

    // clear variables
    envDelay = 0;
    envAttack = 0;
    envDecay = 48;
    envDecayAccented = 16;
    envSustain = 64;
    envRelease = 32;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbCvEnv::tick(const u8 &updateSpeedFactor)
{
    bool sustainPhase = false; // will be the return value
    if( restartReq ) {
        restartReq = false;
        envState = MBCV_ENV_STATE_ATTACK;
        envDelayCtr = envDelay ? 1 : 0;
        envInitialLevel = envWaveOut;
        envCtr = 0;
    }

    if( releaseReq ) {        
        releaseReq = false;
        envState = MBCV_ENV_STATE_RELEASE;
        envInitialLevel = envWaveOut;
        envCtr = 0;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envModeClkSync && !syncClockReq ) {
        if( envState == MBCV_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        switch( envState ) {
        case MBCV_ENV_STATE_ATTACK: {
            if( envDelayCtr ) {
                int newDelayCtr = envDelayCtr + (mbCvEnvTable[envDelay] / (envModeFast ? 1 : updateSpeedFactor));
                if( newDelayCtr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = newDelayCtr; // delay not passed
                    return false; // no error
                }
            }

            // the rate can be modulated
            s32 attack = envAttack + (envRateModulation / 512);
            if( attack > 255 ) attack = 255; else if( attack < 0 ) attack = 0;
  
            u16 incrementer = mbCvEnvTable[attack] / (envModeFast ? 1 : updateSpeedFactor);
            if( step(envInitialLevel, 0xffff, incrementer, false) )
                envState = MBCV_ENV_STATE_DECAY;
        } break;
  
        case MBCV_ENV_STATE_DECAY: {
            // the rate can be modulated
            s32 decay = accentReq ? envDecayAccented : envDecay;
            decay = decay + (envRateModulation / 512);
            if( decay > 255 ) decay = 255; else if( decay < 0 ) decay = 0;

            u16 incrementer = mbCvEnvTable[decay] / (envModeFast ? 1 : updateSpeedFactor);
            if( step(0xffff, envSustain << 8, incrementer, false) ) {
                envState = envModeOneshot ? MBCV_ENV_STATE_SUSTAIN : MBCV_ENV_STATE_RELEASE;
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;

                envInitialLevel = envWaveOut;
                envCtr = 0;
            }
        } break;
  
        case MBCV_ENV_STATE_SUSTAIN:
            // always update sustain level
            envWaveOut = envSustain << 8;
            envInitialLevel = envWaveOut;
            envCtr = 0;
            break;
  
        case MBCV_ENV_STATE_RELEASE: {

            // the rate can be modulated
            s32 release = envRelease + (envRateModulation / 512);
            if( release > 255 ) release = 255; else if( release < 0 ) release = 0;

            u16 incrementer = mbCvEnvTable[release] / (envModeFast ? 1 : updateSpeedFactor);
            if( step(envInitialLevel, 0x0000, incrementer, false) ) {
                if( envModeOneshot ) {
                    envState = MBCV_ENV_STATE_IDLE;
                } else {
                    envState = MBCV_ENV_STATE_ATTACK;
                }
            }
        } break;
  
        default: // like MBCV_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // the amplitude can be modulated
    s32 amplitude = envAmplitude + (envAmplitudeModulation / 512);
    if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

    // final output value
    //envOut = ((s32)(envWaveOut / 2) * (s32)amplitude) / 128;
    // multiplied by *2 to allow envelopes over full range
    envOut = ((s32)envWaveOut * (s32)amplitude) / 128;

    accentReq = false;

    return sustainPhase;
}
