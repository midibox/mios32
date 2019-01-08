/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Bassline Sequencer
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
#include "MbSidSeqBassline.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeqBassline::MbSidSeqBassline()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeqBassline::~MbSidSeqBassline()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer init function
/////////////////////////////////////////////////////////////////////////////
void MbSidSeqBassline::init(void)
{
    MbSidSeq::init();
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer handler
/////////////////////////////////////////////////////////////////////////////
void MbSidSeqBassline::tick(MbSidVoice *v, MbSidSe *se)
{
    // always bypass the notestack when triggering instruments
    //bool bypassNotestack = true;

    // clear gate and deselect sequence if MIDI clock stop requested
    if( seqStopReq ) {
        seqStopReq = 0;
        seqRunning = 0;
        v->voiceGateClrReq = 1;
        v->voiceGateSetReq = 0;
        se->triggerNoteOff(v, 0); // propagate to trigger matrix
        return; // nothing else to do
    }

    // exit if sequencer not enabled
    // clear gate if enable flag just has been disabled (for proper transitions)
    if( !seqEnabled ) {
        if( seqEnabledSaved ) {
            v->voiceGateClrReq = 1;
            v->voiceGateSetReq = 0;
            se->triggerNoteOff(v, 0); // propagate to trigger matrix
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
        seqPos = (seqPatternNumber << 4) - 1;
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
                u8 nextStep = (seqPos & 0x0f) + 1;
                if( nextStep > seqPatternLength )
                    nextStep = 0;
                else
                    nextStep &= 0x0f; // just to ensure...

                // change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
                u8 nextPattern = seqPos >> 4;
                if( !seqSynchToMeasure || nextStep == 0 )
                    nextPattern = seqPatternNumber;

                seqPos = (nextPattern << 4) | nextStep;

                // play the step

                // gate off (without slide) if invalid song number (stop feature: seq >= 8)
                if( seqPatternNumber >= 8 ) {
                    if( v->voiceGateActive ) {
                        v->voiceGateClrReq = 1;
                        v->voiceGateSetReq = 0;
                        se->triggerNoteOff(v, 0); // propagate to trigger matrix
                    }
                } else {
                    // Sequence Storage - Structure:
                    //   2 bytes for each step (selected with address bit #7)
                    //   lower byte: [3:0] note, [5:4] octave, [6] glide, [7] gate
                    //   upper byte: [6:0] parameter value, [7] accent
                    // 16 Steps per sequence (offset 0x00..0x0f)
                    // 8 sequences:
                    //  0x100..0x10f/0x180..0x18f: sequence #1
                    //  0x110..0x11f/0x190..0x19f: sequence #2
                    //  0x120..0x12f/0x1a0..0x1af: sequence #3
                    //  0x130..0x13f/0x1b0..0x1bf: sequence #4
                    //  0x140..0x14f/0x1c0..0x1cf: sequence #5
                    //  0x150..0x15f/0x1d0..0x1df: sequence #6
                    //  0x160..0x16f/0x1e0..0x1ef: sequence #7
                    //  0x170..0x17f/0x1f0..0x1ff: sequence #8

                    // get note/par value
                    sid_se_seq_note_item_t noteItem;
                    noteItem.ALL = seqPatternMemory[seqPos];
                    sid_se_seq_asg_item_t asgItem;
                    asgItem.ALL = seqPatternMemory[seqPos + 0x80];

#if 0
                    DEBUG_MSG("SEQ %d@%d/%d: 0x%02x 0x%02x\n", 0, seqPos >> 4, seqPos & 0xf, noteItem.ALL, asgItem.ALL);
#endif

                    // determine note
                    u8 note = noteItem.NOTE + 0x3c;

                    // add octave
                    switch( noteItem.OCTAVE ) {
                    case 1: note += 12; break; // Up
                    case 2: note -= 12; break; // Down
                    case 3: note += 24; break; // Up2
                    }

                    // transfer to voice
                    v->voiceNote = note;

                    // set accent
                    // ignore if slide has been set by previous step
                    // (important for SID sustain: transition from sustain < 0xf to 0xf will reset the VCA)
                    if( !v->voiceSlideActive ) {
                        // take over accent
                        v->voiceAccentActive = asgItem.ACCENT;
                    }

                    // activate portamento if slide has been set by previous step
                    v->voicePortamentoActive = v->voiceSlideActive;

                    // set slide flag of current flag
                    v->voiceSlideActive = noteItem.SLIDE;

                    // set gate if flag is set
                    if( noteItem.GATE && v->voiceActive ) {
                        if( !v->voiceGateActive ) {
                            v->voiceGateClrReq = 0;
                            v->voiceGateSetReq = 1;
                            se->triggerNoteOn(v, 0); // propagate to trigger matrix
                            v->voiceNoteRestartReq = false; // clear note restart request which has been set by trigger function - gate already set!
                        }
                    }

                    // parameter track:
                    // determine SID channels which should be modified
                    if( seqParameterAssign ) {
                        u8 sidlr = (v->voiceNum >= 3) ? 2 : 1; // SIDL/R selection
                        u8 ins = 0;
                        se->parSetWT(seqParameterAssign, asgItem.PAR_VALUE + 0x80, sidlr, ins);
                    }
                }
            } else if( seqSubCtr == 4 ) { // clear gate
                // don't clear if slide flag is set!
                if( !v->voiceSlideActive ) {
                    v->voiceGateClrReq = 1;
                    v->voiceGateSetReq = 0;
                    se->triggerNoteOff(v, 0); // propagate to trigger matrix
                }
            }
        }
    }
}
