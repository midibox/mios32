/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Bassline Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidSeBassline.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeBassline::MbSidSeBassline()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeBassline::~MbSidSeBassline()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr)
{
    // assign SID registers to voices
    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
        u8 physVoice = voice % 3;
        sid_voice_t *physSidVoice;
        if( voice >= 3 )
            physSidVoice = (sid_voice_t *)&sidRegRPtr->v[physVoice];
        else
            physSidVoice = (sid_voice_t *)&sidRegLPtr->v[physVoice];

        v->init(voice, physVoice, physSidVoice);
    }

    // assign SID registers to filters
    for(int filter=0; filter<mbSidFilter.size; ++filter)
        mbSidFilter[filter].init(filter ? sidRegRPtr : sidRegLPtr);

    // initialize clock and patch pointer
    mbSidClockPtr = _mbSidClockPtr;
    mbSidPatchPtr = _mbSidPatchPtr;
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the structures of a SID sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::initPatch(bool patchOnly)
{
    if( !patchOnly ) {
        for(int voice=0; voice<mbSidVoice.size; ++voice)
            mbSidVoice[voice].init();

        for(int filter=0; filter<mbSidFilter.size; ++filter)
            mbSidFilter[filter].init();

        for(int lfo=0; lfo<mbSidLfo.size; ++lfo)
            mbSidLfo[lfo].init();

        for(int env=0; env<mbSidEnv.size; ++env)
            mbSidEnv[env].init();

        for(int arp=0; arp<mbSidArp.size; ++arp)
            mbSidArp[arp].init();

        for(int seq=0; seq<mbSidSeqBassline.size; ++seq)
            mbSidSeqBassline[seq].init();
    }

    // iterate over patch to transfer all SysEx parameters into SE objects
    for(int addr=0x10; addr<0x100; ++addr) // reduced range... loop over patch name and WT memory not required
        sysexSetParameter(addr, mbSidPatchPtr->body.ALL[addr]);

    for(int seq=0; seq<mbSidSeqBassline.size; ++seq)
        mbSidSeqBassline[seq].seqPatternMemory = &mbSidPatchPtr->body.B.seq_memory[0];
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeBassline::tick(const u8 &updateSpeedFactor)
{
    // clear modulation destinations
    mbSidVoice[0].voicePitchModulation = 0;
    mbSidVoice[0].voicePulsewidthModulation = 0;
    mbSidVoice[3].voicePitchModulation = 0;
    mbSidVoice[3].voicePulsewidthModulation = 0;
    mbSidFilter[0].filterCutoffModulation = 0;
    mbSidFilter[1].filterCutoffModulation = 0;


    // Clock
    if( mbSidClockPtr->eventStart ) {
        for(int seq=0; seq<mbSidSeqBassline.size; ++seq)
            mbSidSeqBassline[seq].seqRestartReq = 1;
    }

    if( mbSidClockPtr->eventClock ) {
        // clock ARPs
        MbSidArp *a = mbSidArp.first();
        for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
            a->clockReq = true;

        // clock sequencers
        for(int seq=0; seq<mbSidSeqBassline.size; ++seq)
            mbSidSeqBassline[seq].seqClockReq = 1;

        // propagate clock/4 event to trigger matrix on each 6th clock
        if( mbSidClockPtr->clkCtr6 == 0 ) {
            // sync LFOs and ENVs
            MbSidLfo *l = mbSidLfo.first();
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                l->syncClockReq = true;

            MbSidEnv *e = mbSidEnv.first();
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                e->syncClockReq = true;
        }
    }

    // LFOs
    MbSidLfo *l = mbSidLfo.first();
    for(int lfo=0; lfo < mbSidLfo.size; ++lfo, ++l) {
        l->tick(updateSpeedFactor);

        u8 voice = (lfo >= 2) ? 3 : 0;
        u8 filter = (lfo >= 2) ? 1 : 0;
        mbSidVoice[voice].voicePitchModulation += (l->lfoOut * (s32)l->lfoDepthPitch) / 128;
        mbSidVoice[voice].voicePulsewidthModulation += (l->lfoOut * (s32)l->lfoDepthPulsewidth) / 128;
        mbSidFilter[filter].filterCutoffModulation += (l->lfoOut * (s32)l->lfoDepthFilter) / 128;
    }

    // ENVs
    MbSidEnv *e = mbSidEnv.first();
    for(int env=0; env < mbSidEnv.size; ++env, ++e) {
        e->tick(updateSpeedFactor);

        u8 voice = 3*env;
        u8 filter = env;
        mbSidVoice[voice].voicePitchModulation += (e->envOut * (s32)e->envDepthPitch) / 128;
        mbSidVoice[voice].voicePulsewidthModulation += (e->envOut * (s32)e->envDepthPulsewidth) / 128;
        mbSidFilter[filter].filterCutoffModulation += (e->envOut * (s32)e->envDepthFilter) / 128;
    }

    // Arps, Sequencers and Voices
    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
        if( voice == 0 || voice == 3 ) {
            // sequencer/arp only for first voice (OSC1)
            int instrument = voice / 3;

            if( !mbSidArp[instrument].arpEnabled )
                mbSidSeqBassline[instrument].tick(v, this);
            else
                mbSidArp[instrument].tick(v, this);

            // transfer note values and state flags from OSC1 (master) to OSC2/3 (slaves)
            MbSidVoice *vm = &mbSidVoice[voice];
            MbSidVoice *vs = &mbSidVoice[voice+1];
            for(int i=0; i<2; ++i, ++vs) {
                // TODO: most of these values never change 
                // and copying them here instead of MbSidVoice.cpp is dirty (-> can lead to inconsistencies)
                // We should provide separate methods to transfer static/dynamic parameters of vm to vs
                vs->voiceNote = vm->voiceNote;
                vs->voiceLegato = vm->voiceLegato;
                vs->voiceSusKey = vm->voiceSusKey;
                vs->voiceConstantTimeGlide = vm->voiceConstantTimeGlide;
                vs->voiceGlissandoMode = vm->voiceGlissandoMode;
                vs->voiceGateStaysActive = vm->voiceGateStaysActive;
                vs->voiceAdsrBugWorkaround = vm->voiceAdsrBugWorkaround;
                vs->voiceAttackDecay = vm->voiceAttackDecay;
                vs->voiceSustainRelease = vm->voiceSustainRelease;
                vs->voiceAccentRate = vm->voiceAccentRate;
                vs->voiceDelay = vm->voiceDelay;
                vs->voiceFinetune = vm->voiceFinetune;
                vs->voicePitchrange = vm->voicePitchrange;
                vs->voicePortamentoRate = vm->voicePortamentoRate;
                vs->voiceSwinSidMode = vm->voiceSwinSidMode;
                vs->voiceOscPhase = vm->voiceOscPhase;

                vs->voicePitchModulation = vm->voicePitchModulation;
                vs->voicePulsewidthModulation = vm->voicePulsewidthModulation;

                vs->voiceActive = vm->voiceActive;
                vs->voiceGateSetReq = vm->voiceGateSetReq;
                vs->voiceGateClrReq = vm->voiceGateClrReq;
                vs->voicePortamentoActive = vm->voicePortamentoActive;
                vs->voicePortamentoInitialized = vm->voicePortamentoInitialized;
                vs->voiceAccentActive = vm->voiceAccentActive;
                vs->voiceSlideActive = vm->voiceSlideActive;
                vs->voiceForceFrqRecalc = vm->voiceForceFrqRecalc;
                vs->voiceNoteRestartReq = vm->voiceNoteRestartReq;
            }
        }

        if( v->gate(updateSpeedFactor, this) )
            v->pitch(updateSpeedFactor, this);
        v->pw(updateSpeedFactor, this);

        v->physSidVoice->waveform = v->voiceWaveform;
        v->physSidVoice->sync = v->voiceWaveformSync;
        v->physSidVoice->ringmod = v->voiceWaveformRingmod;

        // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
        if( !v->voiceSetDelayCtr ) {
            v->physSidVoice->ad = v->voiceAttackDecay.ALL;

            // force sustain to maximum if accent flag active
            u8 sr = v->voiceSustainRelease.ALL;
            if( v->voiceAccentActive )
                sr |= 0xf0;
            v->physSidVoice->sr = sr;
        }
    }

    // Filters and Volume
    MbSidFilter *f = mbSidFilter.first();
    for(int filter=0; filter<mbSidFilter.size; ++filter, ++f) {
        f->filterKeytrackFrequency = mbSidVoice[filter*3].voiceLinearFrq;
        f->tick(updateSpeedFactor);
    }

    // currently no detection if SIDs have to be updated
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::triggerNoteOn(MbSidVoice *v, u8 no_wt)
{
    u8 voiceOffset;
    u8 lfoOffset;
    u8 env;

    if( v->voiceNum < 3 ) {
        voiceOffset = 0;
        lfoOffset = 0;
        env = 0;
    } else {
        voiceOffset = 3;
        lfoOffset = 2;
        env = 1;
    }

    mbSidVoice[voiceOffset+0].voiceNoteRestartReq = 1;
    mbSidVoice[voiceOffset+1].voiceNoteRestartReq = 1;
    mbSidVoice[voiceOffset+2].voiceNoteRestartReq = 1;

    if( mbSidLfo[lfoOffset+0].lfoMode.KEYSYNC )
        mbSidLfo[lfoOffset+0].restartReq = 1;
    if( mbSidLfo[lfoOffset+1].lfoMode.KEYSYNC )
        mbSidLfo[lfoOffset+1].restartReq = 1;

    mbSidEnv[env].restartReq = 1;
}


void MbSidSeBassline::triggerNoteOff(MbSidVoice *v, u8 no_wt)
{
    u8 env = (v->voiceNum < 3) ? 0 : 1;
    mbSidEnv[env].releaseReq = 1;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    // go through four midi voices
    // 1 and 2 used to play the bassline/select the sequence
    // 3 and 4 used to transpose a sequence
    MbSidMidiVoice *mv = mbSidVoice[0].midiVoicePtr;

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    for(int midiVoice=0; midiVoice<4; ++midiVoice, ++mv) {
        // check if MIDI channel and splitzone matches
        if( !mv->isAssigned(chn, note) )
            continue; // next MIDI voice

        if( midiVoice >= 2 ) { // Transposer
            if( velocity ) // Note On
                mv->notestackPush(note, velocity);
            else // Note Off
                mv->notestackPop(note);

        } else { // Voice Control
            if( velocity ) { // Note On
                // copy velocity into mod matrix source
                knobSet(SID_KNOB_VELOCITY, velocity << 1);

                noteOn(midiVoice, note, velocity, false);
            } else { // Note Off
                noteOff(midiVoice, note, false);
            }
        }
    }


    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    if( mbSidVoice[0].midiVoicePtr->isAssigned(chn) ) {
        switch( ccNumber ) {
        case  1: knobSet(SID_KNOB_1, value << 1); break;
        case 16: knobSet(SID_KNOB_2, value << 1); break;
        case 17: knobSet(SID_KNOB_3, value << 1); break;
        case 18: knobSet(SID_KNOB_4, value << 1); break;
        case 19: knobSet(SID_KNOB_5, value << 1); break;
        case 123: if( value == 0 ) noteAllOff(0, false); break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    u16 pitchbendValue8bit = pitchbendValue14bit >> 6;

    MbSidMidiVoice *mv = mbSidVoice[0].midiVoicePtr;
    for(int midiVoice=0; midiVoice<2; ++midiVoice, ++mv)
        if( mv->isAssigned(chn) ) {
            mv->midivoicePitchbender = pitchbendValue8bit;
            knobSet(SID_KNOB_PITCHBENDER, pitchbendValue8bit);
        }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbSidVoice[0].midiVoicePtr->isAssigned(chn) ) {
        knobSet(SID_KNOB_AFTERTOUCH, value << 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack)
{
    if( instrument >= 2 )
        return;

    u8 seq = instrument;
    u8 arp = instrument;
    u8 voice = 3*instrument;
    MbSidVoice *v = &mbSidVoice[voice];
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // push note into WT stack
    if( !bypassNotestack )
        mv->wtstackPush(note);

    // branch depending on normal/arp/seq mode
    if( mbSidSeqBassline[seq].seqEnabled ) {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // reset sequencer if voice was not active before
        // do this with both voices for proper synchronisation!
        // (only done in Clock Master mode)
        // ensure that this is only done for instruments where WTO (wavetable/sequencer only) is enabled
        if( !mbSidClockPtr->clockSlaveMode ) {
            if( !mbSidVoice[0].voiceActive )
                mbSidSeqBassline[0].seqRestartReq = 1;
            if( !mbSidVoice[3].voiceActive )
                mbSidSeqBassline[1].seqRestartReq = 1;
        }

        // change sequence
        seqChangePattern(instrument, note);

        // always set voice active flag of both voices to ensure that they are in sync
        // ensure that this is only done for instruments where sequencer is enabled
        if( mbSidSeqBassline[0].seqEnabled )
            mbSidVoice[0].voiceActive = 1;
        if( mbSidSeqBassline[1].seqEnabled )
            mbSidVoice[3].voiceActive = 1;

    } else if( mbSidArp[arp].arpEnabled ) {
        mbSidArp[arp].noteOn(v, note, velocity);
    } else {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // switch off gate if not in legato or WTO mode
        if( !v->voiceLegato && !v->voiceWavetableOnly )
            v->gateOff(note);

        // call Note On Handler
        if( v->noteOn(note, velocity) )
            triggerNoteOn(v, 0);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Disable a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::noteOff(u8 instrument, u8 note, bool bypassNotestack)
{
    if( instrument >= 2 )
        return;

    u8 seq = instrument;
    u8 arp = instrument;
    u8 voice = 3*instrument;
    MbSidVoice *v = &mbSidVoice[voice];
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // pop from WT stack if sustain not active (TODO: sustain switch)
    if( !bypassNotestack )
        mv->wtstackPop(note);

    // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
    // if not in arp mode: sustain only relevant if only one active note in stack

    // branch depending on normal/arp/seq mode
    if( mbSidSeqBassline[seq].seqEnabled ) {
        // pop note from stack
        if( mv->notestackPop(note) > 0 ) {
            // change sequence if there is still a note in stack
            if( mv->midivoiceNotestack.len )
                seqChangePattern(instrument, mv->midivoiceNotestack.note_items[0].note);

            // disable voice active flag of both voices if both are playing invalid sequences (seq off)
            // only used in master mode
            if( !mbSidClockPtr->clockSlaveMode ) {
                if( mbSidSeqBassline[0].seqPatternNumber >= 8 &&
                    mbSidSeqBassline[1].seqPatternNumber >= 8 ) {
                    // this should only be done for instruments where WTO (sequence) is selected
                    if( mbSidSeqBassline[0].seqEnabled )
                        mbSidVoice[0].voiceActive = 0;
                    if( mbSidSeqBassline[1].seqEnabled )
                        mbSidVoice[3].voiceActive = 0;
                }
            }
        }
    } else if( mbSidArp[arp].arpEnabled ) {
        mbSidArp[arp].noteOff(v, note);
    } else {
        if( bypassNotestack ) {
            v->noteOff(note, 0);
            triggerNoteOff(v, 0);
        } else {
            u8 lastFirstNote = mv->midivoiceNotestack.note_items[0].note;
            // pop note from stack
            if( mv->notestackPop(note) > 0 ) {
                // call Note Off Handler
                if( v->noteOff(note, lastFirstNote) > 0 ) { // retrigger requested?
                    if( v->noteOn(mv->midivoiceNotestack.note_items[0].note, mv->midivoiceNotestack.note_items[0].tag) ) // new note
                        triggerNoteOn(v, 0);
                } else {
                    triggerNoteOff(v, 0);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::noteAllOff(u8 instrument, bool bypassNotestack)
{
    if( instrument >= 2 )
        return;

    u8 voice = 3*instrument;
    MbSidVoice *v = &mbSidVoice[voice];

    v->voiceActive = 0;
    v->voiceGateClrReq = 1;
    v->voiceGateSetReq = 0;
    triggerNoteOff(v, 0);

    if( !bypassNotestack ) {
        v->midiVoicePtr->notestackReset();
        v->midiVoicePtr->wtstackReset();
    }
}


/////////////////////////////////////////////////////////////////////////////
// Bassline Sequencer: change pattern on keypress
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::seqChangePattern(u8 instrument, u8 note)
{
    u8 voice = instrument*3;

    // add MIDI voice based transpose value
    int transposedNote = (s32)note + (s32)mbSidVoice[voice].midiVoicePtr->midivoiceTranspose - 64;

    // octavewise saturation if note is negative
    while( transposedNote < 0 )
        transposedNote += 12;

    // determine pattern based on note number
    u8 patternNumber = transposedNote % 12;

    // if number >= 8: set to 8 (sequence off)
    if( patternNumber >= 8 )
        patternNumber = 8;

    // set new sequence
    mbSidSeqBassline[instrument].seqPatternNumber = patternNumber;

    // also in patch
    sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatchPtr->body.B.voice[instrument][0];
    sid_se_seq_patch_t *seqPatch = (sid_se_seq_patch_t *)&voicePatch->B.seq[0];
    seqPatch->num = patternNumber;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::knobSet(u8 knob, u8 value)
{
    // store new value into patch
    mbsid_knob_t *k = (mbsid_knob_t *)&mbSidPatchPtr->body.knob[knob];
    k->value = value;

    // get scaled value between min/max boundaries
    s32 factor = ((k->max > k->min) ? (k->max - k->min) : (k->min - k->max)) + 1;
    s32 knobScaledValue = k->min + (value * factor); // 16bit

    u8 sidlr = 3; // always modify both SIDs
    u8 ins = 0; // instrument n/a
    bool scaleFrom16bit = true;

    // forward to parameter handler
    // interrupts should be disabled to ensure atomic access
    MIOS32_IRQ_Disable();
    parSet(k->assign1, knobScaledValue, sidlr, ins, scaleFrom16bit);
    parSet(k->assign2, knobScaledValue, sidlr, ins, scaleFrom16bit);
    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// returns the current knob value
/////////////////////////////////////////////////////////////////////////////
u8 MbSidSeBassline::knobGet(u8 knob)
{
    mbsid_knob_t *k = (mbsid_knob_t *)&mbSidPatchPtr->body.knob[knob];
    return k->value;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// value contains unscaled value (range depends on parameter)
// sidlr selects left/right SID channel (0..1)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
// scaleFrom16bit optionally allows to input 16bit values instead of unscaled value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit)
{
    u8 voiceSel;
    u8 insSel;
    switch( par & 3 ) {
    case 1: insSel = sidlr; voiceSel = (sidlr&1) | ((sidlr&2)<<2); break; // selected voices
    case 2: insSel = 0; voiceSel = 1; break; // first voice only
    case 3: insSel = 1; voiceSel = 0x08; break; // second voice only
    default: insSel = 3; voiceSel = 0x09; break; // both voices
    }

    MbSidVoice *v = mbSidVoice.first();

    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            if( scaleFrom16bit ) value >>= 9;
            if( sidlr & 1 ) mbSidFilter[0].filterVolume = value;
            if( sidlr & 2 ) mbSidFilter[1].filterVolume = value;
            break;

        case 0x03: // OSC Detune
            if( scaleFrom16bit ) value >>= 8;
            sysexSetParameter(0x51, value);
            break;

        case 0x04: // Filter CutOff
            if( scaleFrom16bit ) value >>= 4;
            if( sidlr & 1 ) mbSidFilter[0].filterCutoff = value;
            if( sidlr & 2 ) mbSidFilter[1].filterCutoff = value;
            break;

        case 0x05: // Filter Resonance
            if( scaleFrom16bit ) value >>= 8;
            if( sidlr & 1 ) mbSidFilter[0].filterResonance = value;
            if( sidlr & 2 ) mbSidFilter[1].filterResonance = value;
            break;

        case 0x06: // Filter Channels
            if( scaleFrom16bit ) value >>= 12;
            if( sidlr & 1 ) mbSidFilter[0].filterChannels = value;
            if( sidlr & 2 ) mbSidFilter[1].filterChannels = value;
            break;

        case 0x07: // Filter Mode
            if( scaleFrom16bit ) value >>= 12;
            if( sidlr & 1 ) mbSidFilter[0].filterMode = value;
            if( sidlr & 2 ) mbSidFilter[1].filterMode = value;
            break;
        }
    } else if( par <= 0x0f ) { // Knobs
        if( scaleFrom16bit ) value >>= 8;
        knobSet(par & 0x07, value);
    } else if( par <= 0x1f ) {
        if( par <= 0x17 ) {
            // External Parameters (CV): TODO
        } else {
            // External Switches: TODO
        }
    } else if( par <= 0x5f ) { // Voice related parameters
        switch( par & 0xfc ) {
        case 0x20: { // Waveform
            if( scaleFrom16bit ) value >>= 9;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) ) {
                    sid_se_voice_waveform_t waveform;
                    waveform.ALL = value;
                    v->voiceWaveformOff = waveform.VOICE_OFF;
                    v->voiceWaveformSync = waveform.SYNC;
                    v->voiceWaveformRingmod = waveform.RINGMOD;
                    v->voiceWaveform = waveform.WAVEFORM;
                }
        } break;

        case 0x24: // Transpose
            if( scaleFrom16bit ) value >>= 9;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceTranspose = value;
            break;

        case 0x28: // Finetune
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceFinetune = value;
            break;

        case 0x2c: // Portamento
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voicePortamentoRate = value;
            break;

        case 0x30: // Pulsewidth
            if( scaleFrom16bit ) value >>= 4;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voicePulsewidth = value;
            break;

        case 0x34: // Delay
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceDelay = value;
            break;

        case 0x38: // Attack
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceAttackDecay.ATTACK = value;
            break;

        case 0x3c: // Decay
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceAttackDecay.DECAY = value;
            break;

        case 0x40: // Sustain
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceSustainRelease.SUSTAIN = value;
            break;

        case 0x44: // Release
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( voiceSel & (1 << voice) )
                    v->voiceSustainRelease.RELEASE = value;
            break;

        case 0x48: { // Arp Speed
            if( scaleFrom16bit ) value >>= 10;
            MbSidArp *a = mbSidArp.first();
            for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
                if( insSel & (1 << arp) )
                    a->arpSpeed = value;
        } break;

        case 0x4c: { // Arp Gatelength
            if( scaleFrom16bit ) value >>= 11;
            MbSidArp *a = mbSidArp.first();
            for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
                if( insSel & (1 << arp) )
                    a->arpGatelength = value;
        } break;

        case 0x50: // Pitchbender
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; voice+=3, v+=3)
                if( insSel & (1 << voice) )
                    v->midiVoicePtr->midivoicePitchbender = value;
            break;
        }
    } else if( par <= 0x7f ) { // Extra Bassline parameters for Knob Assignments
        switch( par & 0xfc ) {
        case 0x60: // CutOff
            if( scaleFrom16bit ) value >>= 4;
            if( insSel & 1 ) mbSidFilter[0].filterCutoff = value;
            if( insSel & 2 ) mbSidFilter[1].filterCutoff = value;
            break;
        case 0x64: // Resonance
            if( scaleFrom16bit ) value >>= 8;
            if( insSel & 1 ) mbSidFilter[0].filterResonance = value;
            if( insSel & 2 ) mbSidFilter[1].filterResonance = value;
            break;
        case 0x68: // EnvMod
            if( scaleFrom16bit ) value >>= 8;
            if( insSel & 1 ) mbSidEnv[0].envDepthFilter = (s32)value - 0x80;
            if( insSel & 2 ) mbSidEnv[1].envDepthFilter = (s32)value - 0x80;
            break;
        case 0x6c: // EnvDecay
            if( scaleFrom16bit ) value >>= 8;
            if( insSel & 1 ) mbSidEnv[0].envDecay = value;
            if( insSel & 2 ) mbSidEnv[1].envDecay = value;
            break;
        case 0x70: // Accent
            if( scaleFrom16bit ) value >>= 8;
            if( insSel & 1 ) mbSidVoice[0].voiceAccentRate = value;
            if( insSel & 2 ) mbSidVoice[3].voiceAccentRate = value;
            break;
        }
    } else if( par <= 0xbf ) { // LFOs
        u8 lfoSel = (insSel & 1) | ((insSel&2)<<1);
        if( par & 4 )
            lfoSel |= (lfoSel << 1);
        MbSidLfo *l = mbSidLfo.first();

        switch( par & 0xf8 ) {
        case 0x80: // LFO Waveform
            if( scaleFrom16bit ) value >>= 12;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoMode.WAVEFORM = value;
            break;

        case 0x88: // LFO Depth Pitch
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoDepthPitch = (s32)value - 0x80;
            break;

        case 0x90: // LFO Depth Pulsewidth
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoDepthPulsewidth = (s32)value - 0x80;
            break;

        case 0x98: // LFO Depth Filter
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoDepthFilter = (s32)value - 0x80;
            break;

        case 0xa0: // LFO Rate
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoRate = value;
            break;

        case 0xa8: // LFO Delay
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoDelay = value;
            break;

        case 0xb0: // LFO Phase
            if( scaleFrom16bit ) value >>= 8;
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                if( lfoSel & (1 << lfo) )
                    l->lfoDelay = value;
            break;
        }
    } else if( par <= 0xe7 ) { // ENV
        MbSidEnv *e = mbSidEnv.first();
        if( scaleFrom16bit ) value >>= 8;

        switch( par & 0xfc ) {
        case 0xc0: // ENV Depth Pitch
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envDepthPitch = (s32)value - 0x80;
            break;

        case 0xc4: // ENV Depth Pulsewidth
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envDepthPulsewidth = (s32)value - 0x80;
            break;

        case 0xc8: // ENV Depth Filter
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envDepthFilter = (s32)value - 0x80;
            break;

        case 0xcc: // ENV Attack
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envAttack = value;
            break;

        case 0xd0: // ENV Decay
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envDecay = value;
            break;

        case 0xd4: // ENV Sustain
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envSustain = value;
            break;

        case 0xd8: // ENV Release
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envRelease = value;
            break;

        case 0xdc: // ENV Curve
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envCurve = value;
            break;

        case 0xe0: // ENV Decay Accented
            for(int env=0; env<mbSidEnv.size; ++env, ++e)
                if( insSel & (1 << env) )
                    e->envDecayAccented = value;
            break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Returns a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// sidlr selects left/right SID channel (if 0, 1 or 3, SIDL register will be returned, with 2 the SIDR register)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
// scaleTo16bit optionally allows to return 16bit values instead of unscaled value
/////////////////////////////////////////////////////////////////////////////
u16 MbSidSeBassline::parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit)
{
    u16 value = 0;

    // note: input parameter "ins" will be overwritten - thats ok for Bassline Engine!
    ins = (!(sidlr & 1)) ? 1 : 0;
    u8 voice = (!(sidlr & 1)) ? 2 : 1;
    MbSidVoice *v = &mbSidVoice[voice];

    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            value = mbSidFilter[ins].filterVolume;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0x03: // OSC Detune
            value = v->voiceDetuneDelta;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x04: // Filter CutOff
            value = mbSidFilter[ins].filterCutoff;
            if( scaleTo16bit ) value <<= 4;
            break;

        case 0x05: // Filter Resonance
            value = mbSidFilter[ins].filterResonance;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x06: // Filter Channels
            value = mbSidFilter[ins].filterChannels;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x07: // Filter Mode
            value = mbSidFilter[ins].filterMode;
            if( scaleTo16bit ) value <<= 12;
            break;
        }
    } else if( par <= 0x0f ) { // Knobs
        knobGet(par & 0x07);
        if( scaleTo16bit ) value <<= 8;
    } else if( par <= 0x1f ) {
        if( par <= 0x17 ) {
            // External Parameters (CV): TODO
        } else {
            // External Switches: TODO
        }
    } else if( par <= 0x5f ) { // Voice related parameters
        switch( par & 0xfc ) {
        case 0x20: { // Waveform
            sid_se_voice_waveform_t waveform;
            waveform.ALL = 0;
            waveform.VOICE_OFF = v->voiceWaveformOff;
            waveform.SYNC = v->voiceWaveformSync;
            waveform.RINGMOD = v->voiceWaveformRingmod;
            waveform.WAVEFORM = v->voiceWaveform;
            if( scaleTo16bit ) value <<= 9;
        } break;

        case 0x24: // Transpose
            value = v->voiceTranspose;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0x28: // Finetune
            value = v->voiceFinetune;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x2c: // Portamento
            value = v->voicePortamentoRate;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x30: // Pulsewidth
            value = v->voicePulsewidth;
            if( scaleTo16bit ) value <<= 4;
            break;

        case 0x34: // Delay
            value = v->voiceDelay;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x38: // Attack
            value = v->voiceAttackDecay.ATTACK;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x3c: // Decay
            value = v->voiceAttackDecay.DECAY;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x40: // Sustain
            value = v->voiceSustainRelease.SUSTAIN;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x44: // Release
            value = v->voiceSustainRelease.RELEASE;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x48: { // Arp Speed
            value = mbSidArp[ins].arpSpeed;
            if( scaleTo16bit ) value <<= 10;
        } break;

        case 0x4c: { // Arp Gatelength
            value = mbSidArp[ins].arpGatelength;
            if( scaleTo16bit ) value <<= 11;
        } break;

        case 0x50: // Pitchbender
            value = v->midiVoicePtr->midivoicePitchbender;
            if( scaleTo16bit ) value <<= 8;
            break;
        }
    } else if( par <= 0x7f ) { // Extra Bassline parameters for Knob Assignments
        switch( par & 0xfc ) {
        case 0x60: // CutOff
            value = mbSidFilter[ins].filterCutoff;
            if( scaleTo16bit ) value <<= 4;
            break;
        case 0x64: // Resonance
            value = mbSidFilter[ins].filterResonance;
            if( scaleTo16bit ) value <<= 8;
            break;
        case 0x68: // EnvMod
            value = (s32)mbSidEnv[ins].envDepthFilter + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;
        case 0x6c: // EnvDecay
            value = mbSidEnv[ins].envDecay;
            if( scaleTo16bit ) value <<= 8;
            break;
        case 0x70: // Accent
            value = mbSidVoice[voice].voiceAccentRate;
            if( scaleTo16bit ) value <<= 8;
            break;
        }
    } else if( par <= 0xbf ) { // LFOs
        u8 lfo = (par & 4) ? 1 : 0;
        if( ins == 1 )
            lfo <<= 2;
        MbSidLfo *l = &mbSidLfo[lfo];

        switch( par & 0xf8 ) {
        case 0x80: // LFO Waveform
            value = l->lfoMode.WAVEFORM;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x88: // LFO Depth Pitch
            value = (s32)l->lfoDepthPitch + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x90: // LFO Depth Pulsewidth
            value = (s32)l->lfoDepthPulsewidth + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x98: // LFO Depth Filter
            value = (s32)l->lfoDepthFilter + 0x80;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0xa0: // LFO Rate
            value = l->lfoRate;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0xa8: // LFO Delay
            value = l->lfoDelay;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0xb0: // LFO Phase
            value = l->lfoPhase;
            if( scaleTo16bit ) value <<= 8;
            break;
        }
    } else if( par <= 0xe7 ) { // ENV
        MbSidEnv *e = &mbSidEnv[ins];

        switch( par & 0xfc ) {
        case 0xc0: value = (s32)e->envDepthPitch + 0x80; break;
        case 0xc4: value = (s32)e->envDepthPulsewidth + 0x80; break;
        case 0xc8: value = (s32)e->envDepthFilter + 0x80; break;
        case 0xcc: value = e->envAttack; break;
        case 0xd0: value = e->envDecay; break;
        case 0xd4: value = e->envSustain; break;
        case 0xd8: value = e->envRelease; break;
        case 0xdc: value = e->envCurve; break;
        case 0xe0: value = e->envDecayAccented; break;
        }

        if( scaleTo16bit ) value <<= 8;
    }

    return value;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from NRPN value
// sid selects the SID
// addr_[lsb|msb] selects parameter
// data_[lsb|msb] contains value
// value contains 8bit scaled value (0..255)
// sidlr selects left/right SID channel
// ins selects multi/drum instrument
/////////////////////////////////////////////////////////////////////////////
void MbSidSeBassline::parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins)
{
    return;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to set a dedicated SysEx parameter
// (forwarded from MbSidEnvironment and MbSid)
// returns false on invalid access
//
// This function is also used by init() to map the patch to sound engine 
// objects so that we avoid redundancies
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeBassline::sysexSetParameter(u16 addr, u8 data)
{
    if( addr <= 0x0f ) { // Patch Name
        return true; // no update required
    } else if( addr <= 0x012 ) { // global section
        if( addr == 0x012 ) { // opt_flags
            sid_se_opt_flags_t optFlags;
            optFlags.ALL = data;
            for(MbSidVoice *v = mbSidVoice.first(); v != NULL ; v=mbSidVoice.next(v)) {
                v->voiceAdsrBugWorkaround = optFlags.ABW;
            }
        }
        return true;
    } else if( addr <= 0x04f ) { // Custom Switches, Knobs and External Parameters (CV)
        return true; // already handled by MbSid before this function has been called
    } else if( addr <= 0x053 ) { // Engine specific parameters
        if( addr == 0x51 ) {
            for(MbSidVoice *v = mbSidVoice.first(); v != NULL ; v=mbSidVoice.next(v)) {
                v->voiceDetuneDelta = 0;
                if( data ) {
                    switch( v->voiceNum ) {
                    case 0: v->voiceDetuneDelta = +data/4; break; // makes only sense on stereo sounds
                    case 3: v->voiceDetuneDelta = -data/4; break; // makes only sense on stereo sounds

                    case 1:
                    case 5: v->voiceDetuneDelta = +data; break;

                    case 2:
                    case 4: v->voiceDetuneDelta = -data; break;
                    }
                }
            }
        } else if( addr == 0x52 ) {
            for(MbSidFilter *f = mbSidFilter.first(); f != NULL ; f=mbSidFilter.next(f))
                f->filterVolume = data;
        }
        return true;
    } else if( addr <= 0x05f ) { // filters
        u8 filter = (addr >= 0x05a) ? 1 : 0;
        MbSidFilter *f = &mbSidFilter[filter];

        switch( (addr - 0x54) % 6 ) {
        case 0x0: f->filterChannels = data & 0x0f; f->filterMode = data >> 4; break;
        case 0x1: f->filterCutoff = (f->filterCutoff & 0xff00) | data; break;
        case 0x2: f->filterCutoff = (f->filterCutoff & 0x00ff) | ((data & 0x0f) << 8); break;
        case 0x3: f->filterResonance = data; break;
        case 0x4: f->filterKeytrack = data; break;
        }
        return true;
    } else if( addr <= 0x0ff ) { // voices
        u8 instrument = (addr >= 0xb0) ? 1 : 0;
        u8 voice = instrument*3;
        MbSidVoice *v = &mbSidVoice[voice];
        u8 voiceAddr = (addr - 0x60) % 0x50;

        if( voiceAddr <= 0x0f ) { // common voice part
            switch( voiceAddr ) {
            case 0x0: {
                sid_se_voice_flags_t voiceFlags;
                voiceFlags.ALL = data;
                v->voiceConstantTimeGlide = voiceFlags.PORTA_MODE == 1;
                v->voiceGlissandoMode = voiceFlags.PORTA_MODE == 2;
                v->voiceGateStaysActive = voiceFlags.GSA;
            } break;
            
            case 0x1: {
                sid_se_voice_waveform_t waveform;
                waveform.ALL = data;
                v->voiceWaveformOff = waveform.VOICE_OFF;
                v->voiceWaveformSync = waveform.SYNC;
                v->voiceWaveformRingmod = waveform.RINGMOD;
                v->voiceWaveform = waveform.WAVEFORM;
            } break;
            
            case 0x2: v->voiceAttackDecay.ALL = data; break;
            case 0x3: v->voiceSustainRelease.ALL = data; break;
            case 0x4: v->voicePulsewidth = (v->voicePulsewidth & 0xff00) | data; break;
            case 0x5: v->voicePulsewidth = (v->voicePulsewidth & 0x00ff) | ((data & 0x0f) << 8); break;
            case 0x6: v->voiceAccentRate = data; break;
            case 0x7: v->voiceDelay = data; break;
            case 0x8: v->voiceTranspose = data; break;
            case 0x9: v->voiceFinetune = data; break;
            case 0xa: v->voicePitchrange = data; break;
            case 0xb: v->voicePortamentoRate = data; break;
            case 0xc: {
                MbSidArp *a = &mbSidArp[voice];
                sid_se_voice_arp_mode_t mode;
                mode.ALL = data;
                a->arpEnabled = mode.ENABLE;
                a->arpDown = mode.DIR & 1;
                a->arpUpAndDown = mode.DIR >= 2 && mode.DIR <= 5;
                a->arpPingPong = mode.DIR >= 2 && mode.DIR <= 3;
                a->arpRandomNotes = mode.DIR >= 6;
                a->arpSortedNotes = mode.SORTED;
                a->arpHoldMode = mode.HOLD;
                a->arpSyncMode = mode.SYNC;
                a->arpConstantCycle = mode.CAC;
            } break;
            case 0xd: {
                MbSidArp *a = &mbSidArp[voice];
                sid_se_voice_arp_speed_div_t speedDiv;
                speedDiv.ALL = data;
                a->arpEasyChordMode = speedDiv.EASY_CHORD;
                a->arpOneshotMode = speedDiv.ONESHOT;
                a->arpSpeed = speedDiv.DIV;
            } break;
            case 0xe: {
                MbSidArp *a = &mbSidArp[voice];
                sid_se_voice_arp_gl_rng_t glRng;
                glRng.ALL = data;
                a->arpGatelength = glRng.GATELENGTH;
                a->arpOctaveRange = glRng.OCTAVE_RANGE;
            } break;
            case 0xf: v->voiceSwinSidMode = data; break;
            }
        } else if( voiceAddr == 0x10 ) { // voice specific v_flags
            sid_se_v_flags_t globalVoiceFlags;
            globalVoiceFlags.ALL = data;
            v->voiceLegato = globalVoiceFlags.LEGATO;
            v->voiceWavetableOnly = globalVoiceFlags.WTO;
            v->voiceSusKey = globalVoiceFlags.SUSKEY;
            v->voicePoly = globalVoiceFlags.POLY;
            v->voiceOscPhase = globalVoiceFlags.OSC_PHASE_SYNC; // either 0 or 1 supported, no free definable phase

            // special for Bassline
            mbSidSeqBassline[instrument].seqEnabled = v->voiceWavetableOnly;
        } else if( voiceAddr <= 0x21 ) { // LFOs
            u8 lfo = 2*instrument + ((voiceAddr >= 0x2b) ? 1 : 0);
            MbSidLfo *l = &mbSidLfo[lfo];
            u8 lfoAddr = (voiceAddr - 0x14) % 7;
            switch( lfoAddr ) {
            case 0: l->lfoMode.ALL = data; break;
            case 1: l->lfoDepthPitch = (s32)data - 0x80; break;
            case 2: l->lfoRate = data; break;
            case 3: l->lfoDelay = data; break;
            case 4: l->lfoPhase = data; break;
            case 5: l->lfoDepthPulsewidth = (s32)data - 0x80; break;
            case 6: l->lfoDepthFilter = (s32)data - 0x80; break;
            }
        } else if( voiceAddr <= 0x2a ) { // ENV
            u8 env = instrument;
            MbSidEnv *e = &mbSidEnv[env];
            u8 envAddr = (voiceAddr - 0x22);
            switch( envAddr ) {
            case 0x0: e->envMode.ALL = data; break;
            case 0x1: e->envDepthPitch = (s32)data - 0x80; break;
            case 0x2: e->envDepthPulsewidth = (s32)data - 0x80; break;
            case 0x3: e->envDepthFilter = (s32)data - 0x80; break;
            case 0x4: e->envAttack = data; break;
            case 0x5: e->envDecay = data; break;
            case 0x6: e->envSustain = data; break;
            case 0x7: e->envRelease = data; break;
            case 0x8: e->envCurve = data; break;
            }
        } else if( voiceAddr <= 0x2f ) { // Sequencer
            u8 seq = instrument;
            MbSidSeqBassline *s = &mbSidSeqBassline[seq];
            u8 seqAddr = (voiceAddr - 0x2b);
            switch( seqAddr ) {
            case 0: s->seqClockDivider = data & 0x3f; s->seqSynchToMeasure = (data >> 7); break;
            case 1: s->seqPatternNumber = data & 0x0f; break;
            case 2: s->seqPatternLength = data & 0x0f; break;
            case 3: s->seqParameterAssign = data; break;
            }
        } else if( addr == 0x30 ) {
            mbSidEnv[instrument].envDecayAccented = data;
        } else if( addr <= 0x3f ) {
            // huge reserved space :)
        } else { // OSC2/3 section
            u8 oscAddr = voiceAddr & 0x07;
            u8 voiceOffset = (voiceAddr >= 0x48) ? 2 : 1;
            v = &mbSidVoice[voice+voiceOffset];

            switch( oscAddr ) {
            case 0x0: {
                sid_se_voice_waveform_t waveform;
                waveform.ALL = data;
                v->voiceWaveformOff = waveform.VOICE_OFF;
                v->voiceWaveformSync = waveform.SYNC;
                v->voiceWaveformRingmod = waveform.RINGMOD;
                v->voiceWaveform = waveform.WAVEFORM;
            } break;
            case 0x1: v->voicePulsewidth = (v->voicePulsewidth & 0xff00) | data; break;
            case 0x2: v->voicePulsewidth = (v->voicePulsewidth & 0x00ff) | ((data & 0x0f) << 8); break;
            case 0x3:
                if( data & 4 )
                    v->voiceTranspose = 0x40 - 12*(4-(data & 3));
                else
                    v->voiceTranspose = 0x40 + 12*(data & 3);
                break;
            case 0x4: v->voiceForcedNote = data & 0x7f; break;
            }
        }

        return true;
    } else if( addr <= 0x1ff ) {
        return true; // no update required
    }

    // unsupported sysex address
    return false;
}
