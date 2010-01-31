/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Lead Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidSeLead.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeLead::MbSidSeLead()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSeLead::~MbSidSeLead()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initialises the sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr)
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
void MbSidSeLead::initPatch(bool patchOnly)
{
    if( !patchOnly ) {
        for(int voice=0; voice<mbSidVoice.size; ++voice)
            mbSidVoice[voice].init();

        for(int filter=0; filter<mbSidFilter.size; ++filter)
            mbSidFilter[filter].init();

        for(int lfo=0; lfo<mbSidLfo.size; ++lfo)
            mbSidLfo[lfo].init();

        for(int env=0; env<mbSidEnvLead.size; ++env)
            mbSidEnvLead[env].init();

        for(int wt=0; wt<mbSidWt.size; ++wt)
            mbSidWt[wt].init();

        for(int arp=0; arp<mbSidArp.size; ++arp)
            mbSidArp[arp].init();

        mbSidMod.init((sid_se_mod_patch_t *)&mbSidPatchPtr->body.L.mod[0]);
    }

    // iterate over patch to transfer all SysEx parameters into SE objects
    for(int addr=0x10; addr<0x180; ++addr) // reduced range... loop over patch name and WT memory not required
        sysexSetParameter(addr, mbSidPatchPtr->body.ALL[addr]);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeLead::tick(const u8 &updateSpeedFactor)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatchPtr->body.engine;

    // Clear all modulation destinations
    mbSidMod.clearDestinations();

    // Clock
    if( mbSidClockPtr->eventStart )
        triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_MST]);

    if( mbSidClockPtr->eventClock ) {
        triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_CLK]);

        // clock ARPs
        MbSidArp *a = mbSidArp.first();
        for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
            a->clockReq = true;

        // propagate clock/4 event to trigger matrix on each 6th clock
        if( mbSidClockPtr->clkCtr6 == 0 ) {
            triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_CL6]);

            // sync LFOs and ENVs
            MbSidLfo *l = mbSidLfo.first();
            for(int lfo=0; lfo<mbSidLfo.size; ++lfo, ++l)
                l->syncClockReq = true;

            MbSidEnvLead *e = mbSidEnvLead.first();
            for(int env=0; env<mbSidEnvLead.size; ++env, ++e)
                e->syncClockReq = true;
        }

        // propagate clock/16 event to trigger matrix on each 24th clock
        if( mbSidClockPtr->clkCtr24 == 0 )
            triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_C24]);
    }

    // LFOs
    MbSidLfo *l = mbSidLfo.first();
    for(int lfo=0; lfo < mbSidLfo.size; ++lfo, ++l) {
        // the rate can be modulated
        l->lfoRateModulation = mbSidMod.modDst[SID_SE_MOD_DST_LR1 + lfo];

        u16 oldOut = l->lfoOut;
        if( l->tick(updateSpeedFactor) ) // returns true on overrun
            triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_L1P + lfo]);

        if( l->lfoOut != oldOut ) {
            // scale to LFO depth
            // the depth can be modulated
            s32 lfoDepth = ((s32)l->lfoDepth - 0x80) + (mbSidMod.modDst[SID_SE_MOD_DST_LD1 + lfo] / 256);
            if( lfoDepth > 127 ) lfoDepth = 127; else if( lfoDepth < -128 ) lfoDepth = -128;
            // final LFO value
            mbSidMod.modSrc[SID_SE_MOD_SRC_LFO1 + lfo] = (l->lfoOut * lfoDepth) / 128;
        }
    }

    // ENVs
    MbSidEnvLead *e = mbSidEnvLead.first();
    for(int env=0; env < mbSidEnvLead.size; ++env, ++e) {
        if( e->tick(updateSpeedFactor) ) // returns true if sustain phase reached
            triggerLead((sid_se_trg_t *)&mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_E1S + env]);

        // scale to ENV depth
        s32 depth = ((s32)e->envDepth - 0x80);
        // final ENV value (range +/- 0x7fff)
        mbSidMod.modSrc[SID_SE_MOD_SRC_ENV1 + env] = (e->envOut * depth) / 128;
    }

    // Modulation Matrix
    // since this isn't done anywhere else:
    // convert linear frequency of voice1 to 15bit signed value (only positive direction)
    mbSidMod.modSrc[SID_SE_MOD_SRC_KEY] = mbSidVoice[0].voiceLinearFrq >> 1;

    // do ModMatrix calculations
    mbSidMod.tick();

    // Wavetables
    MbSidWt *w = mbSidWt.first();
    sid_se_wt_patch_t *wtPatch = (sid_se_wt_patch_t *)&mbSidPatchPtr->body.L.wt[0];
    for(int wt=0; wt < mbSidWt.size; ++wt, ++w, ++wtPatch) {
        s32 step = -1;

        // if key control flag (END[7]) set, control position from current played key
        if( wtPatch->end & (1 << 7) ) {
            // copy currently played note to step position
            step = mbSidVoice[0].voicePlayedNote;

        // if MOD control flag (BEGIN[7]) set, control step position from modulation matrix
        } else if( wtPatch->begin & (1 << 7) ) {
            step = ((s32)mbSidMod.modDst[SID_SE_MOD_DST_WT1 + wt] + 0x8000) >> 9; // 16bit signed -> 7bit unsigned
        }

        w->tick(step, updateSpeedFactor);

        // check if step should be played
        if( w->wtOut >= 0 ) {
            u8 wtValue = mbSidPatchPtr->body.L.wt_memory[w->wtOut & 0x7f];

            // forward to mod matrix
            if( wtValue < 0x80 ) {
                // relative value -> convert to -0x8000..+0x7fff
                mbSidMod.modSrc[SID_SE_MOD_SRC_WT1 + wt] = ((s32)wtValue - 0x40) * 512;
            } else {
                // absolute value -> convert to 0x0000..+0x7f00
                mbSidMod.modSrc[SID_SE_MOD_SRC_WT1 + wt] = ((s32)wtValue & 0x7f) * 256;
            }

            // call parameter handler
            if( wtPatch->assign ) {
                // determine SID channels which should be modified
                u8 sidlr = (wtPatch->speed >> 6); // SID selection
                u8 ins = wt;

                parSetWT(wtPatch->assign, wtValue, sidlr, ins);
            }
        }
    }

    // Arps and Voices
    MbSidVoice *v = mbSidVoice.first();
    for(int voice=0; voice < mbSidVoice.size; ++voice, ++v) {
        v->voicePitchModulation = mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + voice];
        v->voicePulsewidthModulation = mbSidMod.modDst[SID_SE_MOD_DST_PW1 + voice];

        mbSidArp[voice].tick(v, this);

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
        f->filterCutoffModulation = mbSidMod.modDst[SID_SE_MOD_DST_FIL1 + filter];
        f->filterKeytrackFrequency = mbSidVoice[filter*3].voiceLinearFrq;
        f->filterVolumeModulation = mbSidMod.modDst[SID_SE_MOD_DST_VOL1 + filter];
        f->tick(updateSpeedFactor);
    }

    // currently no detection if SIDs have to be updated
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::triggerNoteOn(MbSidVoice *v, u8 no_wt)
{
    sid_se_trg_t trg;
    u8 *trgNOn = (u8 *)mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_NOn];

    trg.ALL[0] = *trgNOn++ & (0xc0 | (1 << v->voiceNum)); // only the dedicated voice should trigger
    trg.ALL[1] = *trgNOn++;
    trg.ALL[2] = no_wt ? 0 : *trgNOn; // optionally WT triggers are masked out
    triggerLead(&trg);
}


void MbSidSeLead::triggerNoteOff(MbSidVoice *v, u8 no_wt)
{
    sid_se_trg_t trg;
    u8 *trgNOff = (u8 *)mbSidPatchPtr->body.L.trg_matrix[SID_SE_TRG_NOff];

    trg.ALL[0] = *trgNOff++ & 0xc0; // mask out all gate trigger flags
    trg.ALL[1] = *trgNOff++;
    trg.ALL[2] = no_wt ? 0 : *trgNOff; // optionally WT triggers are masked out
    triggerLead(&trg);
}


/////////////////////////////////////////////////////////////////////////////
// Input function for trigger matrix of Lead Engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::triggerLead(sid_se_trg_t *trg)
{
    if( trg->ALL[0] ) {
        if( trg->O1L ) mbSidVoice[0].voiceNoteRestartReq = true;
        if( trg->O2L ) mbSidVoice[1].voiceNoteRestartReq = true;
        if( trg->O3L ) mbSidVoice[2].voiceNoteRestartReq = true;
        if( trg->O1R ) mbSidVoice[3].voiceNoteRestartReq = true;
        if( trg->O2R ) mbSidVoice[4].voiceNoteRestartReq = true;
        if( trg->O3R ) mbSidVoice[5].voiceNoteRestartReq = true;
        if( trg->E1A ) mbSidEnvLead[0].restartReq = true;
        if( trg->E2A ) mbSidEnvLead[1].restartReq = true;
    }

    if( trg->ALL[1] ) {
        if( trg->E1R ) mbSidEnvLead[0].releaseReq = true;
        if( trg->E2R ) mbSidEnvLead[1].releaseReq = true;
        if( trg->L1  ) mbSidLfo[0].restartReq = true;
        if( trg->L2  ) mbSidLfo[1].restartReq = true;
        if( trg->L3  ) mbSidLfo[2].restartReq = true;
        if( trg->L4  ) mbSidLfo[3].restartReq = true;
        if( trg->L5  ) mbSidLfo[4].restartReq = true;
        if( trg->L6  ) mbSidLfo[5].restartReq = true;
    }

    if( trg->ALL[2] ) {
        if( trg->W1R ) mbSidWt[0].restartReq = true;
        if( trg->W2R ) mbSidWt[1].restartReq = true;
        if( trg->W3R ) mbSidWt[2].restartReq = true;
        if( trg->W4R ) mbSidWt[3].restartReq = true;
        if( trg->W1S ) mbSidWt[0].clockReq = true;
        if( trg->W2S ) mbSidWt[1].clockReq = true;
        if( trg->W3S ) mbSidWt[2].clockReq = true;
        if( trg->W4S ) mbSidWt[3].clockReq = true;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    // check if MIDI channel and splitzone matches
    if( !mbSidVoice[0].midiVoicePtr->isAssigned(chn, note) )
        return; // note filtered

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    if( velocity ) { // Note On
        // copy velocity into mod matrix source
        knobSet(SID_KNOB_VELOCITY, velocity << 1);

        noteOn(0, note, velocity, false);
    } else { // Note Off
        noteOff(0, note, false);
    }

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
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
void MbSidSeLead::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    if( mbSidVoice[0].midiVoicePtr->isAssigned(chn) ) {
        u16 pitchbendValue8bit = pitchbendValue14bit >> 6;
        mbSidVoice[0].midiVoicePtr->midivoicePitchbender = pitchbendValue8bit;
        knobSet(SID_KNOB_PITCHBENDER, pitchbendValue8bit);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbSidVoice[0].midiVoicePtr->isAssigned(chn) ) {
        knobSet(SID_KNOB_AFTERTOUCH, value << 1);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack)
{
    if( instrument == 0 ) {
        // go through all voices
        MbSidVoice *v = mbSidVoice.first();
        for(int voice=0; voice<6; ++voice, ++v) {
            MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

            // push note into WT stack
            if( !bypassNotestack )
                mv->wtstackPush(note);

            // branch depending on normal/arp mode
            if( mbSidArp[voice].arpEnabled ) {
                mbSidArp[voice].noteOn(v, note, velocity);
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
    }
}


/////////////////////////////////////////////////////////////////////////////
// Disable a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::noteOff(u8 instrument, u8 note, bool bypassNotestack)
{
    if( instrument == 0 ) {

        // go through all voices
        MbSidVoice *v = mbSidVoice.first();
        for(int voice=0; voice<6; ++voice, ++v) {
            MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

            // pop from WT stack if sustain not active (TODO: sustain switch)
            if( !bypassNotestack )
                mv->wtstackPop(note);

            // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
            // if not in arp mode: sustain only relevant if only one active note in stack

            // branch depending on normal/arp mode
            if( mbSidArp[voice].arpEnabled ) {
                mbSidArp[voice].noteOff(v, note);
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
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::noteAllOff(u8 instrument, bool bypassNotestack)
{
    if( instrument == 0 ) {
        for(MbSidVoice *v = mbSidVoice.first(); v != NULL ; v=mbSidVoice.next(v)) {
            v->voiceActive = 0;
            v->voiceGateClrReq = 1;
            v->voiceGateSetReq = 0;
            triggerNoteOff(v, 0);

            if( !bypassNotestack ) {
                v->midiVoicePtr->notestackReset();
                v->midiVoicePtr->wtstackReset();
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Sets a knob value
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::knobSet(u8 knob, u8 value)
{
    // store new value into patch
    mbsid_knob_t *k = (mbsid_knob_t *)&mbSidPatchPtr->body.knob[knob];
    k->value = value;

    // get scaled value between min/max boundaries
    s32 factor = ((k->max > k->min) ? (k->max - k->min) : (k->min - k->max)) + 1;
    s32 knobScaledValue = k->min + (value * factor); // 16bit

    // copy as signed value into modulation source array
    mbSidMod.modSrc[SID_SE_MOD_SRC_KNOB1 + knob] = (value << 8) - 0x8000;

    if( knob == SID_KNOB_1 ) {
        // copy knob1 to MDW source
        // in distance to KNOB1, this one goes only into positive direction
        mbSidMod.modSrc[SID_SE_MOD_SRC_MDW] = value << 7;
    }

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
u8 MbSidSeLead::knobGet(u8 knob)
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
void MbSidSeLead::parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit)
{
    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            if( scaleFrom16bit ) value >>= 9;
            if( sidlr & 1 ) mbSidFilter[0].filterVolume = value;
            if( sidlr & 2 ) mbSidFilter[1].filterVolume = value;
            break;

        case 0x02: // OSC Phase
            if( scaleFrom16bit ) value >>= 8;
            sysexSetParameter(0x54, value);
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
    } else if( par <= 0x5f || par >= 0xfc ) { // Voice related parameters
        u16 voiceSel = 0x3f; // select all 6 voices by default
        if( (par & 3) > 0)
            voiceSel = 0x09 << ((par&3)-1); // 2 voices
        if( !(sidlr & 1) )
            voiceSel &= ~0x7;
        if( !(sidlr & 2) )
            voiceSel &= ~0x38;
        MbSidVoice *v = mbSidVoice.first();

        switch( par & 0xfc ) {
        case 0xfc: // Note
            if( scaleFrom16bit ) value >>= 9;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) ) {
                    switch( v->playWtNote(value) ) {
                    case 1: triggerNoteOff(v, 1); break; // don't trigger WTs (wouldn't make sense here!)
                    case 2: triggerNoteOn(v, 1); break; // don't trigger WTs (wouldn't make sense here!)
                    }
                }
            break;

        case 0x20: { // Waveform
            if( scaleFrom16bit ) value >>= 9;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
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
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceTranspose = value;
            break;

        case 0x28: // Finetune
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceFinetune = value;
            break;

        case 0x2c: // Portamento
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voicePortamentoRate = value;
            break;

        case 0x30: // Pulsewidth
            if( scaleFrom16bit ) value >>= 4;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voicePulsewidth = value;
            break;

        case 0x34: // Delay
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceDelay = value;
            break;

        case 0x38: // Attack
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceAttackDecay.ATTACK = value;
            break;

        case 0x3c: // Decay
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceAttackDecay.DECAY = value;
            break;

        case 0x40: // Sustain
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceSustainRelease.SUSTAIN = value;
            break;

        case 0x44: // Release
            if( scaleFrom16bit ) value >>= 12;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->voiceSustainRelease.RELEASE = value;
            break;

        case 0x48: { // Arp Speed
            if( scaleFrom16bit ) value >>= 10;
            MbSidArp *a = mbSidArp.first();
            for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
                if( voiceSel & (1 << arp) )
                    a->arpSpeed = value;
        } break;

        case 0x4c: { // Arp Gatelength
            if( scaleFrom16bit ) value >>= 11;
            MbSidArp *a = mbSidArp.first();
            for(int arp=0; arp<mbSidArp.size; ++arp, ++a)
                if( voiceSel & (1 << arp) )
                    a->arpGatelength = value;
        } break;

        case 0x50: // Pitchbender
            if( scaleFrom16bit ) value >>= 8;
            for(int voice=0; voice<mbSidVoice.size; ++voice, ++v)
                if( voiceSel & (1 << voice) )
                    v->midiVoicePtr->midivoicePitchbender = value;
            break;

        }
    } else if( par <= 0x6f ) { // Modulation matrix
        sid_se_mod_patch_t *mp;
        mp = (sid_se_mod_patch_t *)&mbSidPatchPtr->body.L.mod[par & 7][0];
        
        if( par <= 0x67 ) {
            if( scaleFrom16bit ) value >>= 8;
            mp->depth = value;
        } else {
            if( scaleFrom16bit ) value >>= 14;
            mp->op = (mp->op & 0x3f) | (value << 6);
        }
    } else if( par <= 0xa7 ) { // LFO
        MbSidLfo *l = &mbSidLfo[par & 7];

        switch( par & 0xf8 ) {
        case 0x80: // LFO Waveform
            if( scaleFrom16bit ) value >>= 12;
            l->lfoMode.WAVEFORM = value;
            break;

        case 0x88: // LFO Depth
            if( scaleFrom16bit ) value >>= 8;
            l->lfoDepth = value;
            break;

        case 0x90: // LFO Rate
            if( scaleFrom16bit ) value >>= 8;
            l->lfoRate = value;
            break;

        case 0x98: // LFO Delay
            if( scaleFrom16bit ) value >>= 8;
            l->lfoDelay = value;
            break;

        case 0xa0: // LFO Phase
            if( scaleFrom16bit ) value >>= 8;
            l->lfoDelay = value;
            break;
        }
    } else if( par <= 0xdf ) { // ENV
        MbSidEnvLead *e = &mbSidEnvLead[par & 15];
        if( scaleFrom16bit ) value >>= 8;

        switch( par & 0xf0 ) {
        case 0x0: e->envMode.ALL = value; break;
        case 0x1: e->envDepth = value; break;
        case 0x2: e->envDelay = value; break;
        case 0x3: e->envAttack = value; break;
        case 0x4: e->envAttackLevel = value; break;
        case 0x5: e->envAttack2 = value; break;
        case 0x6: e->envDecay = value; break;
        case 0x7: e->envDecayLevel = value; break;
        case 0x8: e->envDecay2 = value; break;
        case 0x9: e->envSustain = value; break;
        case 0xa: e->envRelease = value; break;
        case 0xb: e->envReleaseLevel = value; break;
        case 0xc: e->envRelease2 = value; break;
        case 0xd: e->envAttackCurve = value; break;
        case 0xe: e->envDecayCurve = value; break;
        case 0xf: e->envReleaseCurve = value; break;
        }
    } else if( par <= 0xf3 ) { // WT
        MbSidWt *w = &mbSidWt[par & 3];

        switch( par & 0xfc ) {
        case 0xe0: // WT Speed
            if( scaleFrom16bit ) value >>= 10;
            w->wtSpeed = value;
            break;

        case 0xe4: // WT Begin
            if( scaleFrom16bit ) value >>= 9;
            w->wtBegin = value;
            break;

        case 0xe8: // WT End
            if( scaleFrom16bit ) value >>= 9;
            w->wtEnd = value;
            break;

        case 0xec: // WT Loop
            if( scaleFrom16bit ) value >>= 9;
            w->wtLoop = value;
            break;

        case 0xf0: // WT Position
            if( scaleFrom16bit ) value >>= 9;
            w->wtPos = value;
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
u16 MbSidSeLead::parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit)
{
    u16 value = 0;

    if( par <= 0x07 ) {
        switch( par ) {
        case 0x01: // Volume
            value = !(sidlr & 1) ? mbSidFilter[1].filterVolume : mbSidFilter[0].filterVolume;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0x02: // OSC Phase
            value = !(sidlr & 1) ? mbSidVoice[3].voiceOscPhase : mbSidVoice[0].voiceOscPhase;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x03: // OSC Detune
            value = !(sidlr & 1) ? mbSidVoice[3].voiceDetuneDelta : mbSidVoice[0].voiceDetuneDelta;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x04: // Filter CutOff
            value = !(sidlr & 1) ? mbSidFilter[1].filterCutoff : mbSidFilter[1].filterCutoff;
            if( scaleTo16bit ) value <<= 4;
            break;

        case 0x05: // Filter Resonance
            value = !(sidlr & 1) ? mbSidFilter[1].filterResonance : mbSidFilter[1].filterResonance;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x06: // Filter Channels
            value = !(sidlr & 1) ? mbSidFilter[1].filterChannels : mbSidFilter[1].filterChannels;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x07: // Filter Mode
            value = !(sidlr & 1) ? mbSidFilter[1].filterMode : mbSidFilter[1].filterMode;
            if( scaleTo16bit ) value <<= 12;
            break;
        }
    } else if( par <= 0x0f ) { // Knobs
        value = knobGet(par & 0x07);
        if( scaleTo16bit ) value <<= 8;
    } else if( par <= 0x1f ) {
        if( par <= 0x17 ) {
            // External Parameters (CV): TODO
        } else {
            // External Switches: TODO
        }
    } else if( par <= 0x5f || par >= 0xfc ) { // Voice related parameters
        int voice = 0; // select first voice by default
        if( (par & 3) > 0)
            voice = (par&3) - 1;
        if( !(sidlr & 1) ) // if first voice not selected
            voice += 3;
        MbSidVoice *v = &mbSidVoice[voice];

        switch( par & 0xfc ) {
        case 0xfc: // Note
            value = v->voiceNote;
            if( scaleTo16bit ) value <<= 9;
            break;

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
            value = mbSidArp[voice].arpSpeed;
            if( scaleTo16bit ) value <<= 10;
        } break;

        case 0x4c: { // Arp Gatelength
            value = mbSidArp[voice].arpGatelength;
            if( scaleTo16bit ) value <<= 11;
        } break;

        case 0x50: // Pitchbender
            value = v->midiVoicePtr->midivoicePitchbender;
            if( scaleTo16bit ) value <<= 8;
            break;

        }
    } else if( par <= 0x6f ) { // Modulation matrix
        sid_se_mod_patch_t *mp;
        mp = (sid_se_mod_patch_t *)&mbSidPatchPtr->body.L.mod[par & 7][0];
        
        if( par <= 0x67 ) {
            value = mp->depth;
            if( scaleTo16bit ) value <<= 8;
        } else {
            value = mp->op >> 6;
            if( scaleTo16bit ) value <<= 14;
        }
    } else if( par <= 0xa7 ) { // LFO
        MbSidLfo *l = &mbSidLfo[par & 7];

        switch( par & 0xf8 ) {
        case 0x80: // LFO Waveform
            value = l->lfoMode.WAVEFORM;
            if( scaleTo16bit ) value <<= 12;
            break;

        case 0x88: // LFO Depth
            value = l->lfoDepth;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x90: // LFO Rate
            value = l->lfoRate;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0x98: // LFO Delay
            value = l->lfoDelay;
            if( scaleTo16bit ) value <<= 8;
            break;

        case 0xa0: // LFO Phase
            value = l->lfoDelay;
            if( scaleTo16bit ) value <<= 8;
            break;
        }
    } else if( par <= 0xdf ) { // ENV
        MbSidEnvLead *e = &mbSidEnvLead[par & 15];

        switch( par & 0xf0 ) {
        case 0x0: value = e->envMode.ALL; break;
        case 0x1: value = e->envDepth; break;
        case 0x2: value = e->envDelay; break;
        case 0x3: value = e->envAttack; break;
        case 0x4: value = e->envAttackLevel; break;
        case 0x5: value = e->envAttack2; break;
        case 0x6: value = e->envDecay; break;
        case 0x7: value = e->envDecayLevel; break;
        case 0x8: value = e->envDecay2; break;
        case 0x9: value = e->envSustain; break;
        case 0xa: value = e->envRelease; break;
        case 0xb: value = e->envReleaseLevel; break;
        case 0xc: value = e->envRelease2; break;
        case 0xd: value = e->envAttackCurve; break;
        case 0xe: value = e->envDecayCurve; break;
        case 0xf: value = e->envReleaseCurve; break;
        }
        if( scaleTo16bit ) value <<= 8;
    } else if( par <= 0xf3 ) { // WT
        MbSidWt *w = &mbSidWt[par & 3];

        switch( par & 0xfc ) {
        case 0xe0: // WT Speed
            value = w->wtSpeed;
            if( scaleTo16bit ) value <<= 10;
            break;

        case 0xe4: // WT Begin
            value = w->wtBegin;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0xe8: // WT End
            value = w->wtEnd;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0xec: // WT Loop
            value = w->wtLoop;
            if( scaleTo16bit ) value <<= 9;
            break;

        case 0xf0: // WT Position
            value = w->wtPos;
            if( scaleTo16bit ) value <<= 9;
            break;
        }
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
void MbSidSeLead::parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins)
{
    return;
}

/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from WT handler
// sid selects the SID
// par selects the parameter (0..255)
// wt_value contains WT table value (0..127: relative change, 128..255: absolute change)
// sidlr selects left/right SID channel
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
/////////////////////////////////////////////////////////////////////////////
void MbSidSeLead::parSetWT(u8 par, u8 wtValue, u8 sidlr, u8 ins)
{
    // branch depending on relative (bit 7 cleared) or absolute value (bit 7 set)
    int parValue;
    if( (wtValue & (1 << 7)) == 0 ) { // relative
        // convert signed 7bit to signed 16bit
        int diff = ((int)wtValue << 9) - 0x8000;
        // do nothing if value is 0 (for compatibility with old presets, e.g. "A077: Analog Dream 1")
        if( diff == 0 )
            return;

        // get current value
        bool scaleTo16bit = true;
        parValue = parGet(par, sidlr, ins, scaleTo16bit);
        // add signed wt value to parameter value and saturate
        parValue += diff;
        if( parValue < 0 ) parValue = 0; else if( parValue > 0xffff ) parValue = 0xffff;
    } else { // absolute
        // 7bit -> 16bit value
        parValue = (wtValue & 0x7f) << 9;
    }

    // perform write operation
    bool scaleFrom16bit = true;
    parSet(par, parValue, sidlr, ins, scaleFrom16bit);
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to set a dedicated SysEx parameter
// (forwarded from MbSidEnvironment and MbSid)
// returns false on invalid access
//
// This function is also used by init() to map the patch to sound engine 
// objects so that we avoid redundancies
/////////////////////////////////////////////////////////////////////////////
bool MbSidSeLead::sysexSetParameter(u16 addr, u8 data)
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
        if( addr == 0x50 ) {
            sid_se_v_flags_t globalVoiceFlags;
            globalVoiceFlags.ALL = data;
            for(MbSidVoice *v = mbSidVoice.first(); v != NULL ; v=mbSidVoice.next(v)) {
                v->voiceLegato = globalVoiceFlags.LEGATO;
                v->voiceSusKey = globalVoiceFlags.SUSKEY;
                v->voicePoly = globalVoiceFlags.POLY;
                v->voiceWavetableOnly = globalVoiceFlags.WTO;
            }
        } else if( addr == 0x51 ) {
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
        } else if( addr == 0x54 ) {
            for(MbSidVoice *v = mbSidVoice.first(); v != NULL ; v=mbSidVoice.next(v))
                v->voiceOscPhase = data;
        }
        return true;
    } else if( addr <= 0x05f ) { // filters
        u8 filter = (addr < 0x05a) ? 1 : 0;
        MbSidFilter *f = &mbSidFilter[filter];

        switch( (addr - 0x54) % 6 ) {
        case 0x0: f->filterChannels = data & 0x0f; f->filterMode = data >> 4; break;
        case 0x1: f->filterCutoff = (f->filterCutoff & 0xff00) | data; break;
        case 0x2: f->filterCutoff = (f->filterCutoff & 0x00ff) | ((data & 0x0f) << 8); break;
        case 0x3: f->filterResonance = data; break;
        case 0x4: f->filterKeytrack = data; break;
        }
        return true;
    } else if( addr <= 0x0bf ) { // voices
        u8 voice = (addr - 0x060) / 16;
        MbSidVoice *v = &mbSidVoice[voice];
        switch( addr & 0x0f ) {
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
        return true;
    } else if( addr <= 0x0dd ) { // LFOs
        u8 lfo = (addr - 0x0c0) / 5;
        MbSidLfo *l = &mbSidLfo[lfo];
        switch( (addr-0xc0) % 5 ) {
        case 0: l->lfoMode.ALL = data; break;
        case 1: l->lfoDepth = data; break;
        case 2: l->lfoRate = data; break;
        case 3: l->lfoDelay = data; break;
        case 4: l->lfoPhase = data; break;
        }
        return true;
    } else if( addr <= 0x0df ) { // reserved
        return true; // ignore
    } else if( addr <= 0x0ff ) { // ENVs
        u8 env = (addr >= 0xf0) ? 1 : 0;
        MbSidEnvLead *e = &mbSidEnvLead[env];
        switch( addr & 0x0f ) {
        case 0x0: e->envMode.ALL = data; break;
        case 0x1: e->envDepth = data; break;
        case 0x2: e->envDelay = data; break;
        case 0x3: e->envAttack = data; break;
        case 0x4: e->envAttackLevel = data; break;
        case 0x5: e->envAttack2 = data; break;
        case 0x6: e->envDecay = data; break;
        case 0x7: e->envDecayLevel = data; break;
        case 0x8: e->envDecay2 = data; break;
        case 0x9: e->envSustain = data; break;
        case 0xa: e->envRelease = data; break;
        case 0xb: e->envReleaseLevel = data; break;
        case 0xc: e->envRelease2 = data; break;
        case 0xd: e->envAttackCurve = data; break;
        case 0xe: e->envDecayCurve = data; break;
        case 0xf: e->envReleaseCurve = data; break;
        }
        return true;
    } else if( addr <= 0x13f ) { // Modulation Matrix
        // u8 mod = (addr - 0x100) / 8;
        // directly read from patch
        return true;
    } else if( addr <= 0x16b ) { // Trigger Matrix
        // u8 trg = (addr - 0x140) / 3;
        // directly read from patch
        return true;
    } else if( addr <= 0x17f ) { // WT Sequencers
        u8 wt = (addr - 0x16c) / 5;
        MbSidWt *w = &mbSidWt[wt];

        switch( (addr-0x16c) % 5 ) {
        case 0: w->wtSpeed = data & 0x3f; break; // left/right flag is read directly from patch
        case 1: break; // assign is directly read from patch
        case 2: w->wtBegin = data & 0x7f; break;
        case 3: w->wtEnd = data & 0x7f; break;
        case 4: w->wtLoop = data & 0x7f; w->wtOneshotMode = (data & 0x80) ? true : false; break;
        }
        return true;
    } else if( addr <= 0x1ff ) { // WT memory
        return true; // no update required
    }

    // unsupported sysex address
    return false;
}
