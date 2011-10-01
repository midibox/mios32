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
    // clear flags
    restartReq = false;
    releaseReq = false;
    syncClockReq = false;
    accentReq = false;

    // clear variables
    envMode.ALL = 0;
    envMode.CURVE_D = 1;
    envAmplitude = 0;
    envDelay = 0;
    envAttack = 48;
    envDecay = 48;
    envDecayAccented = 16;
    envSustain = 64;
    envRelease = 32;
    envCurve = 0;

    envDepthPitch = 127;
    envDepthLfo1Amplitude = 0;
    envDepthLfo1Rate = 0;
    envDepthLfo2Amplitude = 0;
    envDepthLfo2Rate = 0;

    envAmplitudeModulation = 0;
    envDecayModulation = 0;

    envOut = 0;

    envState = MBCV_ENV_STATE_IDLE;
    envCtr = 0;
    envDelayCtr = 0;
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
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBCV_ENV_STATE_RELEASE;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.CLKSYNC && !syncClockReq ) {
        if( envState == MBCV_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        switch( envState ) {
        case MBCV_ENV_STATE_ATTACK: {
            if( envDelayCtr ) {
                int new_delay_ctr = envDelayCtr + (mbCvEnvTable[envDelay] / updateSpeedFactor);
                if( new_delay_ctr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }
  
            s8 curve = envMode.CURVE_A ? envCurve : 0;
            if( step(0xffff, envAttack, curve, updateSpeedFactor) )
                envState = MBCV_ENV_STATE_DECAY;
        } break;
  
        case MBCV_ENV_STATE_DECAY: {
            // the decay can be modulated
            s32 decay = accentReq ? envDecayAccented : envDecay;
            decay = decay + (envAmplitudeModulation / 512);
            if( decay > 255 ) decay = 255; else if( decay < 0 ) decay = 0;

            s8 curve = envMode.CURVE_D ? envCurve : 0;
            if( step(envSustain << 8, decay, curve, updateSpeedFactor) ) {
                envState = MBCV_ENV_STATE_SUSTAIN; // TODO: Set Phase depending on mode
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
        } break;
  
        case MBCV_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envSustain << 8;
            break;
  
        case MBCV_ENV_STATE_RELEASE: {
            s8 curve = envMode.CURVE_R ? envCurve : 0;
            if( envCtr )
                step(0x0000, envRelease, curve, updateSpeedFactor);
        } break;
  
        default: // like MBCV_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // the amplitude can be modulated
    s32 amplitude = envAmplitude + (envAmplitudeModulation / 512);
    if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

    // final output value
    envOut = ((s32)(envCtr / 2) * (s32)amplitude) / 128;

    accentReq = false;

    return sustainPhase;
}



bool MbCvEnv::step(const u16 &target, const u8 &rate, const s8 &curve, const u8 &updateSpeedFactor)
{
    if( target == envCtr )
        return true; // next state

    // modify rate if curve != 0x80
    u16 inc_rate;
    if( curve ) {
        // this nice trick has been proposed by Razmo
        int abs_curve = curve;
        if( abs_curve < 0 )
            abs_curve = -abs_curve;
        else
            abs_curve ^= 0x7f; // invert if positive range for more logical behaviour of positive/negative curve

        int rate_msbs = (rate >> 1); // TODO: we could increase resolution by using an enhanced frq_table
        int feedback = (abs_curve * (envCtr>>8)) >> 8; 
        int ix;
        if( curve > 0 ) { // bend up
            ix = (rate_msbs ^ 0x7f) - feedback;
            if( ix < 0 )
                ix = 0;
        } else { // bend down
            ix = (rate_msbs ^ 0x7f) + feedback;
            if( ix >= 127 )
                ix = 127;
        }
        inc_rate = mbCvFrqTable[ix];
    } else {
        inc_rate = mbCvEnvTable[rate];
    }

    // positive or negative direction?
    if( target > envCtr ) {
        s32 newCtr = (s32)envCtr + (inc_rate / updateSpeedFactor);
        if( newCtr >= target ) {
            envCtr = target;
            return true; // next state
        }
        envCtr = newCtr;
        return false; // stay in state
    }

    s32 newCtr = (s32)envCtr - (inc_rate / updateSpeedFactor);
    if( newCtr <= target ) {
        envCtr = target;
        return true; // next state
    }
    envCtr = newCtr;

    return false; // stay in state
}
