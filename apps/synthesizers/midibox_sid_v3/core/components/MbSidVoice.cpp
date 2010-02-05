/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Voice Handlers
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
#include "MbSidVoice.h"
#include "MbSidMidiVoice.h"
#include "MbSidTables.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoice::MbSidVoice()
{
    init(0, 0, NULL);
    midiVoicePtr = NULL; // assigned by MbSid!
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoice::~MbSidVoice()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function with voice and register assignments
// (usually only called once after startup)
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::init(u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice)
{
    voiceNum = _voiceNum;
    physVoiceNum = _physVoiceNum;
    physSidVoice = _physSidVoice;

    this->init();
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::init(void)
{
    voiceLegato = 0;
    voiceWavetableOnly = 0;
    voiceSusKey = 0;
    voicePoly = 0;
    voiceConstantTimeGlide = 0;
    voiceGlissandoMode = 0;
    voiceGateStaysActive = 0;
    voiceAdsrBugWorkaround = 0;
    voiceWaveformOff = 0;
    voiceWaveformSync = 0;
    voiceWaveformRingmod = 0;
    voiceWaveform = 0;
    voiceAttackDecay.ALL = 0;
    voiceSustainRelease.ALL = 0;
    voicePulsewidth = 0;
    voiceAccentRate = 0;
    voiceDelay = 0;
    voiceTranspose = 0;
    voiceFinetune = 0;
    voiceDetuneDelta = 0;
    voicePitchrange = 0;
    voicePortamentoRate = 0;
    voiceSwinSidMode = 0;
    voiceOscPhase = 0;

    voicePitchModulation = 0;
    voicePulsewidthModulation = 0;

    voiceActive = 0;
    voiceDisabled = 0;
    voiceGateActive = 0;
    voiceGateSetReq = 0;
    voiceGateClrReq = 0;
    voiceOscSyncInProgress = 0;
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
    voiceSetDelayCtr = 0;
    voiceClrDelayCtr = 0;

    voiceAssignedInstrument = ~0; // invalid instrument

    trgMaskNoteOn = NULL;
    trgMaskNoteOff = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Gate
// returns 1 if pitch should be changed
/////////////////////////////////////////////////////////////////////////////
bool MbSidVoice::gate(const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    bool changePitch = 1;

    // restart request?
    if( voiceNoteRestartReq ) {
        voiceNoteRestartReq = 0;

        // request gate if voice is active (and request clear for proper ADSR handling)
        voiceGateClrReq = 1;
        if( voiceActive )
            voiceGateSetReq = 1;

        // check if voice should be delayed - set delay counter to 0x0001 in this case, else to 0x0000
        voiceSetDelayCtr = voiceDelay ? 0x0001 : 0x0000;

        // delay also if ADSR bug workaround option is active
        if( voiceAdsrBugWorkaround ) {
            if( !voiceSetDelayCtr ) // at least +1 delay
                voiceSetDelayCtr = 0x0001;

            // clear ADSR registers, so that the envelope gets completely released
            physSidVoice->ad = 0x00;
            physSidVoice->sr = 0x00;
        }
    }

    if( voiceDisabled ) {
        if( !voiceWaveformOff ) {
            voiceDisabled = 0;
            if( voiceActive )
                voiceGateSetReq = 1;
        }
    } else {
        if( voiceWaveformOff ) {
            voiceDisabled = 1;
            voiceGateClrReq = 1;
        }
    }

    // if gate not active: ignore clear request
    if( !voiceGateActive )
        voiceGateClrReq = 0;

    // gate set/clear request?
    if( voiceGateClrReq ) {
        voiceGateClrReq = 0;

        // clear SID gate flag if GSA function not enabled
        if( !voiceGateStaysActive )
            physSidVoice->gate = 0;

        // gate not active anymore
        voiceGateActive = 0;
    } else if( voiceGateSetReq || voiceOscSyncInProgress ) {
        // don't set gate if oscillator disabled
        if( !voiceWaveformOff ) {
            // delay note so long 16bit delay counter != 0
            if( voiceSetDelayCtr ) {
                int delayInc = voiceDelay;

                // if ADSR bug workaround active: use at least 30 mS delay
                if( voiceAdsrBugWorkaround ) {
                    delayInc += 25;
                    if( delayInc > 255 )
                        delayInc = 255;
                }

                int delayCtr = voiceSetDelayCtr + mbSidEnvTable[delayInc] / updateSpeedFactor;
                if( delayInc && delayCtr < 0xffff ) {
                    // no overrun
                    voiceSetDelayCtr = delayCtr;
                    // don't change pitch so long delay is active
                    changePitch = 0;
                } else {
                    // overrun: clear counter to disable delay
                    voiceSetDelayCtr = 0x0000;
                    // for ADSR Bug Workaround (hard-sync)
                    physSidVoice->test = 0;
                }
            }

            if( !voiceSetDelayCtr ) {
                // acknowledge the set request
                voiceGateSetReq = 0;

                // optional OSC synchronisation
                u8 skipGate = 0;
                if( !voiceOscSyncInProgress ) {
                    if( voiceOscPhase ) {
                        if( voiceOscPhase == 1 ) {
                            // notify that OSC synchronisation has been started
                            voiceOscSyncInProgress = 1;
                            // set test flag for one update cycle
                            physSidVoice->test = 1;
                            // don't change pitch for this update cycle!
                            changePitch = 0;
                            // skip gate handling for this update cycle
                            skipGate = 1;
                        } else {
                            // notify that OSC synchronisation has been started
                            voiceOscSyncInProgress = 1;
                            // set test flag for one update cycle
                            physSidVoice->test = 1;
                            // set pitch depending on selected oscillator phase to achieve an offset between the waveforms
                            // This approach has been invented by Wilba! :-)
                            u32 reference_frq = 16779; // 1kHz
                            u32 osc_sync_frq;
                            switch( voiceNum % 3 ) {
                            case 1: osc_sync_frq = (reference_frq * (1000 + 4*(u32)voiceOscPhase)) / 1000; break;
                            case 2: osc_sync_frq = (reference_frq * (1000 + 8*(u32)voiceOscPhase)) / 1000; break;
                            default: osc_sync_frq = reference_frq; // (Voice 0 and 3)
                            }
                            physSidVoice->frq_l = osc_sync_frq & 0xff;
                            physSidVoice->frq_h = osc_sync_frq >> 8;
                            // don't change pitch for this update cycle!
                            changePitch = 0;
                            // skip gate handling for this update cycle
                            skipGate = 1;
                        }
                    }
                }

                if( !skipGate ) { // set by code below if OSC sync is started
                    if( physSidVoice->test ) {
                        // clear test flag
                        physSidVoice->test = 0;
                        // don't change pitch for this update cycle!
                        changePitch = 0;
                        // ensure that pitch handler will re-calculate pitch frequency on next update cycle
                        voiceForceFrqRecalc = 1;
                    } else {
                        // this code is also executed if OSC synchronisation disabled
                        // set the gate flag
                        physSidVoice->gate = 1;
                        // OSC sync finished
                        voiceOscSyncInProgress = 0;
                    }
                }
            }
        }
        voiceGateActive = 1;
    }

    return changePitch;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pitch(const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // transpose MIDI note
    int transposedNote = voiceForcedNote;

    if( !transposedNote ) {
        transposedNote = voiceNote;
        transposedNote += voiceTranspose - 64;
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
        int portamentoCtr = voicePortamentoCtr + mbSidEnvTable[voicePortamentoRate >> 1] / updateSpeedFactor;
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
        u16 pitchbender = midiVoicePtr->midivoicePitchbender;
        int delta = (int)pitchbender - 0x80;
        delta += (int)voiceFinetune-0x80;

        if( voiceDetuneDelta ) {
            int scaled = (voiceDetuneDelta * 4 * (int)voicePitchrange);
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
        int portaMultiplier = mbSidEnvTable[voicePortamentoRate] / updateSpeedFactor;

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

        // linear frequency -> SID frequency conversion
        // convert bitfield [15:9] of 16bit linear frequency to SID frequency value
        u8 frq_ix = (linear_frq >> 9) + 21;
        if( frq_ix > 126 )
            frq_ix = 126;
        // interpolate between two frequency steps (frq_a and frq_b)
        int frq_a = mbSidFrqTable[frq_ix];
        int frq_b = mbSidFrqTable[frq_ix+1];

        // use bitfield [8:0] of 16bit linear frequency for scaling between two semitones
        int frq_semi = (frq_b-frq_a) * (linear_frq & 0x1ff);
        int frq = frq_a + (frq_semi >> 9);

        // write result into SID frequency register
        physSidVoice->frq_l = frq & 0xff;
        physSidVoice->frq_h = frq >> 8;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pulsewidth
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pw(const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // convert pulsewidth to 12bit signed value
    int pulsewidth = voicePulsewidth;

    // PW modulation
    if( voicePulsewidthModulation ) {
        pulsewidth += voicePulsewidthModulation / 16;
        if( pulsewidth > 0xfff ) pulsewidth = 0xfff; else if( pulsewidth < 0 ) pulsewidth = 0;
    }

    // transfer to SID registers
    physSidVoice->pw_l = pulsewidth & 0xff;
    physSidVoice->pw_h = (pulsewidth >> 8) & 0x0f;
}


/////////////////////////////////////////////////////////////////////////////
// Note On Handling
// returns 1 if gate has been retriggered (for propagation to trigger matrix)
/////////////////////////////////////////////////////////////////////////////
bool MbSidVoice::noteOn(u8 note, u8 velocity)
{
    // save note
    voicePlayedNote = note;
    if( !voiceWavetableOnly )
        voiceNote = note;

    // ensure that note is not depressed anymore
    // TODO n->note_items[0].depressed = 0;

    if( voiceSusKey ) {
        // in SusKey mode, we activate portamento only if at least two keys are played
        // omit portamento if first key played after patch initialisation
        if( midiVoicePtr->midivoiceNotestack.len >= 2 && voicePortamentoInitialized )
            voicePortamentoActive = 1;
    } else {
        // portamento always activated (will immediately finish if portamento value = 0)
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
bool MbSidVoice::noteOff(u8 note, u8 lastFirstNote)
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
void MbSidVoice::gateOn(void)
{
    voiceActive = 1;
    // gate requested via trigger matrix
}


void MbSidVoice::gateOff(u8 note)
{
    if( voicePoly && note != voicePlayedNote )
        return; // has to be done outside the noteOff handler! (see MbSidSeMulti.cpp)

    if( voiceActive ) {
        voiceActive = 0;
        voiceGateSetReq = 0;
        voiceGateClrReq = 1;

        // remove gate set request
        voiceNoteRestartReq = 0;
    }
}



/////////////////////////////////////////////////////////////////////////////
// Wavetable Note
// Returns 1 if additional Note On events should be triggered via matrix
// Returns 2 if additional Note Off events should be triggered via matrix
// Otherwise returns 0
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::playWtNote(MbSidSe *se, MbSidVoice *v, u8 wtValue)
{
    // only absolute values supported
    wtValue &= 0x7f;

    // do nothing if hold note (0x01) is played
    if( wtValue != 0x01 ) {
        // clear gate bit if note wtValue is 0
        if( wtValue == 0 ) {
            if( voiceGateActive ) {
                voiceGateSetReq = 0;
                voiceGateClrReq = 1;

                // propagate to trigger matrix
                se->triggerNoteOff(v, 1);
            }
        } else {
            // TODO: hold mode (via trigger matrix?)

            // set gate bit if voice active and gate not already active
            if( voiceActive && !voiceGateActive ) {
                voiceGateSetReq = 1;

                // propagate to trigger matrix
                se->triggerNoteOn(v, 1);
                voiceNoteRestartReq = 0; // clear note restart request which has been set by trigger function - gate already set!
            }

            // set new note
            // if >= 0x7c, play arpeggiator note
            if( wtValue >= 0x7c ) {
                u8 arp_note = midiVoicePtr->midivoiceWtStack[wtValue & 3] & 0x7f;
                if( !arp_note )
                    arp_note = midiVoicePtr->midivoiceWtStack[0] & 0x7f;

                if( arp_note ) {
                    voiceNote = arp_note;
                    voicePortamentoActive = 1; // will be cleared automatically if no portamento enabled
                } else {
                    // no note played: request note off if gate active
                    if( voiceGateActive ) {
                        voiceGateSetReq = 0;
                        voiceGateClrReq = 1;

                        // propagate to trigger matrix
                        se->triggerNoteOff(v, 1);
                    }
                }
            } else {
                voiceNote = wtValue;
                voicePortamentoActive = 1; // will be cleared automatically if no portamento enabled
            }
        }
    }
}
