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
    init(SID_SE_LEAD, 1, NULL, NULL);
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
void MbSidEnv::init(sid_se_engine_t _engine, u8 _updateSpeedFactor, sid_se_env_patch_t *_envPatch, MbSidClock *_mbSidClockPtr)
{
    engine = _engine;
    updateSpeedFactor = _updateSpeedFactor;
    envPatch = _envPatch;
    mbSidClockPtr = _mbSidClockPtr;

    // clear flags
    restartReq = 0;
    releaseReq = 0;
    accentReq = 0;

    // clear references
    modSrcEnv = 0;
    modDstPitch = 0;
    modDstPw = 0;
    modDstFilter = 0;
    decayA = 0;
    accent = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnv::tick(void)
{
    if( !envPatch ) // exit if no patch reference initialized
        return false;

    if( engine == SID_SE_LEAD )
        return tickLead();

    // Reduced version for Bassline/Multi engine
    bool sustainPhase = false; // will be the return value
    
    sid_se_env_mode_t envMode;
    envMode.ALL = envPatch->mode;

    if( restartReq ) {
        restartReq = 0;
        state = MBSID_ENV_STATE_ATTACK1;
        delayCtr = 0; // delay counter not supported by this function...
    }

    if( releaseReq ) {
        releaseReq = 0;
        state = MBSID_ENV_STATE_RELEASE1;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.MINIMAL.CLKSYNC && (!mbSidClockPtr->event.CLK || mbSidClockPtr->clkCtr6 != 0) ) {
        if( state == MBSID_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        switch( state ) {
        case MBSID_ENV_STATE_ATTACK1: {
            u8 curve = envMode.MINIMAL.CURVE_A ? envPatch->MINIMAL.curve : 0x80;
            if( step(0xffff, envPatch->MINIMAL.attack, curve) )
                state = MBSID_ENV_STATE_DECAY1;
        } break;
  
        case MBSID_ENV_STATE_DECAY1: {
            // use alternative decay on accented notes (only used for basslines)
            u8 decay = (decayA != NULL && accentReq) ? *decayA : envPatch->MINIMAL.decay;
            u8 curve = envMode.MINIMAL.CURVE_D ? envPatch->MINIMAL.curve : 0x80;
            if( step(envPatch->MINIMAL.sustain << 8, decay, curve) )
                state = MBSID_ENV_STATE_SUSTAIN;
        } break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            ctr = envPatch->MINIMAL.sustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE1: {
            u8 curve = envMode.MINIMAL.CURVE_R ? envPatch->MINIMAL.curve : 0x80;
            if( ctr )
                step(0x0000, envPatch->MINIMAL.release, curve);
        } break;
  
        default: // like MBSID_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // directly write to modulation destinations depending on depths
    // if accent flag of voice set: increase/decrease depth based on accent value
    s32 depth_p = (s32)envPatch->MINIMAL.depth_p - 0x80;
    if( depth_p && accentReq ) {
        if( depth_p >= 0 ) {
            if( (depth_p += *accent) > 128 ) depth_p = 128;
        } else {
            if( (depth_p -= *accent) < -128 ) depth_p = -128;
        }
    }
    *modDstPitch += ((ctr/2) * depth_p) / 128;

    s32 depth_pw = (s32)envPatch->MINIMAL.depth_pw - 0x80;
    if( depth_pw && accentReq ) {
        if( depth_pw >= 0 ) {
            if( (depth_pw += *accent) > 128 ) depth_pw = 128;
        } else {
            if( (depth_pw -= *accent) < -128 ) depth_pw = -128;
        }
    }
    *modDstPw += ((ctr/2) * depth_pw) / 128;

    s32 depth_f = (s32)envPatch->MINIMAL.depth_f - 0x80;
    if( depth_f && accentReq ) {
        if( depth_f >= 0 ) {
            if( (depth_f += *accent) > 128 ) depth_f = 128;
        } else {
            if( (depth_f -= *accent) < -128 ) depth_f = -128;
        }
    }
    *modDstFilter += ((ctr/2) * depth_f) / 128;

    accentReq = 0;

    return sustainPhase;
}


// Extended version for Lead engine
bool MbSidEnv::tickLead(void)
{
    bool sustainPhase = false; // will be the return value

    sid_se_env_mode_t envMode;
    envMode.ALL = envPatch->mode;

    if( restartReq ) {
        restartReq = 0;
        state = MBSID_ENV_STATE_ATTACK1;
        delayCtr = envPatch->L.delay ? 1 : 0;
    }

    if( releaseReq ) {
        releaseReq = 0;
        state = MBSID_ENV_STATE_RELEASE1;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.L.CLKSYNC && (!mbSidClockPtr->event.CLK || mbSidClockPtr->clkCtr6 != 0) ) {
        if( state == MBSID_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        switch( state ) {
        case MBSID_ENV_STATE_ATTACK1:
            if( delayCtr ) {
                int new_delay_ctr = delayCtr + (mbSidEnvTable[envPatch->L.delay] / updateSpeedFactor);
                if( new_delay_ctr > 0xffff )
                    delayCtr = 0; // delay passed
                else {
                    delayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }
  
            if( step(envPatch->L.attlvl << 8, envPatch->L.attack1, envPatch->L.att_curve) )
                state = MBSID_ENV_STATE_ATTACK2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_ATTACK2:
            if( step(0xffff, envPatch->L.attack2, envPatch->L.att_curve) )
                state = MBSID_ENV_STATE_DECAY1; // TODO: Set Phase depending on mode
            break;

        case MBSID_ENV_STATE_DECAY1:
            if( step(envPatch->L.declvl << 8, envPatch->L.decay1, envPatch->L.dec_curve) )
                state = MBSID_ENV_STATE_DECAY2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_DECAY2:
            if( step(envPatch->L.sustain << 8, envPatch->L.decay2, envPatch->L.dec_curve) ) {
                state = MBSID_ENV_STATE_SUSTAIN; // TODO: Set Phase depending on mode
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
            break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            ctr = envPatch->L.sustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE1:
            if( step(envPatch->L.rellvl << 8, envPatch->L.release1, envPatch->L.rel_curve) )
                state = MBSID_ENV_STATE_RELEASE2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_RELEASE2:
            if( ctr )
                step(0x0000, envPatch->L.release2, envPatch->L.rel_curve);
            break;
  
        default: // like MBSID_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // scale to ENV depth
    s32 env_depth = (s32)envPatch->L.depth - 0x80;
    
    // final ENV value (range +/- 0x7fff)
    if( modSrcEnv )
        *modSrcEnv = ((ctr/2) * env_depth) / 128;

    return sustainPhase;
}


bool MbSidEnv::step(u16 target, u8 rate, u8 curve)
{
    if( target == ctr )
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
        int feedback = (abs_curve * (ctr>>8)) >> 8; 
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
    if( target > ctr ) {
        s32 new_ctr = (s32)ctr + (inc_rate / updateSpeedFactor);
        if( new_ctr >= target ) {
            ctr = target;
            return true; // next state
        }
        ctr = new_ctr;
        return false; // stay in state
    }

    s32 new_ctr = (s32)ctr - (inc_rate / updateSpeedFactor);
    if( new_ctr <= target ) {
        ctr = target;
        return true; // next state
    }
    ctr = new_ctr;

    return false; // stay in state
}

