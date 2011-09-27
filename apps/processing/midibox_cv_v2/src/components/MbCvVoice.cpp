/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Voice Handlers
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
#include "MbCvVoice.h"
#include "MbCvMidiVoice.h"
#include "MbCvTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvVoice::MbCvVoice()
{
    init(0, NULL); // assigned by MbCv!
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvVoice::~MbCvVoice()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function with voice and register assignments
// (usually only called once after startup)
/////////////////////////////////////////////////////////////////////////////
void MbCvVoice::init(u8 _voiceNum, MbCvMidiVoice* _midiVoicePtr)
{
    voiceNum = _voiceNum;
    midiVoicePtr = _midiVoicePtr;

    this->init();
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbCvVoice::init(void)
{
    voiceEventMode = MBCV_MIDI_EVENT_MODE_NOTE;
    voiceLegato = 0;
    voiceSusKey = 0;
    voiceSequencerOnly = 0;
    voicePoly = 0;
    voiceConstantTimeGlide = 0;
    voiceGlissandoMode = 0;
    voiceAccentRate = 0;
    voiceTransposeOctave = 0;
    voiceTransposeSemitone = 0;
    voiceFinetune = 0;
    voicePitchrange = 2;
    voicePortamentoRate = 0;

    voicePitchModulation = 0;

    voiceActive = 0;
    voiceDisabled = 0;
    voiceGateActive = 0;
    voicePhysGateActive = 0;
    voiceGateSetReq = 0;
    voiceGateClrReq = 0;
    voicePortamentoActive = 0;
    voicePortamentoInitialized = 0;
    voiceAccentActive = 0;
    voiceSlideActive = 0;
    voiceForceFrqRecalc = 0;
    voiceNoteRestartReq = 0;

    voiceNote = 0;
    voiceForcedNote = 0;
    voicePlayedNote = 0;
    voicePrevTransposedNote = 0;
    voiceGlissandoNote = 0;
    voicePortamentoBeginFrq = 0;
    voicePortamentoEndFrq = 0;
    voicePortamentoCtr = 0;
    voiceLinearFrq = 0;
    voiceFrq = 0;
    voiceRetriggerDelay = 0;
    voiceSetDelayCtr = 0;

    voiceAssignedInstrument = ~0; // invalid instrument
}


/////////////////////////////////////////////////////////////////////////////
// Voice Gate
// returns 1 if pitch should be changed
/////////////////////////////////////////////////////////////////////////////
bool MbCvVoice::gate(const u8 &updateSpeedFactor)
{
    bool changePitch = 1;

    // restart request?
    if( voiceNoteRestartReq ) {
        voiceNoteRestartReq = 0;

        // request gate if voice is active (and request clear for proper ADSR handling)
        voiceGateClrReq = 1;
        if( voiceActive )
            voiceGateSetReq = 1;

        // delay voice by some ticks (at least one)
        voiceSetDelayCtr = voiceRetriggerDelay ? voiceRetriggerDelay : 1;
    }

    // if gate not active: ignore clear request
    if( !voiceGateActive )
        voiceGateClrReq = 0;

    // gate set/clear request?
    if( voiceGateClrReq ) {
        voiceGateClrReq = 0;

        // clear physical gate flag (set is delayed)
        voicePhysGateActive = 0;

        // gate not active anymore
        voiceGateActive = 0;
    } else if( voiceGateSetReq ) {
        // delay note so long 16bit delay counter != 0
        if( voiceSetDelayCtr )
            --voiceSetDelayCtr;

        if( !voiceSetDelayCtr ) {
            // acknowledge the set request
            voiceGateSetReq = 0;

            // set the physical gate flag
            voicePhysGateActive = 1;
        }

        voiceGateActive = 1;
    }

    return changePitch;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch
/////////////////////////////////////////////////////////////////////////////
void MbCvVoice::pitch(const u8 &updateSpeedFactor)
{
    // transpose MIDI note
    int transposedNote = voiceForcedNote;

    if( !transposedNote ) {
        transposedNote = voiceNote;
        transposedNote += 12*voiceTransposeOctave + voiceTransposeSemitone;
        transposedNote += (int)midiVoicePtr->midivoiceTranspose - 64;

        // octave-wise saturation
        while( transposedNote < 0 )
            transposedNote += 12;
        while( transposedNote >= 128 )
            transposedNote -= 12;
    }

    // Glissando handling
    if( transposedNote != voicePrevTransposedNote ) {
        voicePrevTransposedNote = transposedNote;
        if( voiceGlissandoMode ) // Glissando active?
            voicePortamentoCtr = 0xffff; // force overrun
    }

    if( voicePortamentoActive && voicePortamentoRate && voiceGlissandoMode ) {
        // increment portamento counter, amount derived from envelope table  .. make it faster
        int portamentoCtr = voicePortamentoCtr + mbCvEnvTable[voicePortamentoRate >> 1] / updateSpeedFactor;
        // next note step?
        if( portamentoCtr >= 0xffff ) {
            // reset counter
            voicePortamentoCtr = 0;

            // increment/decrement note
            if( transposedNote > voiceGlissandoNote )
                ++voiceGlissandoNote;
            else if( transposedNote < voiceGlissandoNote )
                --voiceGlissandoNote;

            // target reached?
            if( transposedNote == voiceGlissandoNote )
                voicePortamentoActive = 0;
        } else {
            // take over new counter value
            voicePortamentoCtr = portamentoCtr;
        }
        // switch to current glissando note
        transposedNote = voiceGlissandoNote;
    } else {
        // store current transposed note (optionally varried by glissando effect)
        voiceGlissandoNote = transposedNote;
    }

    // transfer note -> linear frequency
    int target_frq = transposedNote << 9;

    // increase/decrease target frequency by pitchrange
    // depending on pitchbender and finetune value
    if( voicePitchrange ) {
        int delta = midiVoicePtr->midivoicePitchbender;
        delta += (int)voiceFinetune-0x80;

        if( delta ) {
            int scaled = delta * (int)voicePitchrange;
            target_frq += scaled;
        }
    }

    // saturate target frequency to 16bit
    if( target_frq < 0 ) target_frq = 0; else if( target_frq > 0xffff ) target_frq = 0xffff;

    // portamento
    // whenever target frequency has been changed, update portamento frequency
    if( voicePortamentoEndFrq != target_frq ) {
        voicePortamentoEndFrq = target_frq;
        voicePortamentoBeginFrq = voiceLinearFrq;
        // reset portamento counter if not in glissando mode
        if( !voiceGlissandoMode )
            voicePortamentoCtr = 0;

        if( target_frq == voiceLinearFrq )
            voicePortamentoActive = 0; // nothing to do...
    }

    int linear_frq = target_frq;
    if( voicePortamentoActive && voicePortamentoRate && !voiceGlissandoMode ) {
        linear_frq = voiceLinearFrq;

        // get portamento multiplier from envelope table
        // this one is used for "constant time glide" and "normal portamento"
        int portaMultiplier = mbCvEnvTable[voicePortamentoRate] / updateSpeedFactor;

        if( voiceConstantTimeGlide || voiceGlissandoMode ) {
            // increment counter
            int portamentoCtr = voicePortamentoCtr + portaMultiplier;

            // target reached on overrun
            if( portamentoCtr >= 0xffff ) {
                linear_frq = target_frq;
                voicePortamentoActive = 0;
            } else {
                voicePortamentoCtr = portamentoCtr;

                // scale between new and old frequency
                int delta = voicePortamentoEndFrq - voicePortamentoBeginFrq;
                linear_frq = voicePortamentoBeginFrq + ((delta * portamentoCtr) >> 16);
                if( delta > 0 ) {
                    if( linear_frq >= target_frq ) {
                        linear_frq = target_frq;
                        voicePortamentoActive = 0;
                    }
                } else {
                    if( linear_frq <= target_frq ) {
                        linear_frq = target_frq;
                        voicePortamentoActive = 0;
                    }
                }
            }
        } else { // normal portamento

            // increment/decrement frequency
            int inc = (int)(((u32)linear_frq * (u32)portaMultiplier) >> 16);
            if( !inc )
                inc = 1;

            if( target_frq > linear_frq ) {
                linear_frq += inc;
                if( linear_frq >= target_frq ) {
                    linear_frq = target_frq;
                    voicePortamentoActive = 0;
                }
            } else {
                linear_frq -= inc;
                if( linear_frq <= target_frq ) {
                    linear_frq = target_frq;
                    voicePortamentoActive = 0;
                }
            }
        }
    }

    // pitch modulation
    if( voicePitchModulation ) {
        linear_frq += voicePitchModulation;
        if( linear_frq < 0 ) linear_frq = 0; else if( linear_frq > 0xffff ) linear_frq = 0xffff;
    }

    // if frequency has been changed
    if( voiceForceFrqRecalc || voiceLinearFrq != linear_frq ) {
        voiceForceFrqRecalc = 0;
        voiceLinearFrq = linear_frq;

        // TODO: curve via frq_ix
#if 0
        // linear frequency -> CV frequency conversion
        // convert bitfield [15:9] of 16bit linear frequency to CV frequency value
        u8 frq_ix = (linear_frq >> 9) + 21;
        if( frq_ix > 126 )
            frq_ix = 126;
        // interpolate between two frequency steps (frq_a and frq_b)
        int frq_a = mbCvFrqTable[frq_ix];
        int frq_b = mbCvFrqTable[frq_ix+1];

        // use bitfield [8:0] of 16bit linear frequency for scaling between two semitones
        int frq_semi = (frq_b-frq_a) * (linear_frq & 0x1ff);
        int frq = frq_a + (frq_semi >> 9);
#endif

        // final value
        voiceFrq = voiceLinearFrq;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Note On Handling
// returns 1 if gate has been retriggered (for propagation to trigger matrix)
/////////////////////////////////////////////////////////////////////////////
bool MbCvVoice::noteOn(u8 note, u8 velocity)
{
    // save note
    voicePlayedNote = note;
    if( !voiceSequencerOnly )
        voiceNote = note;

    // save velocity
    voiceVelocity = velocity;

    // ensure that note is not depressed anymore
    // TODO n->note_items[0].depressed = 0;

    if( voiceSusKey ) {
        // in SusKey mode, we activate portamento only if at least two keys are played
        // omit portamento if first key played after patch initialisation
        if( midiVoicePtr->midivoiceNotestack.len >= 2 && voicePortamentoInitialized )
            voicePortamentoActive = 1;
    } else {
        // portamento always activated (will immediately finish if portamento value = 0)
        // omit portamento if first key played after patch initialisation
        if( voicePortamentoInitialized )
            voicePortamentoActive = 1;
    }

    // next key will allow portamento
    voicePortamentoInitialized = 1;

    // gate bit handling
    if( !voiceLegato || !voiceActive ) {
        // enable gate
        gateOn();

        // set accent flag depending on velocity (set when velocity >= 0x40)
        voiceAccentActive = (velocity >= 0x40) ? 1 : 0;

        // notify that new note has been triggered
        return 1;
    }

    return 0; // gate not triggered
}


/////////////////////////////////////////////////////////////////////////////
// Note Off Handling
// returns 1 to request gate-retrigger
/////////////////////////////////////////////////////////////////////////////
bool MbCvVoice::noteOff(u8 note, u8 lastFirstNote)
{
    // if there is still a note in the stack, play new note with NoteOn Function (checked by caller)
    if( midiVoicePtr->midivoiceNotestack.len && !voicePoly ) {
        // if not in legato mode and current note-off number equat to last entry #0: gate off
        if( !voiceLegato && note == lastFirstNote )
            gateOff(note);

        // activate portamento (will be ignored by pitch handler if no portamento active - important for SusKey function to have it here!)
        voicePortamentoActive = 1;
        return 1; // request Note On!
    }

    // request gate clear bit
    gateOff(note);

    return 0; // NO note on!
}


/////////////////////////////////////////////////////////////////////////////
// Gate Handling
/////////////////////////////////////////////////////////////////////////////
void MbCvVoice::gateOn(void)
{
    voiceActive = 1;
    // gate requested via trigger matrix
}


void MbCvVoice::gateOff(u8 note)
{
    if( voicePoly && note != voicePlayedNote )
        return; // has to be done outcve the noteOff handler! (see MbCvSeMulti.cpp)

    if( voiceActive ) {
        voiceActive = 0;
        voiceGateSetReq = 0;
        voiceGateClrReq = 1;

        // remove gate set request
        voiceNoteRestartReq = 0;
    }
}
