/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Base Class for Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvEnvBase.h"
#include "MbCvTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvBase::MbCvEnvBase()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvEnvBase::~MbCvEnvBase()
{
}



/////////////////////////////////////////////////////////////////////////////
// ENV init function
/////////////////////////////////////////////////////////////////////////////
void MbCvEnvBase::init(void)
{
    // clear flags
    restartReq = false;
    releaseReq = false;
    syncClockReq = false;

    // clear variables
    envModeClkSync = 0;
    envModeKeySync = 1;
    envModeOneshot = 1;
    envModeFast = 0;

    envAmplitude = 0;
    envCurve = 0;

    envDepthPitch = 64;
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
bool MbCvEnvBase::tick(const u8 &updateSpeedFactor)
{
    envOut = 0;

    return false;
}



bool MbCvEnvBase::step(const u16 &target, const u16 &rate, const s8 &curve, const u8 &updateSpeedFactor, const bool& rateFromEnvTable)
{
    if( target == envCtr )
        return true; // next state

    // modify rate if curve != 0x80
    u16 inc_rate;
    if( curve ) {
        // this nice trick has been proposed by Razmo
        int abs_curve = (curve < 0) ? -curve : (curve ^ 0x7f);
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
