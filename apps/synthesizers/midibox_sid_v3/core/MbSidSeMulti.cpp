/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Multi Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidSeMulti.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeMulti::MbSidSeMulti()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeMulti::~MbSidSeMulti()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr)
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
void MbSidSeMulti::initPatch(bool patchOnly)
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

        for(int wt=0; wt<mbSidWt.size; ++wt)
            mbSidWt[wt].init();

        for(int arp=0; arp<mbSidArp.size; ++arp)
            mbSidArp[arp].init();

        // clear voice queue
        voiceQueue.init(&mbSidPatchPtr->body);
    }

    // iterate over patch to transfer all SysEx parameters into SE objects
    for(int addr=0x10; addr<0x180; ++addr) // reduced range... loop over patch name and WT memory not required
        sysexSetParameter(addr, mbSidPatchPtr->body.ALL[addr]);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeMulti::tick(const u8 &updateSpeedFactor)
{
    // clear modulation destinations
    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice<mbSidVoice.size; ++voice, ++v) {
        v->voicePitchModulation = 0;
        v->voicePulsewidthModulation = 0;
    }
    mbSidFilter[0].filterCutoffModulation = 0;
    mbSidFilter[1].filterCutoffModulation = 0;


    // Clock
    if( mbSidClockPtr->eventStart ) {
    }

    if( mbSidClockPtr->eventClock ) {
        // clock WTs
        MbSidWt *w = mbSidWt.first();
        for(int wt=0; wt<mbSidWt.size; ++wt, ++w)
            w->clockReq = true;

        // clock ARPs
        MbSidArp *a = mbSidArp.first();
        for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
            a->clockReq = true;

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

        u8 voice = lfo/2;
        u8 filter = lfo/6;
        mbSidVoice[voice].voicePitchModulation += (l->lfoOut * (s32)l->lfoDepthPitch) / 128;
        mbSidVoice[voice].voicePulsewidthModulation += (l->lfoOut * (s32)l->lfoDepthPulsewidth) / 128;
        mbSidFilter[filter].filterCutoffModulation += (l->lfoOut * (s32)l->lfoDepthFilter) / 128;
    }

    // ENVs
    MbSidEnv *e = mbSidEnv.first();
    for(int env=0; env < mbSidEnv.size; ++env, ++e) {
        e->tick(updateSpeedFactor);

        u8 voice = env;
        u8 filter = env/3;
        mbSidVoice[voice].voicePitchModulation += (e->envOut * (s32)e->envDepthPitch) / 128;
        mbSidVoice[voice].voicePulsewidthModulation += (e->envOut * (s32)e->envDepthPulsewidth) / 128;
        mbSidFilter[filter].filterCutoffModulation += (e->envOut * (s32)e->envDepthFilter) / 128;
    }

    // WTs
    MbSidWt *w = mbSidWt.first();
    for(int wt=0; wt < mbSidWt.size; ++wt, ++w) {
        s32 step = -1;

        // if key control flag set, control position from current played key
        if( w->wtKeyControlMode ) {
            // copy currently played note to step position
            step = mbSidVoice[wt].voicePlayedNote;
        }

        w->tick(step, updateSpeedFactor);

        // check if step should be played
        if( w->wtOut >= 0 ) {
            u8 wtValue = mbSidPatchPtr->body.M.wt_memory[w->wtOut & 0x7f];

            // call parameter handler
            if( w->wtAssign ) {
                // determine SID channels which should be modified
                u8 sidlr = w->wtAssignLeftRight; // SID selection
                u8 ins = wt;

                parSetWT(w->wtAssign, wtValue, sidlr, ins);
            }
        }
    }


    // Arps
    // in difference to other engines, each MIDI voice controls
    // one arpeggiator for the last assigned voice
    MbSidMidiVoice *mv = midiVoicePtr;
    for(int midiVoice=0; midiVoice<6; ++midiVoice, ++mv) {
        MbSidVoice *v = &mbSidVoice[mv->midivoiceLastVoice];
        if( v->voiceAssignedInstrument == midiVoice )
            mbSidArp[midiVoice].tick(v, this);
    }


    // Voices
    v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
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
void MbSidSeMulti::triggerNoteOn(MbSidVoice *v, u8 no_wt)
{
    v->voiceNoteRestartReq = 1;

    u8 lfoOffset = 2*v->voiceNum;
    if( mbSidLfo[lfoOffset+0].lfoMode.KEYSYNC )
        mbSidLfo[lfoOffset+0].restartReq = 1;
    if( mbSidLfo[lfoOffset+1].lfoMode.KEYSYNC )
        mbSidLfo[lfoOffset+1].restartReq = 1;

    mbSidEnv[v->voiceNum].restartReq = 1;
}


void MbSidSeMulti::triggerNoteOff(MbSidVoice *v, u8 no_wt)
{
    mbSidEnv[v->voiceNum].restartReq = 1;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    // go through six midi voices
    MbSidMidiVoice *mv = midiVoicePtr;

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    for(int midiVoice=0; midiVoice<6; ++midiVoice, ++mv) {
        // check if MIDI channel and splitzone matches
        if( !mv->isAssigned(chn, note) )
            continue; // next MIDI voice

        if( velocity ) { // Note On
            // copy velocity into mod matrix source
            knobSet(SID_KNOB_VELOCITY, velocity << 1);

            noteOn(midiVoice, note, velocity, false);
        } else { // Note Off
            noteOff(midiVoice, note, false);
        }
    }


    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    // only for first MIDI voice
    if( midiVoicePtr->isAssigned(chn) ) {
        switch( ccNumber ) {
        case  1: knobSet(SID_KNOB_1, value << 1); break;
        case 16: knobSet(SID_KNOB_2, value << 1); break;
        case 17: knobSet(SID_KNOB_3, value << 1); break;
        case 18: knobSet(SID_KNOB_4, value << 1); break;
        case 19: knobSet(SID_KNOB_5, value << 1); break;
        }
    }

    MbSidMidiVoice *mv = midiVoicePtr;
    for(int midiVoice=0; midiVoice<6; ++midiVoice, ++mv)
        if( mv->isAssigned(chn) ) {
            switch( ccNumber ) {
            case 123: if( value == 0 ) noteAllOff(midiVoice, false); break;
            }
        }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    u16 pitchbendValue8bit = pitchbendValue14bit >> 6;

    MbSidMidiVoice *mv = midiVoicePtr;
    for(int midiVoice=0; midiVoice<6; ++midiVoice, ++mv)
        if( mv->isAssigned(chn) ) {
            mv->midivoicePitchbender = pitchbendValue8bit;
            if( midiVoice == 0 ) // only for first MIDI voice
                knobSet(SID_KNOB_PITCHBENDER, pitchbendValue8bit);
        }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::midiReceiveAftertouch(u8 chn, u8 value)
{
    // only for first MIDI voice
    if( midiVoicePtr->isAssigned(chn) )
        knobSet(SID_KNOB_AFTERTOUCH, value << 1);
}


/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack)
{
    if( instrument >= 6 )
        return;

    MbSidMidiVoice *mv = &midiVoicePtr[instrument];
    sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatchPtr->body.M.voice[instrument];
    sid_se_v_flags_t vFlags; vFlags.ALL = voicePatch->M.v_flags;

    // push note into WT stack
    if( !bypassNotestack )
        mv->wtstackPush(note);

    // branch depending on normal/arp mode
    if( mbSidArp[instrument].arpEnabled ) {
        // get new voice
        MbSidVoice *v = voiceGet(instrument, voicePatch, false);

        // TODO: dedicated velocity assignment for instrument

        mbSidArp[instrument].noteOn(v, note, velocity);
    } else {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // get new voice
        MbSidVoice *v = voiceGet(instrument, voicePatch, vFlags.POLY);

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
void MbSidSeMulti::noteOff(u8 instrument, u8 note, bool bypassNotestack)
{
    if( instrument >= 6 )
        return;

    MbSidMidiVoice *mv = &midiVoicePtr[instrument];
    sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatchPtr->body.M.voice[instrument];
    sid_se_v_flags_t vFlags; vFlags.ALL = voicePatch->M.v_flags;

    // pop from WT stack if sustain not active (TODO: sustain switch)
    if( !bypassNotestack )
        mv->wtstackPop(note);

    // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
    // if not in arp mode: sustain only relevant if only one active note in stack

    // get pointer to last voice
    u8 voice = mv->midivoiceLastVoice;
    MbSidVoice *vLast = &mbSidVoice[voice];

    // branch depending on normal/arp mode
    if( mbSidArp[instrument].arpEnabled ) {
        mbSidArp[instrument].noteOff(vLast, note);
        voiceQueue.release(voice);
    } else {
        if( bypassNotestack ) {
            vLast->noteOff(note, 0);
            voiceQueue.release(voice);
        } else {
            u8 lastFirstNote = mv->midivoiceNotestack.note_items[0].note;
            // pop note from stack
            if( mv->notestackPop(note) > 0 ) {
                // call Note Off Handler
                if( vLast->noteOff(note, lastFirstNote) > 0 ) { // retrigger requested?
                    MbSidVoice *v = voiceGet(instrument, voicePatch, vFlags.POLY);
                    if( v->noteOn(mv->midivoiceNotestack.note_items[0].note, mv->midivoiceNotestack.note_items[0].tag) ) { // new note
                        triggerNoteOn(v, 0);
                    }
                } else {
                    // turn off all voices which are assigned to the current instrument and note
                    MbSidVoice *v = mbSidVoice.first();
                    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
                        if( v->voiceAssignedInstrument == instrument && v->voicePlayedNote == note ) {
                            v->gateOff(note);
                            triggerNoteOff(v, 0);
                        }
                    }
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::noteAllOff(u8 instrument, bool bypassNotestack)
{
    if( instrument >= 6 )
        return;

    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
        if( v->voiceAssignedInstrument == instrument ) {
            v->voiceActive = 0;
            v->voiceGateClrReq = 1;
            v->voiceGateSetReq = 0;
            triggerNoteOff(v, 0);

        }
    }

    if( !bypassNotestack ) {
        midiVoicePtr[instrument].notestackReset();
        midiVoicePtr[instrument].wtstackReset();
    }
}



/////////////////////////////////////////////////////////////////////////////
// Functions to get/release a voice
/////////////////////////////////////////////////////////////////////////////
MbSidVoice *MbSidSeMulti::voiceGet(u8 instrument, sid_se_voice_patch_t *voicePatch, bool polyMode)
{
    MbSidMidiVoice *mv = &midiVoicePtr[instrument];
    
    // get voice assignment
    u8 voiceAssignment = voicePatch->M.voice_asg;

    // number of voices depends on mono/stereo mode (TODO: once ensemble available...)
    u8 numVoices = mbSidVoice.size;

    // request new voice depending on poly or mono/arp mode
    u8 voice;
    if( polyMode )
        voice = voiceQueue.get(instrument, voiceAssignment, numVoices);
    else
        voice = voiceQueue.getLast(instrument, voiceAssignment, numVoices, mv->midivoiceLastVoice);

    MbSidVoice *v = &mbSidVoice[voice];

    if( voice != mv->midivoiceLastVoice ) {
        // transfer frequency settings of last voice into new voice (for proper portamento in poly mode)
        MbSidVoice *vLast = &mbSidVoice[mv->midivoiceLastVoice];

        v->voicePrevTransposedNote = vLast->voicePrevTransposedNote;
        v->voicePortamentoEndFrq = vLast->voicePortamentoEndFrq;
        v->voiceLinearFrq = vLast->voiceLinearFrq;
        v->voiceForceFrqRecalc = 1; // take over new frequencies
    }

    // remember new voice in MIDI voice structure
    mv->midivoiceLastVoice = voice;

    // if instrument has changed
    if( v->voiceAssignedInstrument != instrument ) {
        // store pointer to MIDI voice structure and patch in voice structure
        v->voiceAssignedInstrument = instrument;
        v->midiVoicePtr = mv;

        // transfer sound parameters from patch
        u8 *voicePatchPtr = (u8 *)voicePatch;
        for(int voiceAddr=0; voiceAddr<0x30; ++voiceAddr)
            sysexSetParameterVoice(v, voiceAddr, voicePatchPtr[voiceAddr]);
    }
    return v; // return voice
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::knobSet(u8 knob, u8 value)
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
u8 MbSidSeMulti::knobGet(u8 knob)
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
void MbSidSeMulti::parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit)
{
    u8 voiceSel;
    u8 insSel;
    switch( par & 3 ) {
    case 1: insSel = sidlr; voiceSel = (sidlr&1) | ((sidlr&2)<<2); break; // selected voices
    case 2: insSel = 0; voiceSel = 1; break; // first voice only
    case 3: insSel = 1; voiceSel = 0x08; break; // second voice only
    default: insSel = 3; voiceSel = 0x09; break; // both voices
    }

    //MbSidVoice *v = mbSidVoice.first();

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
    } else { // Voice related parameters
        // TODO
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
u16 MbSidSeMulti::parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit)
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
    } else { // Voice related parameters
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
void MbSidSeMulti::parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins)
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
bool MbSidSeMulti::sysexSetParameter(u16 addr, u8 data)
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
    } else if( addr <= 0x17f ) { // voices
        u8 instrument = (addr - 0x60) / 0x30;
        u8 voiceAddr = (addr - 0x60) % 0x30;

        // only check for MIDI voice related parameters here
        switch( voiceAddr ) {
        case 0xc: {
            MbSidArp *a = &mbSidArp[instrument];
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
            MbSidArp *a = &mbSidArp[instrument];
            sid_se_voice_arp_speed_div_t speedDiv;
            speedDiv.ALL = data;
            a->arpEasyChordMode = speedDiv.EASY_CHORD;
            a->arpOneshotMode = speedDiv.ONESHOT;
            a->arpSpeed = speedDiv.DIV;
        } break;
        case 0xe: {
            MbSidArp *a = &mbSidArp[instrument];
            sid_se_voice_arp_gl_rng_t glRng;
            glRng.ALL = data;
            a->arpGatelength = glRng.GATELENGTH;
            a->arpOctaveRange = glRng.OCTAVE_RANGE;
        } break;

        case 0x11:
        case 0x12:
        case 0x13:
            // only stored in patch, not taken over
            break;

        case 0x2b: mbSidWt[instrument].wtSpeed = data & 0x3f; break;
        case 0x2c: mbSidWt[instrument].wtAssign = data; break;
        case 0x2d: mbSidWt[instrument].wtBegin = data & 0x7f; break;
        case 0x2e: mbSidWt[instrument].wtEnd = data & 0x7f; mbSidWt[instrument].wtKeyControlMode = (data & 0x80) ? true : false; break;
        case 0x2f: mbSidWt[instrument].wtLoop = data & 0x7f; mbSidWt[instrument].wtOneshotMode = (data & 0x80) ? true : false; break;

        default: {
            // update all assigned voices
            MbSidVoice *v = mbSidVoice.first();
            for(int voice=0; voice < mbSidVoice.size; ++voice, ++v)
                if( v->voiceAssignedInstrument == instrument )
                    sysexSetParameterVoice(v, voiceAddr, data);
        }
        }

        return true;
    } else if( addr <= 0x1ff ) {
        return true; // no update required
    }

    // unsupported sysex address
    return false;
}


/////////////////////////////////////////////////////////////////////////////
// Utility function to update a sound parameter of a single voice
/////////////////////////////////////////////////////////////////////////////
void MbSidSeMulti::sysexSetParameterVoice(MbSidVoice *v, u8 voiceAddr, u8 data)
{
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
        // 0xc..0xe (arp parameters) taken over outside this function, since they are assigned to MIDI voices
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
    } else if( voiceAddr <= 0x13 ) {
        // voice/velocity/pitch assignment taken over outside this function, since they are assigned to MIDI voices
    } else if( voiceAddr <= 0x21 ) { // LFOs
        u8 lfo = 2*v->voiceNum + ((voiceAddr >= 0x1b) ? 1 : 0);
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
        u8 env = v->voiceNum;
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
    }

    // 0x2b..0x2f (WT parameters) taken over outside this function, since they are assigned to MIDI voices
}

