/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Bassline Sequencer
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
#include "MbCvSeqBassline.h"
#include "MbCv.h"

#define TRIGGER_CV2_AND_CV3 1
#if TRIGGER_CV2_AND_CV3
#include <app.h>
#include "MbCvEnvironment.h"
#endif


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvSeqBassline::MbCvSeqBassline()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvSeqBassline::~MbCvSeqBassline()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer init function
/////////////////////////////////////////////////////////////////////////////
void MbCvSeqBassline::init(void)
{
    MbCvSeq::init();

    for(int pattern=0; pattern<MBCV_SEQ_BASSLINE_NUM_PATTERNS; ++pattern)
        for(int step=0; step<MBCV_SEQ_BASSLINE_NUM_STEPS; ++step) {
            seqBasslineKey[pattern][step] = 36; // C-2
            seqBasslineArgs[pattern][step].ALL = 0;
            seqBasslineArgs[pattern][step].gate = 1;
        }

    seqGateLength = 75;
    seqEnvMod = 128;
    seqAccent = 128;
    seqAccentEffective = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer handler
/////////////////////////////////////////////////////////////////////////////
void MbCvSeqBassline::tick(MbCvVoice *v, MbCv *cv)
{
    u8 seqClockDivider = 3;

    // always bypass the notestack when triggering instruments
    //bool bypassNotestack = true;

    // clear gate and deselect sequence if MIDI clock stop requested
    if( seqStopReq ) {
        seqStopReq = 0;
        seqRunning = 0;
        v->voiceGateClrReq = 1;
        v->voiceGateSetReq = 0;
        cv->triggerNoteOff(v); // propagate to trigger matrix
        return; // nothing else to do
    }

    // exit if sequencer not enabled
    // clear gate if enable flag just has been disabled (for proper transitions)
    if( !seqEnabled ) {
        if( seqEnabledSaved ) {
            v->voiceGateClrReq = 1;
            v->voiceGateSetReq = 0;
            cv->triggerNoteOff(v); // propagate to trigger matrix
        }
        seqEnabledSaved = 0;
        return; // nothing else to do
    }
    seqEnabledSaved = 1;

    // check if reset requested or sequencer was disabled before (transition Seq off->on)
    if( !seqEnabled || seqRestartReq ) {
        if( seqRestartReq )
            v->voiceActive = 1; // enable voice (again)

        // clear request
        seqRestartReq = 0;

        // sequencer enabled and running
        seqEnabled = 1;
        seqRunning = 1;

        // next clock event will increment to 0
        seqDivCtr = ~0;
        seqSubCtr = ~0;
        // next step will increment to start position
        seqPos = ~0;
        seqCurrentPattern = seqPatternNumber;
        // ensure that slide flag is cleared
        v->voiceSlideActive = 0;
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
                seqPos = (seqPos & 0x1f) + 1;
                if( seqPos > seqPatternLength )
                    seqPos = 0;
                else
                    seqPos %= MBCV_SEQ_BASSLINE_NUM_STEPS; // just to ensure...

                // change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
                if( !seqSynchToMeasure || seqPos == 0 )
                    seqCurrentPattern = seqPatternNumber % MBCV_SEQ_BASSLINE_NUM_PATTERNS; // just to ensure...

                // play the step

                // gate off (without slide) if invalid song number (stop feature: seq >= 8)
                if( seqPatternNumber >= 8 ) {
                    if( v->voiceGateActive ) {
                        v->voiceGateClrReq = 1;
                        v->voiceGateSetReq = 0;
                        cv->triggerNoteOff(v); // propagate to trigger matrix
                    }
                } else {
                    // get note/par value
                    u8 keyItem = seqBasslineKey[seqCurrentPattern][seqPos];
                    BasslineArgsT argItem;
                    argItem.ALL = seqBasslineArgs[seqCurrentPattern][seqPos].ALL;

#if 0
                    DEBUG_MSG("SEQ %d@%d/%d: 0x%02x 0x%02x\n", 0, seqCurrentPattern, seqPos, keyItem, argItem);
#endif

                    // transfer note to voice
                    v->voiceNote = keyItem;

                    // set accent
                    // ignore if slide has been set by previous step
                    // (important for SID sustain: transition from sustain < 0xf to 0xf will reset the VCA)
                    if( !v->voiceSlideActive ) {
                        // take over accent
                        v->voiceAccentActive = argItem.accent;

                        // for MOD matrix
                        seqAccentEffective = argItem.accent ? seqAccent : 0;
                    }

                    // activate portamento if slide has been set by previous step
                    v->voicePortamentoActive = v->voiceSlideActive;

                    // set slide flag of current flag
                    v->voiceSlideActive = argItem.glide;

                    // set gate if flag is set
                    if( argItem.gate && v->voiceActive ) {
                        if( !v->voiceGateActive ) {
                            v->voiceGateClrReq = 0;
                            v->voiceGateSetReq = 1;
                            cv->triggerNoteOn(v); // propagate to trigger matrix
#if TRIGGER_CV2_AND_CV3
                            {
                                // TMP as long as trigger matrix isn't available
                                MbCvEnvironment* env = APP_GetEnv();
                                if( env ) {
                                    env->mbCv[1].triggerNoteOn(v);
                                    env->mbCv[2].triggerNoteOn(v);
                                }
                            }
#endif
                            v->voiceNoteRestartReq = false; // clear note restart request which has been set by trigger function - gate already set!
                        }
                    }

                }
            } else if( seqSubCtr == 4 ) { // clear gate
                // don't clear if slide flag is set!
                if( !v->voiceSlideActive ) {
                    v->voiceGateClrReq = 1;
                    v->voiceGateSetReq = 0;
                    cv->triggerNoteOff(v); // propagate to trigger matrix
#if TRIGGER_CV2_AND_CV3
                    {
                        // TMP as long as trigger matrix isn't available
                        MbCvEnvironment* env = APP_GetEnv();
                        if( env ) {
                            env->mbCv[1].triggerNoteOff(v);
                            env->mbCv[2].triggerNoteOff(v);
                        }
                    }
#endif
                }
            }
        }
    }
}
