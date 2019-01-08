/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Envelope Generator
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
#include "MbSidEnv.h"
#include "MbSidTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnv::MbSidEnv()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnv::~MbSidEnv()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbSidEnv::init(void)
{
    // clear flags
    restartReq = false;
    releaseReq = false;
    syncClockReq = false;
    accentReq = false;

    // clear variables
    envMode.ALL = 0;
    envDelay = 0;
    envAttack = 0;
    envDecay = 0;
    envDecayAccented = 0;
    envSustain = 0;
    envRelease = 0;
    envCurve = 0;

    envDepthPitch = 0;
    envDepthPulsewidth = 0;
    envDepthFilter = 0;

    envOut = 0;

    envState = MBSID_ENV_STATE_IDLE;
    envCtr = 0;
    envDelayCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnv::tick(const u8 &updateSpeedFactor)
{
    bool sustainPhase = false; // will be the return value

    if( restartReq ) {
        restartReq = false;
        envState = MBSID_ENV_STATE_ATTACK;
        envDelayCtr = envDelay ? 1 : 0;
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBSID_ENV_STATE_RELEASE;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.L.CLKSYNC && !syncClockReq ) {
        if( envState == MBSID_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        switch( envState ) {
        case MBSID_ENV_STATE_ATTACK: {
            if( envDelayCtr ) {
                int new_delay_ctr = envDelayCtr + (mbSidEnvTable[envDelay] / updateSpeedFactor);
                if( new_delay_ctr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }
  
            u8 curve = envMode.MINIMAL.CURVE_A ? envCurve : 0x80;
            if( step(0xffff, envAttack, curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY;
        } break;
  
        case MBSID_ENV_STATE_DECAY: {
            u8 decay = accentReq ? envDecayAccented : envDecay;
            u8 curve = envMode.MINIMAL.CURVE_D ? envCurve : 0x80;
            if( step(envSustain << 8, decay, curve, updateSpeedFactor) ) {
                envState = MBSID_ENV_STATE_SUSTAIN; // TODO: Set Phase depending on mode
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
        } break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envSustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE: {
            u8 curve = envMode.MINIMAL.CURVE_R ? envCurve : 0x80;
            if( envCtr )
                step(0x0000, envRelease, curve, updateSpeedFactor);
        } break;
  
        default: // like MBSID_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    envOut = envCtr / 2;

    accentReq = false;

    return sustainPhase;
}



bool MbSidEnv::step(const u16 &target, const u8 &rate, const u8 &curve, const u8 &updateSpeedFactor)
{
    if( target == envCtr )
        return true; // next state

    // modify rate if curve != 0x80
    u16 inc_rate;
    if( curve != 0x80 ) {
        // this nice trick has been proposed by Razmo
        int abs_curve = curve - 0x80;
        if( abs_curve < 0 )
            abs_curve = -abs_curve;
        else
            abs_curve ^= 0x7f; // invert if positive range for more logical behaviour of positive/negative curve

        int rate_msbs = (rate >> 1); // TODO: we could increase resolution by using an enhanced frq_table
        int feedback = (abs_curve * (envCtr>>8)) >> 8; 
        int ix;
        if( curve > 0x80 ) { // bend up
            ix = (rate_msbs ^ 0x7f) - feedback;
            if( ix < 0 )
                ix = 0;
        } else { // bend down
            ix = (rate_msbs ^ 0x7f) + feedback;
            if( ix >= 127 )
                ix = 127;
        }
        inc_rate = mbSidFrqTable[ix];
    } else {
        inc_rate = mbSidEnvTable[rate];
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
