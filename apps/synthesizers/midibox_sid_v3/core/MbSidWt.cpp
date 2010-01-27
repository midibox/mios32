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
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidWt::MbSidWt()
{
    init(NULL, 0, NULL);
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
void MbSidWt::init(sid_se_wt_patch_t *_wtPatch, u8 _wtNum, MbSidPar *_mbSidParPtr)
{
    wtPatch = _wtPatch;
    wtNum = _wtNum;
    mbSidParPtr = _mbSidParPtr;

    // clear flags
    restartReq = false;
    clockReq = false;

    // clear references
    modSrcWt = NULL;
    modDstWt = NULL;
    wtMemory = NULL;

    // clear variables
    wtDrumSpeed = 0;
    wtDrumPar = 0;
    wtPos = 0;
    wtDivCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidWt::tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor)
{
    bool restarted = false;
    s32 step = -1;

    if( !wtPatch ) // exit if no patch reference initialized
        return false;

    // if key control flag (END[7]) set, control position from current played key
    if( wtPatch->end & (1 << 7) ) {
        // copy currently played note to step position
        //TODO step = sid_se_voice[0].played_note;

        // if MOD control flag (BEGIN[7]) set, control step position from modulation matrix
    } else if( modSrcWt && wtPatch->begin & (1 << 7) ) {
        step = ((s32)*modDstWt + 0x8000) >> 9; // 16bit signed -> 7bit unsigned
    }

    if( step >= 0 ) {
        // use modulated step position
        // scale between begin/end range
        u8 rangeBegin = wtPatch->begin & 0x7f;
        u8 rangeEnd = wtPatch->end & 0x7f;

        if( rangeEnd > rangeBegin ) {
            s32 range = rangeEnd - rangeBegin + 1;
            step = rangeBegin + ((step * range) / 128);
        } else {
            // should we invert the waveform?
            s32 range = rangeBegin - rangeEnd + 1;
            step = rangeEnd + ((step * range) / 128);
        }
    } else {
        // don't use modulated position - normal mode
        bool nextStepReq = false;

        // check if WT reset requested
        if( restartReq ) {
            restartReq = false;
            restarted = true;

            // next clock will increment div to 0
            wtDivCtr = ~0;
            // next step will increment to start position
            wtPos = (wtPatch->begin & 0x7f) - 1;
        }

        // check for WT clock event
        if( clockReq ) {
            clockReq = false;
            // increment clock divider
            // reset divider if it already has reached the target value
            if( ++wtDivCtr == 0 || (wtDivCtr > (wtPatch->speed & 0x3f)) ) {
                wtDivCtr = 0;
                nextStepReq = true;
            }
        }

        // check for next step request
        // skip if position is 0xaa (notifies oneshot -> WT stopped)
        if( nextStepReq && wtPos != 0xaa ) {
            // increment position counter, reset at end position
            if( ++wtPos > (wtPatch->end & 0x7f) ) {
                restarted = true;
                // if oneshot mode: set position to 0xaa, WT is stopped now!
                if( wtPatch->loop & (1 << 7) )
                    wtPos = 0xaa;
                else
                    wtPos = wtPatch->loop & 0x7f;
            }
            step = wtPos; // step is positive now -> will be played
        }
    }

    // check if step should be played
    if( step >= 0 && wtMemory ) {
        u8 wtValue = wtMemory[step & 0x7f];

        if( modSrcWt ) {
            // forward to mod matrix
            if( wtValue < 0x80 ) {
                // relative value -> convert to -0x8000..+0x7fff
                *modSrcWt = ((s32)wtValue - 0x40) * 512;
            } else {
                // absolute value -> convert to 0x0000..+0x7f00
                *modSrcWt = ((s32)wtValue & 0x7f) * 256;
            }
        }
    
        // call parameter handler
        if( wtPatch->assign ) {
            // determine SID channels which should be modified
            u8 sidlr = (wtPatch->speed >> 6); // SID selection
            u8 ins = wtNum;

#if DEBUG_VERBOSE_LEVEL >= 2
            DEBUG_MSG("WT %d: 0x%02x 0x%02x\n", wtNum, step, wtValue);
#endif
            if( mbSidParPtr )
                mbSidParPtr->parSetWT(wtPatch->assign, wtValue, sidlr, ins);
        }
    }

    return restarted;
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable handler from drum models
/////////////////////////////////////////////////////////////////////////////
bool MbSidWt::tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor,
                   sid_drum_model_t *drumModel, u8 &voiceNote, u8 &voiceWaveform)
{
    bool restarted = false;
    bool nextStepReq = false;

    // exit if no drum model selected
    if( !drumModel )
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
    if( ++wtDivCtr > (wtDrumSpeed * updateSpeedFactor) ) {
        wtDivCtr = 0;
        nextStepReq = true;
    }

    // check for next step request
    // skip if position is 0xaa (notifies oneshot -> WT stopped)
    if( nextStepReq && wtPos != 0xaa ) {
        // increment position counter, reset at end position
        ++wtPos;
        if( drumModel->wavetable[2*wtPos] == 0 ) {
            if( drumModel->wt_loop == 0xff )
                wtPos = 0xaa; // oneshot mode
            else
                wtPos = drumModel->wt_loop;
        }

        if( wtPos != 0xaa ) {
            // "play" the step
            int note = drumModel->wavetable[2*wtPos + 0];
            // transfer to voice
            // if bit #7 of note entry is set: add PAR3/2 and saturate
            if( note & (1 << 7) ) {
                note = (note & 0x7f) + (((int)wtDrumPar - 0x80) / 2);
                if( note > 127 ) note = 127; else if( note < 0 ) note = 0;
            }
            voiceNote = note;

            // set waveform
            voiceWaveform = drumModel->wavetable[2*wtPos + 1];

#if DEBUG_VERBOSE_LEVEL >= 3
            DEBUG_MSG("WT %d: %d (%02x %02x)\n", wtNum, wtPos, voiceNote, voiceWaveform);
#endif
        }
    }

    return restarted;
}
