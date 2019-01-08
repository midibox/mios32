/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Wavetable Sequencer
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
#include "MbSidWt.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidWt::MbSidWt()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidWt::~MbSidWt()
{
}



/////////////////////////////////////////////////////////////////////////////
// WT init function
/////////////////////////////////////////////////////////////////////////////
void MbSidWt::init(void)
{
    // clear flags
    restartReq = false;
    clockReq = false;

    // clear variables
    wtBegin = 0;
    wtEnd = 0;
    wtLoop = 0;
    wtSpeed = 0;
    wtAssign = 0;
    wtAssignLeftRight = 0;
    wtOneshotMode = 0;
    wtKeyControlMode = 0;
    wtModControlMode = 0;

    wtOut = -1;

    wtPos = 0;
    wtDivCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidWt::tick(s32 step, const u8 &updateSpeedFactor)
{
    bool restarted = 0;

    if( step >= 0 ) {
        // use modulated step position
        // scale between begin/end range

        if( wtEnd > wtBegin ) {
            s32 range = wtEnd - wtBegin + 1;
            step = wtBegin + ((step * range) / 128);
        } else {
            // should we invert the waveform?
            s32 range = wtBegin - wtEnd + 1;
            step = wtEnd + ((step * range) / 128);
        }
    } else {
        // don't use modulated position - normal mode
        bool nextStepReq = 0;

        // check if WT reset requested
        if( restartReq ) {
            restartReq = 0;
            restarted = 1;

            // next clock will increment div to 0
            wtDivCtr = ~0;
            // next step will increment to start position
            wtPos = wtBegin - 1;
        }

        // check for WT clock event
        if( clockReq ) {
            clockReq = 0;
            // increment clock divider
            // reset divider if it already has reached the target value
            if( ++wtDivCtr == 0 || (wtDivCtr > wtSpeed) ) {
                wtDivCtr = 0;
                nextStepReq = 1;
            }
        }

        // check for next step request
        // skip if position is 0xaa (notifies oneshot -> WT stopped)
        if( nextStepReq && wtPos != 0xaa ) {
            // increment position counter, reset at end position
            if( ++wtPos > wtEnd ) {
                restarted = 1;
                // if oneshot mode: set position to 0xaa, WT is stopped now!
                if( wtOneshotMode )
                    wtPos = 0xaa;
                else
                    wtPos = wtLoop;
            }
            step = wtPos; // step is positive now -> will be played
        }
    }

    wtOut = step;

    return restarted;
}
