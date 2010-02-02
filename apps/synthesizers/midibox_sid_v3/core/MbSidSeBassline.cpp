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
    for(int addr=0x10; addr<0x180; ++addr) // reduced range... loop over patch name and WT memory not required
        sysexSetParameter(addr, mbSidPatchPtr->body.ALL[addr]);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeBassline::tick(const u8 &updateSpeedFactor)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatchPtr->body.engine;

    // clear modulation destinations
    mbSidVoice[0].voicePitchModulation = 0;
    mbSidVoice[0].voicePulsewidthModulation = 0;
    mbSidVoice[3].voicePitchModulation = 0;
    mbSidVoice[3].voicePulsewidthModulation = 0;
    mbSidFilter[0].filterCutoffModulation = 0;
    mbSidFilter[1].filterCutoffModulation = 0;


    // Clock
    if( mbSidClockPtr->eventStart ) {
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
        u16 oldOut = l->lfoOut;
        l->tick(updateSpeedFactor);

        if( l->lfoOut != oldOut ) {
            u8 voice = (lfo >= 2) ? 3 : 0;
            mbSidVoice[voice].voicePitchModulation += (l->lfoOut * ((s32)l->lfoDepthPitch-0x80)) / 128;
            mbSidVoice[voice].voicePulsewidthModulation += (l->lfoOut * ((s32)l->lfoDepthPulsewidth-0x80)) / 128;
            u8 filter = (lfo >= 2) ? 1 : 0;
            mbSidFilter[filter].filterCutoffModulation += (l->lfoOut * ((s32)l->lfoDepthFilter-0x80)) / 128;
        }
    }

    // ENVs
    MbSidEnv *e = mbSidEnv.first();
    for(int env=0; env < mbSidEnv.size; ++env, ++e) {
        e->tick(updateSpeedFactor);

        u8 voice = 3*env;
        mbSidVoice[voice].voicePitchModulation += (e->envOut * ((s32)e->envDepthPitch-0x80)) / 128;
        mbSidVoice[voice].voicePulsewidthModulation += (e->envOut * ((s32)e->envDepthPulsewidth-0x80)) / 128;
        u8 filter = env;
        mbSidFilter[filter].filterCutoffModulation += (e->envOut * ((s32)e->envDepthFilter-0x80)) / 128;
    }

    // Arps, Sequencers and Voices
    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; voice+=3, v+=3) {
        if( !mbSidArp[voice/3].arpEnabled )
            mbSidSeqBassline[voice/3].tick(v, this);
        else
            mbSidArp[voice/3].tick(v, this);

        if( v->gate(engine, updateSpeedFactor, this) > 0 )
            v->pitch(engine, updateSpeedFactor, this);
        v->pw(engine, updateSpeedFactor, this);

        v->physSidVoice->waveform = v->voiceWaveform;
        v->physSidVoice->sync = v->voiceWaveformSync;
        v->physSidVoice->ringmod = v->voiceWaveformRingmod;

        // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
        if( !v->voiceSetDelayCtr ) {
            v->physSidVoice->ad = v->voiceAttackDecay.ALL;
            v->physSidVoice->sr = v->voiceSustainRelease.ALL;
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

    if( mbSidLfo[lfoOffset+0].lfoKeySync )
        mbSidLfo[lfoOffset+0].restartReq = 1;
    if( mbSidLfo[lfoOffset+1].lfoKeySync )
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

                noteOn(0, note, velocity, false);
            } else { // Note Off
                noteOff(0, note, false);
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

    u8 arp = instrument;
    u8 voice = 3*instrument;
    MbSidVoice *v = &mbSidVoice[voice];
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // push note into WT stack
    if( !bypassNotestack )
        mv->wtstackPush(note);

    // branch depending on normal/arp mode
    if( mbSidArp[arp].arpEnabled ) {
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

    u8 arp = instrument;
    u8 voice = 3*instrument;
    MbSidVoice *v = &mbSidVoice[voice];
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // pop from WT stack if sustain not active (TODO: sustain switch)
    if( !bypassNotestack )
        mv->wtstackPop(note);

    // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
    // if not in arp mode: sustain only relevant if only one active note in stack

    // branch depending on normal/arp mode
    if( mbSidArp[arp].arpEnabled ) {
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
    // TODO
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
    // TODO
	return 0;
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
	return false;
    // TODO
}
