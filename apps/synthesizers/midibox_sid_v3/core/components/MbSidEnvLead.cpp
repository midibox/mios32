/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Envelope Generator for Lead Engine
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
#include "MbSidEnvLead.h"
#include "MbSidTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvLead::MbSidEnvLead()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvLead::~MbSidEnvLead()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvLead::init(void)
{
    MbSidEnv::init();

    // clear additional variables
    envDepth = 0;
    envAttackLevel = 0;
    envAttack2 = 0;
    envDecayLevel = 0;
    envDecay2 = 0;
    envReleaseLevel = 0;
    envRelease2 = 0;
    envAttackCurve = 0;
    envDecayCurve = 0;
    envReleaseCurve = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Envelope handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvLead::tick(const u8 &updateSpeedFactor)
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
        case MBSID_ENV_STATE_ATTACK:
            if( envDelayCtr ) {
                int new_delay_ctr = envDelayCtr + (mbSidEnvTable[envDelay] / updateSpeedFactor);
                if( new_delay_ctr > 0xffff )
                    envDelayCtr = 0; // delay passed
                else {
                    envDelayCtr = new_delay_ctr; // delay not passed
                    return false; // no error
                }
            }
  
            if( step(envAttackLevel << 8, envAttack, envAttackCurve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_ATTACK2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_ATTACK2:
            if( step(0xffff, envAttack2, envAttackCurve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY; // TODO: Set Phase depending on mode
            break;

        case MBSID_ENV_STATE_DECAY:
            if( step(envDecayLevel << 8, envDecay, envDecayCurve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_DECAY2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_DECAY2:
            if( step(envSustain << 8, envDecay2, envDecayCurve, updateSpeedFactor) ) {
                envState = MBSID_ENV_STATE_SUSTAIN; // TODO: Set Phase depending on mode
  
                // propagate sustain phase to trigger matrix
                sustainPhase = true;
            }
            break;
  
        case MBSID_ENV_STATE_SUSTAIN:
            // always update sustain level
            envCtr = envSustain << 8;
            break;
  
        case MBSID_ENV_STATE_RELEASE:
            if( step(envReleaseLevel << 8, envRelease, envReleaseCurve, updateSpeedFactor) )
                envState = MBSID_ENV_STATE_RELEASE2; // TODO: Set Phase depending on mode
            break;
  
        case MBSID_ENV_STATE_RELEASE2:
            if( envCtr )
                step(0x0000, envRelease2, envReleaseCurve, updateSpeedFactor);
            break;
  
        default: // like MBSID_ENV_STATE_IDLE
            return false; // nothing to do...
        }
    }

    envOut = envCtr / 2;

    accentReq = false;

    return sustainPhase;
}
