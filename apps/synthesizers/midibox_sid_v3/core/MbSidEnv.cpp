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
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnv::MbSidEnv()
{
    init(NULL);
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
void MbSidEnv::init(sid_se_env_patch_t *_envPatch)
{
    envPatch = _envPatch;

    // clear flags
    restartReq = false;
    releaseReq = false;
    syncClockReq = false;
    accentReq = false;

    // clear variables
    envState = MBSID_ENV_STATE_IDLE;
    envCtr = 0;
    envDelayCtr = 0;

    // clear references
    modSrcEnv = NULL;
    modDstPitch = NULL;
    modDstPw = NULL;
    modDstFilter = NULL;
    decayA = NULL;
    accent = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnv::tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor)
{
    if( !envPatch ) // exit if no patch reference initialized
        return false;

    if( engine == SID_SE_LEAD )
        return tickLead(engine, updateSpeedFactor);

    // Reduced version for Bassline/Multi engine
    bool sustainPhase = false; // will be the return value
    
    sid_se_env_mode_t envMode;
    envMode.ALL = envPatch->mode;

    if( restartReq ) {
        restartReq = false;
        envState = MBSID_ENV_STATE_ATTACK1;
        envDelayCtr = 0; // delay counter not supported by this function...
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBSID_ENV_STATE_RELEASE1;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.MINIMAL.CLKSYNC && !syncClockReq ) {
        if( envState == MBSID_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        switch( envState ) {
        case MBSID_ENV_STATE_ATTACK1: {
            u8 curve = envMode.MINIMAL.CURVE_A ? envPatch->MINIMAL.curve : 0x80;
            if( step(0xffff, envPatch->MINIMAL.attack, curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY1;
        } break;
  
        case MBSID_ENV_STATE_DECAY1: {
            // use alternative decay on accented notes (only used for basslines)
            u8 decay = (decayA != NULL && accentReq) ? *decayA : envPatch->MINIMAL.decay;
            u8 curve = envMode.MINIMAL.CURVE_D ? envPatch->MINIMAL.curve : 0x80;
            if( step(envPatch->MINIMAL.sustain << 8, decay, curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_SUSTAIN;
        } break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envPatch->MINIMAL.sustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE1: {
            u8 curve = envMode.MINIMAL.CURVE_R ? envPatch->MINIMAL.curve : 0x80;
            if( envCtr )
                step(0x0000, envPatch->MINIMAL.release, curve, updateSpeedFactor);
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
    if( modDstPitch )
        *modDstPitch += ((envCtr/2) * depth_p) / 128;

    s32 depth_pw = (s32)envPatch->MINIMAL.depth_pw - 0x80;
    if( depth_pw && accentReq ) {
        if( depth_pw >= 0 ) {
            if( (depth_pw += *accent) > 128 ) depth_pw = 128;
        } else {
            if( (depth_pw -= *accent) < -128 ) depth_pw = -128;
        }
    }
    if( modDstPw )
        *modDstPw += ((envCtr/2) * depth_pw) / 128;

    s32 depth_f = (s32)envPatch->MINIMAL.depth_f - 0x80;
    if( depth_f && accentReq ) {
        if( depth_f >= 0 ) {
            if( (depth_f += *accent) > 128 ) depth_f = 128;
        } else {
            if( (depth_f -= *accent) < -128 ) depth_f = -128;
        }
    }
    if( modDstFilter )
        *modDstFilter += ((envCtr/2) * depth_f) / 128;

    accentReq = false;

    return sustainPhase;
}


// Extended version for Lead engine
bool MbSidEnv::tickLead(const sid_se_engine_t &engine, const u8 &updateSpeedFactor)
{
    bool sustainPhase = false; // will be the return value

    sid_se_env_mode_t envMode;
    envMode.ALL = envPatch->mode;

    if( restartReq ) {
        restartReq = false;
        envState = MBSID_ENV_STATE_ATTACK1;
        envDelayCtr = envPatch->L.delay ? 1 : 0;
    }

    if( releaseReq ) {
        releaseReq = false;
        envState = MBSID_ENV_STATE_RELEASE1;
    }

    // if clock sync enabled: only increment on each 16th clock event
    if( envMode.L.CLKSYNC && !syncClockReq ) {
        if( envState == MBSID_ENV_STATE_IDLE )
            return false; // nothing to do
    } else {
        syncClockReq = false;

        switch( envState ) {
        case MBSID_ENV_STATE_ATTACK1:
            if( envDelayCtr ) {
                int new_delay_ctr = envDelayCtr + (mbSidEnvTable[envPatch->L.delay] / updateSpeedFactor);
                if( new_delay_ctr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }
  
            if( step(envPatch->L.attlvl << 8, envPatch->L.attack1, envPatch->L.att_curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_ATTACK2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_ATTACK2:
            if( step(0xffff, envPatch->L.attack2, envPatch->L.att_curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY1; // TODO: Set Phase depending on mode
            break;

        case MBSID_ENV_STATE_DECAY1:
            if( step(envPatch->L.declvl << 8, envPatch->L.decay1, envPatch->L.dec_curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_DECAY2:
            if( step(envPatch->L.sustain << 8, envPatch->L.decay2, envPatch->L.dec_curve, updateSpeedFactor) ) {
                envState = MBSID_ENV_STATE_SUSTAIN; // TODO: Set Phase depending on mode
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
            break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envPatch->L.sustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE1:
            if( step(envPatch->L.rellvl << 8, envPatch->L.release1, envPatch->L.rel_curve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_RELEASE2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_RELEASE2:
            if( envCtr )
                step(0x0000, envPatch->L.release2, envPatch->L.rel_curve, updateSpeedFactor);
            break;
  
        default: // like MBSID_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    // scale to ENV depth
    s32 env_depth = (s32)envPatch->L.depth - 0x80;
    
    // final ENV value (range +/- 0x7fff)
    if( modSrcEnv )
        *modSrcEnv = ((envCtr/2) * env_depth) / 128;

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

