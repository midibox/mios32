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
#include "MbSidTables.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoice::MbSidVoice()
{
    init(NULL, 0, 0, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoice::~MbSidVoice()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::init(sid_se_voice_patch_t *_voicePatch, u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice, sid_se_midi_voice_t *_midiVoice)
{
    voicePatch = _voicePatch;
    voiceNum = _voiceNum;
    physVoiceNum = _physVoiceNum;
    physSidVoice = _physSidVoice;
    midiVoice = _midiVoice;

    voiceActive = false;
    voiceDisabled = false;
    gateActive = false;
    gateSetReq = false;
    gateClrReq = false;
    oscSyncInProgress = false;
    portaActive = false;
    portaInitialized = false;
    accent = false;
    slide = false;
    forceFrqRecalc = false;
    noteRestartReq = false;

    note = 0;
    arpNote = 0;
    playedNote = 0;
    prevTransposedNote = 0;
    glissandoNote = 0;
    portamentoBeginFrq = 0;
    portamentoEndFrq = 0;
    portamentoCtr = 0;
    linearFrq = 0;
    setDelayCtr = 0;
    clrDelayCtr = 0;

    assignedInstrument = ~0; // invalid instrument

    drumWaveform = 0;
    drumGatelength = 0;

    modDstPitch = NULL;
    modDstPw = NULL;
    trgMaskNoteOn = NULL;
    trgMaskNoteOff = NULL;
    drumModel = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Gate
// returns true if pitch should be changed
/////////////////////////////////////////////////////////////////////////////
bool MbSidVoice::gate(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    bool changePitch = true;

    // restart request?
    if( noteRestartReq ) {
        noteRestartReq = false;

        // request gate if voice is active (and request clear for proper ADSR handling)
        gateClrReq = true;
        if( voiceActive )
            gateSetReq = true;

        if( engine == SID_SE_DRUM ) {
            // initialize clear delay counter (drum engine controls the gate active time)
            clrDelayCtr = drumGatelength ? 1 : 0;
            // no set delay by default
            setDelayCtr = 0;

            // delay if ABW (ADSR bug workaround) option active
            // this feature works different for drum engine: test flag is set and waveform is cleared
            // instead of clearing the ADSR registers - this approach is called "hard-sync"
            sid_se_opt_flags_t optFlags;
            optFlags.ALL = mbSidSe->mbSidPatch.body.opt_flags;
            if( optFlags.ABW ) {
                setDelayCtr = 0x0001;

                // clear waveform register and set test+gate flag
                physSidVoice->waveform_reg = 0x09;
            }
        } else {
            // check if voice should be delayed - set delay counter to 0x0001 in this case, else to 0x0000
            setDelayCtr = voicePatch->delay ? 0x0001 : 0x0000;

            // delay also if ABW (ADSR bug workaround) option is active
            sid_se_opt_flags_t optFlags;
			optFlags.ALL = mbSidSe->mbSidPatch.body.opt_flags;
            if( optFlags.ABW ) {
                if( !setDelayCtr ) // at least +1 delay
                    setDelayCtr = 0x0001;

                // clear ADSR registers, so that the envelope gets completely released
                physSidVoice->ad = 0x00;
                physSidVoice->sr = 0x00;
            }
        }
    }

    // voice disable handling (allows to turn on/off voice via waveform parameter)
    sid_se_voice_waveform_t waveform;
    if( engine == SID_SE_DRUM ) {
        if( drumModel )
            waveform.ALL = drumModel->waveform;
        else
			return false; // no drum model selected
    } else
        waveform.ALL = voicePatch->waveform;

    if( voiceDisabled ) {
        if( !waveform.VOICE_OFF ) {
            voiceDisabled = false;
            if( voiceActive )
                gateSetReq = true;
        }
    } else {
        if( waveform.VOICE_OFF ) {
            voiceDisabled = true;
            gateClrReq = true;
        }
    }

    // if gate not active: ignore clear request
    if( !gateActive )
        gateClrReq = false;

    // gate set/clear request?
    if( gateClrReq ) {
        gateClrReq = false;

        // clear SID gate flag if GSA (gate stays active) function not enabled
        sid_se_voice_flags_t voice_flags;
        if( engine == SID_SE_DRUM )
            voice_flags.ALL = 0;
        else
            voice_flags.ALL = voicePatch->flags;

        if( !voice_flags.GSA )
            physSidVoice->gate = 0;

        // gate not active anymore
        gateActive = false;
    } else if( gateSetReq || oscSyncInProgress ) {
        // don't set gate if oscillator disabled
        if( !waveform.VOICE_OFF ) {
            sid_se_opt_flags_t optFlags;
			optFlags.ALL = mbSidSe->mbSidPatch.body.opt_flags;

            // delay note so long 16bit delay counter != 0
            if( setDelayCtr ) {
                int delayInc = 0;
                if( engine != SID_SE_DRUM )
                    delayInc = voicePatch->delay;

                // if ABW (ADSR bug workaround) active: use at least 30 mS delay
                if( optFlags.ABW ) {
                    delayInc += 25;
                    if( delayInc > 255 )
                        delayInc = 255;
                }

                int setDelayCtr = setDelayCtr + mbSidEnvTable[delayInc] / updateSpeedFactor;
                if( delayInc && setDelayCtr < 0xffff ) {
                    // no overrun
                    setDelayCtr = setDelayCtr;
                    // don't change pitch so long delay is active
                    changePitch = false;
                } else {
                    // overrun: clear counter to disable delay
                    setDelayCtr = 0x0000;
                    // for ABW (hard-sync)
                    physSidVoice->test = 0;
                }
            }

            if( !setDelayCtr ) {
                // acknowledge the set request
                gateSetReq = false;

                // optional OSC synchronisation
                u8 skip_gate = 0;
                if( !oscSyncInProgress ) {
                    switch( engine ) {
                    case SID_SE_LEAD: {
                        u8 osc_phase;
                        if( (osc_phase=mbSidSe->mbSidPatch.body.L.osc_phase) ) {
                            // notify that OSC synchronisation has been started
                            oscSyncInProgress = true;
                            // set test flag for one update cycle
                            physSidVoice->test = 1;
                            // set pitch depending on selected oscillator phase to achieve an offset between the waveforms
                            // This approach has been invented by Wilba! :-)
                            u32 reference_frq = 16779; // 1kHz
                            u32 osc_sync_frq;
                            switch( voiceNum % 3 ) {
                            case 1: osc_sync_frq = (reference_frq * (1000 + 4*(u32)osc_phase)) / 1000; break;
                            case 2: osc_sync_frq = (reference_frq * (1000 + 8*(u32)osc_phase)) / 1000; break;
                            default: osc_sync_frq = reference_frq; // (Voice 0 and 3)
                            }
                            physSidVoice->frq_l = osc_sync_frq & 0xff;
                            physSidVoice->frq_h = osc_sync_frq >> 8;
                            // don't change pitch for this update cycle!
                            changePitch = false;
                            // skip gate handling for this update cycle
                            skip_gate = 1;
                        }
                    } break;

                    case SID_SE_BASSLINE: {
                        // set test flag if OSC_PHASE_SYNC enabled
                        sid_se_v_flags_t v_flags;
						v_flags.ALL = voicePatch->B.v_flags;
                        if( v_flags.OSC_PHASE_SYNC ) {
                            // notify that OSC synchronisation has been started
                            oscSyncInProgress = 1;
                            // set test flag for one update cycle
                            physSidVoice->test = 1;
                            // don't change pitch for this update cycle!
                            changePitch = false;
                            // skip gate handling for this update cycle
                            skip_gate = 1;
                        }
                    } break;

                    case SID_SE_MULTI: {
                        // TODO
                    } break;

                    case SID_SE_DRUM: {
                        // not relevant
                    } break;
                    }
                }

                if( !skip_gate ) { // set by code below if OSC sync is started
                    if( physSidVoice->test ) {
                        // clear test flag
                        physSidVoice->test = 0;
                        // don't change pitch for this update cycle!
                        changePitch = false;
                        // ensure that pitch handler will re-calculate pitch frequency on next update cycle
                        forceFrqRecalc = 1;
                    } else {
                        // this code is also executed if OSC synchronisation disabled
                        // set the gate flag
                        physSidVoice->gate = 1;
                        // OSC sync finished
                        oscSyncInProgress = 0;
                    }
                }
            }
        }
        gateActive = 1;
    }

    return changePitch;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pitch(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // transpose MIDI note
    sid_se_voice_arp_mode_t arp_mode;
	arp_mode.ALL = voicePatch->arp_mode;
    int transposed_note = arp_mode.ENABLE ? arpNote : note;

    if( engine == SID_SE_BASSLINE ) {
        // get transpose value depending on voice number
        switch( voiceNum ) {
        case 1:
        case 4:
            if( voicePatch->B.v2_static_note )
                transposed_note = voicePatch->B.v2_static_note;
            else {
                u8 oct_transpose = voicePatch->B.v2_oct_transpose;
                if( oct_transpose & 4 )
                    transposed_note -= 12*(4-(oct_transpose & 3));
                else
                    transposed_note += 12*(oct_transpose & 3);
            }
            break;

        case 2:
        case 5:
            if( voicePatch->B.v3_static_note )
                transposed_note = voicePatch->B.v3_static_note;
            else {
                u8 oct_transpose = voicePatch->B.v3_oct_transpose;
                if( oct_transpose & 4 )
                    transposed_note -= 12*(4-(oct_transpose & 3));
                else
                    transposed_note += 12*(oct_transpose & 3);
            }
            break;

        default: // 0 and 3
            transposed_note += voicePatch->transpose - 64;
        }

        // MV transpose: if sequencer running, we can transpose with MIDI voice 3/4
        sid_se_v_flags_t v_flags;
		v_flags.ALL = voicePatch->B.v_flags;
        if( v_flags.WTO ) {
            sid_se_midi_voice_t *mv_transpose = (voiceNum >= 3)
                ? (sid_se_midi_voice_t *)&mbSidSe->sid_se_midi_voice[3]
                : (sid_se_midi_voice_t *)&mbSidSe->sid_se_midi_voice[2];

            // check for MIDI channel to ensure compatibility with older ensemble patches
            if( mv_transpose->midi_channel < 16 && mv_transpose->notestack.len )
                transposed_note += mv_transpose->notestack.note_items[0].note - 0x3c + mv_transpose->transpose - 64;
            else
                transposed_note += (int)midiVoice->transpose - 64;
        } else {
            transposed_note += (int)midiVoice->transpose - 64;
        }    
    } else {
        // Lead & Multi engine
        transposed_note += voicePatch->transpose - 64;
        transposed_note += (int)midiVoice->transpose - 64;
    }



    // octave-wise saturation
    while( transposed_note < 0 )
        transposed_note += 12;
    while( transposed_note >= 128 )
        transposed_note -= 12;

    // Glissando handling
    sid_se_voice_flags_t voice_flags;
	voice_flags.ALL = voicePatch->flags;
    if( transposed_note != prevTransposedNote ) {
        prevTransposedNote = transposed_note;
        if( voice_flags.PORTA_MODE >= 2 ) // Glissando active?
            portamentoCtr = 0xffff; // force overrun
    }

    u8 portamento = voicePatch->portamento;
    if( portaActive && portamento && voice_flags.PORTA_MODE >= 2 ) {
        // increment portamento counter, amount derived from envelope table  .. make it faster
        int portamento_ctr = portamentoCtr + mbSidEnvTable[portamento >> 1] / updateSpeedFactor;
        // next note step?
        if( portamento_ctr >= 0xffff ) {
            // reset counter
            portamentoCtr = 0;

            // increment/decrement note
            if( transposed_note > glissandoNote )
                ++glissandoNote;
            else if( transposed_note < glissandoNote )
                --glissandoNote;

            // target reached?
            if( transposed_note == glissandoNote )
                portaActive = 0;
        } else {
            // take over new counter value
            portamentoCtr = portamento_ctr;
        }
        // switch to current glissando note
        transposed_note = glissandoNote;
    } else {
        // store current transposed note (optionally varried by glissando effect)
        glissandoNote = transposed_note;
    }

    // transfer note -> linear frequency
    int target_frq = transposed_note << 9;

    // increase/decrease target frequency by pitchrange
    // depending on pitchbender and finetune value
    if( voicePatch->pitchrange ) {
        u16 pitchbender = midiVoice->pitchbender;
        int delta = (int)pitchbender - 0x80;
        delta += (int)voicePatch->finetune-0x80;

        // detuning
        u8 detune = mbSidSe->mbSidPatch.body.L.osc_detune;
        if( detune ) {
            // additional detuning depending on SID channel and oscillator
            // Left OSC1: +detune/4   (lead only, 0 in bassline mode)
            // Right OSC1: -detune/4  (lead only, 0 in bassline mode)
            // Left OSC2: +detune
            // Right OSC2: -detune
            // Left OSC3: -detune
            // Right OSC3: +detune
            switch( voiceNum ) {
            case 0: if( engine == SID_SE_LEAD ) delta += detune/4; break; // makes only sense on stereo sounds
            case 3: if( engine == SID_SE_LEAD ) delta -= detune/4; break; // makes only sense on stereo sounds

            case 1:
            case 5: delta += detune; break;

            case 2:
            case 4: delta -= detune; break;
            }
        }

        if( delta ) {
            int scaled = (delta * 4 * (int)voicePatch->pitchrange);
            target_frq += scaled;
        }
    }

    // saturate target frequency to 16bit
    if( target_frq < 0 ) target_frq = 0; else if( target_frq > 0xffff ) target_frq = 0xffff;

    // portamento
    // whenever target frequency has been changed, update portamento frequency
    if( portamentoEndFrq != target_frq ) {
        portamentoEndFrq = target_frq;
        portamentoBeginFrq = linearFrq;
        // reset portamento counter if not in glissando mode
        if( voice_flags.PORTA_MODE < 2 )
            portamentoCtr = 0;

        if( target_frq == linearFrq )
            portaActive = 0; // nothing to do...
    }

    int linear_frq = target_frq;
    if( portaActive && portamento && voice_flags.PORTA_MODE < 2 ) {
        linear_frq = linearFrq;

        // get portamento multiplier from envelope table
        // this one is used for "constant time glide" and "normal portamento"
        int porta_multiplier = mbSidEnvTable[portamento] / updateSpeedFactor;

        if( voice_flags.PORTA_MODE >= 1 ) { // constant glide time and glissando
            // increment counter
            int portamento_ctr = portamentoCtr + porta_multiplier;

            // target reached on overrun
            if( portamento_ctr >= 0xffff ) {
                linear_frq = target_frq;
                portaActive = 0;
            } else {
                portamentoCtr = portamento_ctr;

                // scale between new and old frequency
                int delta = portamentoEndFrq - portamentoBeginFrq;
                linear_frq = portamentoBeginFrq + ((delta * portamento_ctr) >> 16);
                if( delta > 0 ) {
                    if( linear_frq >= target_frq ) {
                        linear_frq = target_frq;
                        portaActive = 0;
                    }
                } else {
                    if( linear_frq <= target_frq ) {
                        linear_frq = target_frq;
                        portaActive = 0;
                    }
                }
            }
        } else { // normal portamento

            // increment/decrement frequency
            int inc = (int)(((u32)linear_frq * (u32)porta_multiplier) >> 16);
            if( !inc )
                inc = 1;

            if( target_frq > linear_frq ) {
                linear_frq += inc;
                if( linear_frq >= target_frq ) {
                    linear_frq = target_frq;
                    portaActive = 0;
                }
            } else {
                linear_frq -= inc;
                if( linear_frq <= target_frq ) {
                    linear_frq = target_frq;
                    portaActive = 0;
                }
            }
        }
    }

    // pitch modulation
    if( modDstPitch ) {
        linear_frq += *modDstPitch;
        if( linear_frq < 0 ) linear_frq = 0; else if( linear_frq > 0xffff ) linear_frq = 0xffff;
    }

    // if frequency has been changed
    if( forceFrqRecalc || linearFrq != linear_frq ) {
        forceFrqRecalc = 0;
        linearFrq = linear_frq;

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
// Voice Pitch for Drum Instruments
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pitchDrum(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // get frequency depending on base note
    u8 note = note;
    u8 frq_ix = note + 21;
    if( frq_ix > 127 )
        frq_ix = 127;
    s32 target_frq = mbSidFrqTable[frq_ix];

    if( assignedInstrument < 16 ) {
        sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidSe->mbSidPatch.body.D.voice[assignedInstrument];

        // increase/decrease target frequency by pitchrange depending on finetune value
        u8 tune = voice_patch->D.tune;
        if( tune != 0x80 ) {
            u8 pitchrange = 24;
            if( tune > 0x80 ) {
                int dst_note = note + pitchrange + 21;
                if( dst_note > 127 ) dst_note = 127;
                u16 dst_frq = mbSidFrqTable[dst_note];
                target_frq += ((dst_frq-target_frq) * (tune-0x80)) / 128;
                if( target_frq >= 0xffff ) target_frq = 0xffff;
            } else {
                int dst_note = note - pitchrange + 21;
                if( dst_note < 0 ) dst_note = 0;
                u16 dst_frq = mbSidFrqTable[dst_note];
                target_frq -= ((target_frq-dst_frq) * (0x80-tune)) / 128;
                if( target_frq < 0 ) target_frq = 0;
            }
        }
    }

    // copy target frequency into SID registers
    physSidVoice->frq_l = target_frq & 0xff;
    physSidVoice->frq_h = target_frq >> 8;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pulsewidth
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pw(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // convert pulsewidth to 12bit signed value
    int pulsewidth;
    if( engine == SID_SE_BASSLINE ) {
        // get pulsewidth value depending on voice number
        switch( voiceNum ) {
        case 1:
        case 4:
            pulsewidth = ((voicePatch->B.v2_pulsewidth_h & 0x0f) << 8) | voicePatch->B.v2_pulsewidth_l;
            break;

        case 2:
        case 5:
            pulsewidth = ((voicePatch->B.v3_pulsewidth_h & 0x0f) << 8) | voicePatch->B.v3_pulsewidth_l;
            break;

        default: // 0 and 3
            pulsewidth = ((voicePatch->pulsewidth_h & 0x0f) << 8) | voicePatch->pulsewidth_l;
            break;
        }
    } else {
        pulsewidth = ((voicePatch->pulsewidth_h & 0x0f) << 8) | voicePatch->pulsewidth_l;
    }

    // PW modulation
    if( modDstPw ) {
        pulsewidth += *modDstPw / 16;
        if( pulsewidth > 0xfff ) pulsewidth = 0xfff; else if( pulsewidth < 0 ) pulsewidth = 0;
    }

    // transfer to SID registers
    physSidVoice->pw_l = pulsewidth & 0xff;
    physSidVoice->pw_h = (pulsewidth >> 8) & 0x0f;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pulsewidth for Drum Instruments
/////////////////////////////////////////////////////////////////////////////
void MbSidVoice::pwDrum(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // take pulsewidth specified by drum model
    u16 pulsewidth = drumModel ? (drumModel->pulsewidth << 4) : 0x800;

    // transfer to SID registers
    physSidVoice->pw_l = pulsewidth & 0xff;
    physSidVoice->pw_h = (pulsewidth >> 8) & 0x0f;
}

