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
    envAttack = 48;
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
    const bool rateFromEnvTable = true;
    bool sustainPhase = false; // will be the return value

    if( restartReq ) {
        restartReq = false;
        envState = MBCV_ENV_STATE_ATTACK;
        envDelayCtr = envDelay ? 1 : 0;
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBCV_ENV_STATE_RELEASE;
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
                int new_delay_ctr = envDelayCtr + (mbCvEnvTable[envDelay] / (envModeFast ? 1 : updateSpeedFactor));
                if( new_delay_ctr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }

            // the rate can be modulated
            s32 attack = envAttack + (envRateModulation / 512);
            if( attack > 255 ) attack = 255; else if( attack < 0 ) attack = 0;
  
            s8 curve = envCurve ? ((envAttack < 64) ? (128-envAttack) : 64) : 0;
            if( step(0xffff, attack, curve, envModeFast ? 1 : updateSpeedFactor, rateFromEnvTable) )
                envState = MBCV_ENV_STATE_DECAY;
        } break;
  
        case MBCV_ENV_STATE_DECAY: {
            // the rate can be modulated
            s32 decay = accentReq ? envDecayAccented : envDecay;
            decay = decay + (envRateModulation / 512);
            if( decay > 255 ) decay = 255; else if( decay < 0 ) decay = 0;

            s8 curve = envCurve ? -64 : 0;
            if( step(envSustain << 8, decay, curve, envModeFast ? 1 : updateSpeedFactor, rateFromEnvTable) ) {
                envState = envModeOneshot ? MBCV_ENV_STATE_SUSTAIN : MBCV_ENV_STATE_RELEASE;
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
        } break;
  
        case MBCV_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envSustain << 8;
            break;
  
        case MBCV_ENV_STATE_RELEASE: {
            s8 curve = envCurve ? -64 : 0;
            if( envCtr ) {
                // the rate can be modulated
                s32 release = envRelease + (envRateModulation / 512);
                if( release > 255 ) release = 255; else if( release < 0 ) release = 0;

                step(0x0000, release, curve, envModeFast ? 1 : updateSpeedFactor, rateFromEnvTable);
            } else if( !envModeOneshot )
                envState = MBCV_ENV_STATE_ATTACK; // restart if not in oneshot mode
        } break;
  
        default: // like MBCV_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // the amplitude can be modulated
    s32 amplitude = envAmplitude + (envAmplitudeModulation / 512);
    if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

    // final output value
    //envOut = ((s32)(envCtr / 2) * (s32)amplitude) / 128;
    // multiplied by *2 to allow envelopes over full range
    envOut = ((s32)envCtr * (s32)amplitude) / 128;

    accentReq = false;

    return sustainPhase;
}
