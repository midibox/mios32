/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Wavetable Sequencer for Drums
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
#include "MbSidWtDrum.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidWtDrum::MbSidWtDrum()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidWtDrum::~MbSidWtDrum()
{
}



/////////////////////////////////////////////////////////////////////////////
// WT init function
/////////////////////////////////////////////////////////////////////////////
void MbSidWtDrum::init(void)
{
    MbSidWt::init();
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable handler from drum models
/////////////////////////////////////////////////////////////////////////////
bool MbSidWtDrum::tick(const u8 &updateSpeedFactor, MbSidVoiceDrum *v)
{
    bool restarted = false;
    bool nextStepReq = false;

    // exit if no drum assigned
    if( v->mbSidDrumPtr == NULL )
        return false;

    // check if WT reset requested
    if( restartReq ) {
        restartReq = false;
        // next clock will increment div to 0
        wtDivCtr = ~0;
        // next step will increment to start position
        wtPos = ~0;
    }

    // clock with each update cycle, so that we are independent from the selected BPM rate
    // increment clock divider
    // reset divider if it already has reached the target value
    if( ++wtDivCtr > (v->mbSidDrumPtr->getSpeed() * updateSpeedFactor) ) {
        wtDivCtr = 0;
        nextStepReq = true;
    }

    // check for next step request
    // skip if position is 0xaa (notifies oneshot -> WT stopped)
    if( nextStepReq && wtPos != 0xaa ) {
        // increment position counter, reset at end position
        ++wtPos;

        u8 drumNote = v->mbSidDrumPtr->getNote(wtPos);
        if( drumNote == 0 ) {
            u8 drumLoop = v->mbSidDrumPtr->getLoop();
            if( drumLoop == 0xff )
                wtPos = 0xaa; // oneshot mode
            else
                wtPos = drumLoop;
        }

        if( wtPos != 0xaa ) {
            // set note
            v->voiceNote = drumNote;

            // set waveform
            sid_se_voice_waveform_t waveform;
            waveform.ALL = v->mbSidDrumPtr->getWaveform(wtPos);
            v->voiceWaveformOff = waveform.VOICE_OFF;
            v->voiceWaveformSync = waveform.SYNC;
            v->voiceWaveformRingmod = waveform.RINGMOD;
            v->voiceWaveform = waveform.WAVEFORM;

#if 0
            DEBUG_MSG("WT %d: %d (%02x %02x)\n", v->voiceNum, wtPos, drumNote, waveform.ALL);
#endif
        }
    }

    return restarted;
}
