/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Sequencer
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
#include "MbSidSeqDrum.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeqDrum::MbSidSeqDrum()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeqDrum::~MbSidSeqDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer init function
/////////////////////////////////////////////////////////////////////////////
void MbSidSeqDrum::init(void)
{
    MbSidSeq::init();
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer handler
/////////////////////////////////////////////////////////////////////////////
void MbSidSeqDrum::tick(MbSidSe *se)
{
    // always bypass the notestack when triggering instruments
    bool bypassNotestack = true;

    // clear gate and deselect sequence if MIDI clock stop requested
    if( seqStopReq ) {
        seqStopReq = 0;
        seqRunning = 0;
        // clear gates
        se->noteAllOff(0, bypassNotestack);
        return; // nothing else to do
    }

    // exit if sequencer not enabled
    // clear gate if enable flag just has been disabled (for proper transitions)
    if( !seqEnabled ) {
        if( seqEnabledSaved ) {
            // clear gates
            se->noteAllOff(0, bypassNotestack);
        }
        seqEnabledSaved = false;
        return; // nothing else to do
    }

    // check if reset requested or sequencer was disabled before (transition Seq off->on)
    if( !seqEnabled || seqRestartReq ) {
        // clear request
        seqRestartReq = 0;

        // sequencer enabled and running
        seqEnabled = 1;
        seqRunning = 1;

        // next clock event will increment to 0
        seqDivCtr = ~0;
        seqSubCtr = ~0;
        // next step will increment to start position
        seqPos = (seqPatternNum << 4) - 1;
    }


    // check for clock sync event
    if( seqRunning && seqClockReq ) {
        seqClockReq = 0;

        // increment clock divider
        // reset divider if it already has reached the target value
        if( ++seqDivCtr == 0 || seqDivCtr > seqClockDivider ) {
            seqDivCtr = 0;

            // increment subcounter and check for state
            // 0: new note & gate set
            // 4: gate clear
            // >= 6: reset to 0, new note & gate set
            if( ++seqSubCtr >= 6 )
                seqSubCtr = 0;

            if( seqSubCtr == 0 ) { // set gate
                // increment position counter, reset at end position
                u8 nextStep = (seqPos & 0x0f) + 1;
                if( nextStep > seqPatternLength )
                    nextStep = 0;
                else
                    nextStep &= 0x0f; // just to ensure...

                // change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
                u8 nextPattern = seqPos >> 4;
                if( !seqSynchToMeasure || nextStep == 0 )
                    nextPattern = seqPatternNum;

                seqPos = (nextPattern << 4) | nextStep;

                // play the step

                // gate off if invalid song number (stop feature: seq >= 8)
                if( seqPatternNum >= 8 ) {
                    // clear gates
                    se->noteAllOff(0, bypassNotestack);
                } else {
                    // Sequence Storage - Structure:
                    // 4 bytes for 16 steps:
                    //  - first byte: [0] gate step #1 ... [7] gate step #8
                    //  - second byte: [0] accent step #1 ... [7] accent step #8
                    //  - third byte: [0] gate step #9 ... [7] gate step #16
                    //  - fourth byte: [0] accent step #9 ... [7] accent step #16
                    // 
                    // 8 tracks per sequence:
                    //  offset 0x00-0x03: track #1
                    //  offset 0x04-0x07: track #2
                    //  offset 0x08-0x0b: track #3
                    //  offset 0x0c-0x0f: track #4
                    //  offset 0x00-0x03: track #5
                    //  offset 0x04-0x07: track #6
                    //  offset 0x08-0x0b: track #7
                    //  offset 0x0c-0x0f: track #8
                    // 8 sequences:
                    //  0x100..0x11f: sequence #1
                    //  0x120..0x13f: sequence #2
                    //  0x140..0x15f: sequence #3
                    //  0x160..0x17f: sequence #4
                    //  0x180..0x19f: sequence #5
                    //  0x1a0..0x1bf: sequence #6
                    //  0x1c0..0x1df: sequence #7
                    //  0x1e0..0x1ff: sequence #8
                    u8 *pattern = (u8 *)&seqPatternMemory[(seqPos & 0xf0) << 1];

                    // loop through 8 tracks
                    u8 step = seqPos & 0x0f;
                    u8 stepOffset = (step & (1 << 3)) ? 2 : 0;
                    u8 stepMask = (1 << (step & 0x7));
                    for(int track=0; track<8; ++track) {
                        u8 mode = 0;
                        if( pattern[4*track + stepOffset + 0] & stepMask )
                            mode |= 1;
                        if( pattern[4*track + stepOffset + 1] & stepMask )
                            mode |= 2;

                        // coding:
                        // Gate  Accent  Result
                        //    0       0  Don't play note
                        //    1       0  Play Note w/o accent
                        //    0       1  Play Note w/ accent
                        //    1       1  Play Secondary Note
                        u8 drum = track*2;
                        u8 gate = 0;
                        u8 velocity = 0x3f; // >= 0x40 selects accent
                        switch( mode ) {
                        case 1: gate = 1; break;
                        case 2: gate = 1; velocity = 0x7f; break;
                        case 3: gate = 1; ++drum; break;
                        }

                        se->noteOn(0, drum, velocity, bypassNotestack);
                    }
                }
            } else if( seqSubCtr == 4 ) { // clear gates
                se->noteAllOff(0, bypassNotestack);
            }
        }
    }
}
