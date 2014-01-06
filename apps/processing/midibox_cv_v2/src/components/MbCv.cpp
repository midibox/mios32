/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCv.h"
#include <app.h>
#include <string.h>
#include <mbcv_map.h>

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCv::MbCv()
{
    u8 cv = 0;
    init(cv, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCv::~MbCv()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the sound engines
/////////////////////////////////////////////////////////////////////////////
void MbCv::init(u8 _cvNum, MbCvClock *_mbCvClockPtr)
{
    cvNum = _cvNum;

    mbCvClockPtr = _mbCvClockPtr;
    mbCvVoice.init(_cvNum, &mbCvMidiVoice);
    mbCvMidiVoice.init();
    mbCvMod.init(_cvNum);

    updatePatch(true);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbCv::tick(const u8 &updateSpeedFactor)
{
    // clock
    if( mbCvClockPtr->eventStart ) {
        mbCvSeqBassline.seqRestartReq = true;
    }

    if( mbCvClockPtr->eventStop ) {
        mbCvSeqBassline.seqStopReq = true;
    }

    // clock arp and sequencers
    if( mbCvClockPtr->eventClock ) {
        mbCvArp.clockReq = true;
        mbCvSeqBassline.seqClockReq = true;
    }

    // LFOs
    {
        MbCvLfo *l = mbCvLfo.first();
        for(int lfo=0; lfo < mbCvLfo.size; ++lfo, ++l) {
            if( mbCvClockPtr->eventClock )
                l->syncClockReq = 1;

            l->lfoAmplitudeModulation =  mbCvMod.takeDstValue(MBCV_MOD_DST_LFO1_A + lfo);
            l->lfoRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_LFO1_R + lfo);

            if( l->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_L1P+lfo];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_LFO1 + lfo] = l->lfoOut;
        }
    }

    // ENVs
    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e) {
            if( mbCvClockPtr->eventClock )
                e->syncClockReq = 1;

            e->envAmplitudeModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV1_A);
            e->envRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV1_R);

            if( e->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_E1S];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] = e->envOut;
        }
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e) {
            if( mbCvClockPtr->eventClock )
                e->syncClockReq = 1;

            e->envAmplitudeModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV2_A);
            e->envRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV2_R);

            if( e->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_E2S];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] = e->envOut;
        }
    }

    // Modulation Matrix
    {
        // since this isn't done anywhere else:
        // convert linear frequency of voice1 to 15bit signed value (only positive direction)
        mbCvMod.modSrc[MBCV_MOD_SRC_KEY] = mbCvVoice.voiceLinearFrq >> 1;

        // do ModMatrix calculations
        mbCvMod.tick();

        // additional direct modulation paths
        { // Pitch
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_PITCH];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthPitch) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_PITCH] = mod;
        }

        { // LFO1 Amp
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO1_A];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthLfoAmplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo1Amplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo1Amplitude) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO1_A] = mod;
        }

        { // LFO2 Amp
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO2_A];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthLfoAmplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo2Amplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo2Amplitude) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO2_A] = mod;
        }

        { // LFO1 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO1_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthLfoRate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo1Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo1Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO1_R] = mod;
        }

        { // LFO2 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO2_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthLfoRate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo2Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo2Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO2_R] = mod;
        }

        { // ENV1 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_ENV1_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthEnv1Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthEnv1Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_ENV1_R] = mod;
        }

        { // ENV2 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_ENV2_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthEnv2Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthEnv2Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_ENV2_R] = mod;
        }
    }

    {
        // Voice handling
        MbCvVoice *v = &mbCvVoice; // allows to use multiple voices later

        v->voicePitchModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_PITCH);

        if( mbCvArp.arpEnabled ) {
            mbCvArp.tick(v, this);
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ENVMOD] = mbCvSeqBassline.seqEnvMod << 7; // just pass current value
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ACCENT] = mbCvSeqBassline.seqAccent << 7; // just pass current value (not effective value)
        } else {
            mbCvSeqBassline.tick(v, this);
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ENVMOD] = mbCvSeqBassline.seqEnvMod << 7;
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ACCENT] = mbCvSeqBassline.seqAccentEffective << 7; // effective value is only != 0 when accent flag is set
        }

        if( v->gate(updateSpeedFactor) )
            v->pitch(updateSpeedFactor);
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    if( !mbCvMidiVoice.isAssignedPort(port) )
        return;

    switch( midi_package.event ) {
    case NoteOff:
        midiReceiveNote(midi_package.chn, midi_package.note, 0x00);
        break;

    case NoteOn:
        midiReceiveNote(midi_package.chn, midi_package.note, midi_package.velocity);
        break;

    case PolyPressure:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt2);
        break;

    case CC:
        midiReceiveCC(midi_package.chn, midi_package.cc_number, midi_package.value);
        break;

    case PitchBend: {
        u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
        midiReceivePitchBend(midi_package.chn, pitchbend_value_14bit);
    } break;

    case Aftertouch:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt1);
        break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    // check if MIDI channel and splitzone matches
    if( !mbCvMidiVoice.isAssigned(chn, note) )
        return; // note filtered

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    // set note on/off
    if( velocity ) {
        mbCvMod.modSrc[MBCV_MOD_SRC_VEL] = velocity << 8;
        noteOn(note, velocity, false);
    } else
        noteOff(note, false);

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    // take over CC (valid CCs will be checked by MIDI voice)
    if( mbCvMidiVoice.isAssigned(chn) ) {
        if( ccNumber == 1 )
            mbCvMod.modSrc[MBCV_MOD_SRC_MDW] = value << 8;

        mbCvMidiVoice.setCC(ccNumber, value);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    if( mbCvMidiVoice.isAssigned(chn) ) {
        s16 pitchbendValueSigned = (s16)pitchbendValue14bit - 8192;
        mbCvMidiVoice.midivoicePitchbender = pitchbendValueSigned;

        mbCvMod.modSrc[MBCV_MOD_SRC_PBN] = 2*pitchbendValueSigned;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbCvMidiVoice.isAssigned(chn) ) {
        mbCvMidiVoice.midivoiceAftertouch = value << 8;
        mbCvMod.modSrc[MBCV_MOD_SRC_ATH] = value;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenver the patch has been changed
/////////////////////////////////////////////////////////////////////////////
void MbCv::updatePatch(bool forceEngineInit)
{
    // disable interrupts to ensure atomic change
    MIOS32_IRQ_Disable();

    mbCvMidiVoice.init();
    mbCvVoice.init();
    mbCvArp.init();
    mbCvSeqBassline.init();
    
    for(int lfo=0; lfo<mbCvLfo.size; ++lfo)
        mbCvLfo[lfo].init();

    for(int env=0; env<mbCvEnv1.size; ++env)
        mbCvEnv1[env].init();

    // enable interrupts again
    MIOS32_IRQ_Enable();
}



/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteOn(u8 note, u8 velocity, bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    // branch depending on normal/arp mode
    if( mbCvArp.arpEnabled ) {
        mbCvArp.noteOn(v, note, velocity);
    } else {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // switch off gate if not in legato or sequencer mode
        if( !v->voiceLegato && !v->voiceSequencerOnly )
            v->gateOff(note);

        // call Note On Handler
        if( v->noteOn(note, velocity) )
            triggerNoteOn(v);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Disable a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteOff(u8 note, bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
    // if not in arp mode: sustain only relevant if only one active note in stack

    // branch depending on normal/arp mode
    if( mbCvArp.arpEnabled ) {
        mbCvArp.noteOff(v, note);
    } else {
        if( bypassNotestack ) {
            v->noteOff(note, 0);
            triggerNoteOff(v);
        } else {
            u8 lastFirstNote = mv->midivoiceNotestack.note_items[0].note;
            // pop note from stack
            if( mv->notestackPop(note) > 0 ) {
                // call Note Off Handler
                if( v->noteOff(note, lastFirstNote) > 0 ) { // retrigger requested?
                    if( v->noteOn(mv->midivoiceNotestack.note_items[0].note, mv->midivoiceNotestack.note_items[0].tag) ) // new note
                        triggerNoteOn(v);
                } else {
                    triggerNoteOff(v);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteAllOff(bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;

    v->voiceActive = 0;
    v->voiceGateClrReq = 1;
    v->voiceGateSetReq = 0;
    triggerNoteOff(v);

    if( !bypassNotestack ) {
        v->midiVoicePtr->notestackReset();
    }
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbCv::triggerNoteOn(MbCvVoice *v)
{
    mbCvVoice.voiceNoteRestartReq = 1;

    MbCvLfo *l = mbCvLfo.first();
    for(int lfo=0; lfo < mbCvLfo.size; ++lfo, ++l)
        if( l->lfoModeKeySync )
            l->restartReq = 1;

    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e)
            if( e->envModeKeySync )
                e->restartReq = 1;
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e)
            if( e->envModeKeySync )
                e->restartReq = 1;
    }
}


void MbCv::triggerNoteOff(MbCvVoice *v)
{
    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e)
            if( e->envModeKeySync )
                e->releaseReq = 1;
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e)
            if( e->envModeKeySync )
                e->releaseReq = 1;
    }
}



/////////////////////////////////////////////////////////////////////////////
// NRPNs
/////////////////////////////////////////////////////////////////////////////

#define CREATE_GROUP(name, str) \
    static const char name##String[] = str;

#define CREATE_ACCESS_FUNCTIONS(group, name, str, readCode, writeCode) \
    static const char group##name##String[] = str; \
    static void get##group##name(MbCv* cv, u32 arg, u16 *value) { readCode; }    \
    static void set##group##name(MbCv* cv, u32 arg, u16 value) { writeCode; }

typedef struct {
    const char *groupString;
    const char *nameString;
    void (*getFunct)(MbCv *cv, u32 arg, u16 *value);
    void (*setFunct)(MbCv *cv, u32 arg, u16 value);
    u32 arg;
    u16 min;
    u16 max;
    u8 is_bidir;
} MbCvTableEntry_t;

#define NRPN_TABLE_ITEM(group, name, arg, min, max, is_bidir) \
    { group##String, group##name##String, get##group##name, set##group##name, arg, min, max, is_bidir }

#define NRPN_TABLE_ITEM_EMPTY() \
    { NULL, NULL, NULL, NULL, 0, 0, 0, 0 }

#define NRPN_TABLE_ITEM_EMPTY8() \
    NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY(), \
    NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY(), NRPN_TABLE_ITEM_EMPTY()

#define NRPN_TABLE_ITEM_EMPTY16() \
    NRPN_TABLE_ITEM_EMPTY8(), \
    NRPN_TABLE_ITEM_EMPTY8()

CREATE_GROUP(MidiVoice, "Voice");
CREATE_ACCESS_FUNCTIONS(MidiVoice, Port,               "MIDI Port %d",    *value = cv->mbCvMidiVoice.getPortEnabled(arg),          cv->mbCvMidiVoice.setPortEnabled(arg, value));
CREATE_ACCESS_FUNCTIONS(MidiVoice, Channel,            "MIDI Channel",    *value = cv->mbCvMidiVoice.midivoiceChannel,             cv->mbCvMidiVoice.midivoiceChannel = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, EventMode,          "Event Mode",      *value = (u16)cv->mbCvVoice.voiceEventMode,              cv->mbCvVoice.voiceEventMode = (mbcv_midi_event_mode_t)value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, SplitLower,         "Split Lower",     *value = cv->mbCvMidiVoice.midivoiceSplitLower,          cv->mbCvMidiVoice.midivoiceSplitLower = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, SplitUpper,         "Split Upper",     *value = cv->mbCvMidiVoice.midivoiceSplitUpper,          cv->mbCvMidiVoice.midivoiceSplitUpper = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, CCNumber,           "CC Number",       *value = cv->mbCvMidiVoice.midivoiceCCNumber,            cv->mbCvMidiVoice.midivoiceCCNumber = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, AoutCurve,          "AOUT Curve",      *value = cv->mbCvVoice.getAoutCurve(),                   cv->mbCvVoice.setAoutCurve(value));
CREATE_ACCESS_FUNCTIONS(MidiVoice, AoutSlewRate,       "AOUT Slew Rate",  *value = cv->mbCvVoice.getAoutSlewRate(),                cv->mbCvVoice.setAoutSlewRate(value));
CREATE_ACCESS_FUNCTIONS(MidiVoice, GateInverted,       "Inv. Gate",       *value = cv->mbCvVoice.voiceGateInverted,                cv->mbCvVoice.voiceGateInverted = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, Legato,             "Legato",          *value = cv->mbCvVoice.voiceLegato,                      cv->mbCvVoice.voiceLegato = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, Poly,               "Poly",            *value = cv->mbCvVoice.voicePoly,                        cv->mbCvVoice.voicePoly = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, SusKey,             "SusKey",          *value = cv->mbCvVoice.voiceSusKey,                      cv->mbCvVoice.voiceSusKey = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, PortamentoMode,     "PortamentoMode",  *value = cv->mbCvVoice.getPortamentoMode(),              cv->mbCvVoice.setPortamentoMode(value));
CREATE_ACCESS_FUNCTIONS(MidiVoice, PitchRange,         "Pitch Range",     *value = cv->mbCvVoice.voicePitchrange,                  cv->mbCvVoice.voicePitchrange = value);
CREATE_ACCESS_FUNCTIONS(MidiVoice, Ketrack,            "Keytrack",        *value = (int)cv->mbCvVoice.voiceKeytrack + 0x80,        cv->mbCvVoice.voiceKeytrack = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(MidiVoice, TransposeOctave,    "Transp. Octave",  *value = (int)cv->mbCvVoice.voiceTransposeOctave + 8,    cv->mbCvVoice.voiceTransposeOctave = (int)value - 8);
CREATE_ACCESS_FUNCTIONS(MidiVoice, TransposeSemitone,  "Transp. Semi.",   *value = (int)cv->mbCvVoice.voiceTransposeSemitone + 8,  cv->mbCvVoice.voiceTransposeSemitone = (int)value - 8);
CREATE_ACCESS_FUNCTIONS(MidiVoice, Finetune,           "Finetune",        *value = (int)cv->mbCvVoice.voiceFinetune + 0x80,        cv->mbCvVoice.voiceFinetune = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(MidiVoice, PortamentoRate,     "PortamentoRate",  *value = cv->mbCvVoice.voicePortamentoRate,              cv->mbCvVoice.voicePortamentoRate = value);

CREATE_GROUP(Arp, "Arp");
CREATE_ACCESS_FUNCTIONS(Arp, Enabled,                  "Enable",          *value = cv->mbCvArp.arpEnabled,                         cv->mbCvArp.arpEnabled = value);
CREATE_ACCESS_FUNCTIONS(Arp, Dir,                      "Direction",       *value = cv->mbCvArp.arpDirGet(),                        cv->mbCvArp.arpDirSet(value));
CREATE_ACCESS_FUNCTIONS(Arp, HoldMode,                 "Hold",            *value = cv->mbCvArp.arpHoldMode,                        cv->mbCvArp.arpHoldMode = value);
CREATE_ACCESS_FUNCTIONS(Arp, SortedNotes,              "Sort",            *value = cv->mbCvArp.arpSortedNotes,                     cv->mbCvArp.arpSortedNotes = value);
CREATE_ACCESS_FUNCTIONS(Arp, SyncMode,                 "Sync",            *value = cv->mbCvArp.arpSyncMode,                        cv->mbCvArp.arpSyncMode = value);
CREATE_ACCESS_FUNCTIONS(Arp, OneshotMode,              "Oneshot",         *value = cv->mbCvArp.arpOneshotMode,                     cv->mbCvArp.arpOneshotMode = value);
CREATE_ACCESS_FUNCTIONS(Arp, ConstantCycle,            "Constant Cycle",  *value = cv->mbCvArp.arpConstantCycle,                   cv->mbCvArp.arpConstantCycle = value);
CREATE_ACCESS_FUNCTIONS(Arp, EasyChordMode,            "Easy Chord",      *value = cv->mbCvArp.arpEasyChordMode,                   cv->mbCvArp.arpEasyChordMode = value);
CREATE_ACCESS_FUNCTIONS(Arp, Speed,                    "Speed",           *value = cv->mbCvArp.arpSpeed,                           cv->mbCvArp.arpSpeed = value);
CREATE_ACCESS_FUNCTIONS(Arp, Gatelength,               "Gatelength",      *value = cv->mbCvArp.arpGatelength,                      cv->mbCvArp.arpGatelength = value);

CREATE_GROUP(Seq, "Seq");
CREATE_ACCESS_FUNCTIONS(Seq, Enabled,          "Enable",          *value = cv->mbCvSeqBassline.seqEnabled,                 cv->mbCvSeqBassline.seqEnabled = value);
CREATE_ACCESS_FUNCTIONS(Seq, PatternNumber,    "Pattern #",       *value = cv->mbCvSeqBassline.seqPatternNumber,           cv->mbCvSeqBassline.seqPatternNumber = value);
CREATE_ACCESS_FUNCTIONS(Seq, PatternLength,    "Length",          *value = cv->mbCvSeqBassline.seqPatternLength,           cv->mbCvSeqBassline.seqPatternLength = value);
CREATE_ACCESS_FUNCTIONS(Seq, Resolution,       "Resolution",      *value = cv->mbCvSeqBassline.seqResolution,              cv->mbCvSeqBassline.seqResolution = value);
CREATE_ACCESS_FUNCTIONS(Seq, GateLength,       "Gatelength",      *value = cv->mbCvSeqBassline.seqGateLength,              cv->mbCvSeqBassline.seqGateLength = value);
CREATE_ACCESS_FUNCTIONS(Seq, EnvMod,           "EnvMod",          *value = cv->mbCvSeqBassline.seqEnvMod,                  cv->mbCvSeqBassline.seqEnvMod = value);
CREATE_ACCESS_FUNCTIONS(Seq, Accent,           "Accent",          *value = cv->mbCvSeqBassline.seqAccent,                  cv->mbCvSeqBassline.seqAccent = value);
CREATE_ACCESS_FUNCTIONS(Seq, Key,              "Key #%2d",        *value = cv->mbCvSeqBassline.seqBasslineKey[cv->mbCvSeqBassline.seqPatternNumber][arg],      cv->mbCvSeqBassline.seqBasslineKey[cv->mbCvSeqBassline.seqPatternNumber][arg] = value);
CREATE_ACCESS_FUNCTIONS(Seq, Args,             "Args #%2d",       *value = cv->mbCvSeqBassline.seqBasslineArgs[cv->mbCvSeqBassline.seqPatternNumber][arg].ALL, cv->mbCvSeqBassline.seqBasslineArgs[cv->mbCvSeqBassline.seqPatternNumber][arg].ALL = value);
 
CREATE_GROUP(Lfo, "LFO%d");
CREATE_ACCESS_FUNCTIONS(Lfo, Amplitude,                "Amplitude",       *value = (int)cv->mbCvLfo[arg].lfoAmplitude + 0x80,         cv->mbCvLfo[arg].lfoAmplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Lfo, Rate,                     "Rate",            *value = (int)cv->mbCvLfo[arg].lfoRate,                     cv->mbCvLfo[arg].lfoRate = value);
CREATE_ACCESS_FUNCTIONS(Lfo, Delay,                    "Delay",           *value = (int)cv->mbCvLfo[arg].lfoDelay,                    cv->mbCvLfo[arg].lfoDelay = value);
CREATE_ACCESS_FUNCTIONS(Lfo, Phase,                    "Phase",           *value = (int)cv->mbCvLfo[arg].lfoPhase,                    cv->mbCvLfo[arg].lfoPhase = value);
CREATE_ACCESS_FUNCTIONS(Lfo, Waveform,                 "Waveform",        *value = (int)cv->mbCvLfo[arg].lfoWaveform,                 cv->mbCvLfo[arg].lfoWaveform = value);
CREATE_ACCESS_FUNCTIONS(Lfo, ModeClkSync,              "ClkSync",         *value = (int)cv->mbCvLfo[arg].lfoModeClkSync,              cv->mbCvLfo[arg].lfoModeClkSync = value);
CREATE_ACCESS_FUNCTIONS(Lfo, ModeKeySync,              "KeySync",         *value = (int)cv->mbCvLfo[arg].lfoModeKeySync,              cv->mbCvLfo[arg].lfoModeKeySync = value);
CREATE_ACCESS_FUNCTIONS(Lfo, ModeOneshot,              "Oneshot",         *value = (int)cv->mbCvLfo[arg].lfoModeOneshot,              cv->mbCvLfo[arg].lfoModeOneshot = value);
CREATE_ACCESS_FUNCTIONS(Lfo, ModeFast,                 "Fast",            *value = (int)cv->mbCvLfo[arg].lfoModeFast,                 cv->mbCvLfo[arg].lfoModeFast = value);
CREATE_ACCESS_FUNCTIONS(Lfo, DepthPitch,               "Depth CV",        *value = (int)cv->mbCvLfo[arg].lfoDepthPitch + 0x80,        cv->mbCvLfo[arg].lfoDepthPitch = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Lfo, DepthLfoAmplitude,        "Depth LFO Amp.",  *value = (int)cv->mbCvLfo[arg].lfoDepthLfoAmplitude + 0x80, cv->mbCvLfo[arg].lfoDepthLfoAmplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Lfo, DepthLfoRate,             "Depth LFO Rate",  *value = (int)cv->mbCvLfo[arg].lfoDepthLfoRate + 0x80,      cv->mbCvLfo[arg].lfoDepthLfoRate = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Lfo, DepthEnv1Rate,            "Depth ENV1 Rate", *value = (int)cv->mbCvLfo[arg].lfoDepthEnv1Rate + 0x80,     cv->mbCvLfo[arg].lfoDepthEnv1Rate = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Lfo, DepthEnv2Rate,            "Depth ENV2 Rate", *value = (int)cv->mbCvLfo[arg].lfoDepthEnv2Rate + 0x80,     cv->mbCvLfo[arg].lfoDepthEnv2Rate = (int)value - 0x80);

CREATE_GROUP(Env1, "ENV1");
CREATE_ACCESS_FUNCTIONS(Env1, Amplitude,               "Amplitude",       *value = (int)cv->mbCvEnv1[arg].envAmplitude + 0x80,          cv->mbCvEnv1[arg].envAmplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env1, Curve,                   "Curve",           *value = (int)cv->mbCvEnv1[arg].envCurve,                     cv->mbCvEnv1[arg].envCurve = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, Delay,                   "Delay",           *value = (int)cv->mbCvEnv1[arg].envDelay,                     cv->mbCvEnv1[arg].envDelay = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, Attack,                  "Attack",          *value = (int)cv->mbCvEnv1[arg].envAttack,                    cv->mbCvEnv1[arg].envAttack = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, Decay,                   "Decay",           *value = (int)cv->mbCvEnv1[arg].envDecay,                     cv->mbCvEnv1[arg].envDecay = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, Sustain,                 "Sustain",         *value = (int)cv->mbCvEnv1[arg].envSustain,                   cv->mbCvEnv1[arg].envSustain = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, Release,                 "Release",         *value = (int)cv->mbCvEnv1[arg].envRelease,                   cv->mbCvEnv1[arg].envRelease = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, ModeClkSync,             "ClkSync",         *value = (int)cv->mbCvEnv1[arg].envModeClkSync,               cv->mbCvEnv1[arg].envModeClkSync = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, ModeKeySync,             "KeySync",         *value = (int)cv->mbCvEnv1[arg].envModeKeySync,               cv->mbCvEnv1[arg].envModeKeySync = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, ModeOneshot,             "Oneshot",         *value = (int)cv->mbCvEnv1[arg].envModeOneshot,               cv->mbCvEnv1[arg].envModeOneshot = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, ModeFast,                "Fast",            *value = (int)cv->mbCvEnv1[arg].envModeFast,                  cv->mbCvEnv1[arg].envModeFast = (int)value);
CREATE_ACCESS_FUNCTIONS(Env1, DepthPitch,              "Depth CV",        *value = (int)cv->mbCvEnv1[arg].envDepthPitch + 0x80,         cv->mbCvEnv1[arg].envDepthPitch = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env1, DepthLfo1Amplitude,      "Depth LFO1 Amp.", *value = (int)cv->mbCvEnv1[arg].envDepthLfo1Amplitude + 0x80, cv->mbCvEnv1[arg].envDepthLfo1Amplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env1, DepthLfo1Rate,           "Depth LFO1 Rate", *value = (int)cv->mbCvEnv1[arg].envDepthLfo1Rate + 0x80,      cv->mbCvEnv1[arg].envDepthLfo1Rate = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env1, DepthLfo2Amplitude,      "Depth LFO2 Amp.", *value = (int)cv->mbCvEnv1[arg].envDepthLfo2Amplitude + 0x80, cv->mbCvEnv1[arg].envDepthLfo2Amplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env1, DepthLfo2Rate,           "Depth LFO2 Rate", *value = (int)cv->mbCvEnv1[arg].envDepthLfo2Rate + 0x80,      cv->mbCvEnv1[arg].envDepthLfo2Rate = (int)value - 0x80);

CREATE_GROUP(Env2, "ENV2");
CREATE_ACCESS_FUNCTIONS(Env2, Amplitude,               "Amplitude",       *value = (int)cv->mbCvEnv2[arg].envAmplitude + 0x80,          cv->mbCvEnv2[arg].envAmplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, Curve,                   "Curve",           *value = (int)cv->mbCvEnv2[arg].envCurve,                     cv->mbCvEnv2[arg].envCurve = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, Offset,                  "Offset",          *value = (int)cv->mbCvEnv2[arg].envOffset,                    cv->mbCvEnv2[arg].envOffset = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, Rate,                    "Rate",            *value = (int)cv->mbCvEnv2[arg].envRate,                      cv->mbCvEnv2[arg].envRate = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, LoopAttack,              "Loop Attack",     *value = (int)cv->mbCvEnv2[arg].envLoopAttack,                cv->mbCvEnv2[arg].envLoopAttack = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, SustainStep,             "Sustain Step",    *value = (int)cv->mbCvEnv2[arg].envSustainStep,               cv->mbCvEnv2[arg].envSustainStep = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, LoopRelease,             "Loop Release",    *value = (int)cv->mbCvEnv2[arg].envLoopRelease,               cv->mbCvEnv2[arg].envLoopRelease = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, LastStep,                "Last Step",       *value = (int)cv->mbCvEnv2[arg].envLastStep,                  cv->mbCvEnv2[arg].envLastStep = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, ModeClkSync,             "ClkSync",         *value = (int)cv->mbCvEnv2[arg].envModeClkSync,               cv->mbCvEnv2[arg].envModeClkSync = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, ModeKeySync,             "KeySync",         *value = (int)cv->mbCvEnv2[arg].envModeKeySync,               cv->mbCvEnv2[arg].envModeKeySync = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, ModeOneshot,             "Oneshot",         *value = (int)cv->mbCvEnv2[arg].envModeOneshot,               cv->mbCvEnv2[arg].envModeOneshot = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, ModeFast,                "Fast",            *value = (int)cv->mbCvEnv2[arg].envModeFast,                  cv->mbCvEnv2[arg].envModeFast = (int)value);
CREATE_ACCESS_FUNCTIONS(Env2, DepthPitch,              "Depth CV",        *value = (int)cv->mbCvEnv2[arg].envDepthPitch + 0x80,         cv->mbCvEnv2[arg].envDepthPitch = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, DepthLfo1Amplitude,      "Depth LFO1 Amp.", *value = (int)cv->mbCvEnv2[arg].envDepthLfo1Amplitude + 0x80, cv->mbCvEnv2[arg].envDepthLfo1Amplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, DepthLfo1Rate,           "Depth LFO1 Rate", *value = (int)cv->mbCvEnv2[arg].envDepthLfo1Rate + 0x80,      cv->mbCvEnv2[arg].envDepthLfo1Rate = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, DepthLfo2Amplitude,      "Depth LFO2 Amp.", *value = (int)cv->mbCvEnv2[arg].envDepthLfo2Amplitude + 0x80, cv->mbCvEnv2[arg].envDepthLfo2Amplitude = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, DepthLfo2Rate,           "Depth LFO2 Rate", *value = (int)cv->mbCvEnv2[arg].envDepthLfo2Rate + 0x80,      cv->mbCvEnv2[arg].envDepthLfo2Rate = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Env2, Level,                   "Level Step #%2d", *value = cv->mbCvEnv2[0].envLevel[arg],                       cv->mbCvEnv2[0].envLevel[arg] = value); // TODO: take ENV index into MSBs of arg?

CREATE_GROUP(Mod, "Mod%d");
CREATE_ACCESS_FUNCTIONS(Mod, Depth,                    "Depth",           *value = cv->mbCvMod.modPatch[arg].depth + 0x80,              cv->mbCvMod.modPatch[arg].depth = (int)value - 0x80);
CREATE_ACCESS_FUNCTIONS(Mod, Src1,                     "Source1",         *value = cv->mbCvMod.modPatch[arg].src1,                      cv->mbCvMod.modPatch[arg].src1 = value);
CREATE_ACCESS_FUNCTIONS(Mod, Src1Chn,                  "Source1 CV",      *value = cv->mbCvMod.modPatch[arg].src1_chn,                  cv->mbCvMod.modPatch[arg].src1_chn = value);
CREATE_ACCESS_FUNCTIONS(Mod, Src2,                     "Source2",         *value = cv->mbCvMod.modPatch[arg].src2,                      cv->mbCvMod.modPatch[arg].src2 = value);
CREATE_ACCESS_FUNCTIONS(Mod, Src2Chn,                  "Source2 CV",      *value = cv->mbCvMod.modPatch[arg].src2_chn,                  cv->mbCvMod.modPatch[arg].src2_chn = value);
CREATE_ACCESS_FUNCTIONS(Mod, Op,                       "Operator",        *value = cv->mbCvMod.modPatch[arg].op & 0x3f,                 cv->mbCvMod.modPatch[arg].op &= 0xc0; cv->mbCvMod.modPatch[arg].op |= (value & 0x3f));
CREATE_ACCESS_FUNCTIONS(Mod, Dst1,                     "Destination1",    *value = cv->mbCvMod.modPatch[arg].dst1,                      cv->mbCvMod.modPatch[arg].dst1 = value);
CREATE_ACCESS_FUNCTIONS(Mod, Dst1Inv,                  "Dst1 Inverted",   *value = (cv->mbCvMod.modPatch[arg].op & (1 << 6)) ? 1 : 0,   cv->mbCvMod.modPatch[arg].op &= ~(1 << 6); cv->mbCvMod.modPatch[arg].op |= ((value&1) << 6));
CREATE_ACCESS_FUNCTIONS(Mod, Dst2,                     "Destination2",    *value = cv->mbCvMod.modPatch[arg].dst2,                      cv->mbCvMod.modPatch[arg].dst2 = value);
CREATE_ACCESS_FUNCTIONS(Mod, Dst2Inv,                  "Dst2 Inverted",   *value = (cv->mbCvMod.modPatch[arg].op & (1 << 7)) ? 1 : 0,   cv->mbCvMod.modPatch[arg].op &= ~(1 << 7); cv->mbCvMod.modPatch[arg].op |= ((value&1) << 7));


#define MBCV_NRPN_TABLE_SIZE 0x380
static const MbCvTableEntry_t mbCvNrpnTable[MBCV_NRPN_TABLE_SIZE] = {
    // 0x000
    NRPN_TABLE_ITEM(  MidiVoice, Port,               0, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               1, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               2, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               3, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               4, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               5, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               6, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               7, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               8, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,               9, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              10, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              11, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              12, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              13, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              14, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Port,              15, 0, 1, 0),

    // 0x010
    NRPN_TABLE_ITEM(  MidiVoice, Channel,            0, 0, 16, 0),
    NRPN_TABLE_ITEM(  MidiVoice, EventMode,          0, 0, MBCV_MIDI_EVENT_MODE_NUM-1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, SplitLower,         0, 0, 127, 0),
    NRPN_TABLE_ITEM(  MidiVoice, SplitUpper,         0, 0, 127, 0),
    NRPN_TABLE_ITEM(  MidiVoice, CCNumber,           0, 0, 127, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x018
    NRPN_TABLE_ITEM(  MidiVoice, AoutCurve,          0, 0, MBCV_MAP_NUM_CURVES-1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, AoutSlewRate,       0, 0, 255, 0),
    NRPN_TABLE_ITEM(  MidiVoice, GateInverted,       0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x20
    NRPN_TABLE_ITEM(  MidiVoice, Legato,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Poly,               0, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, SusKey,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  MidiVoice, PortamentoMode,     0, 0, 2, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x030
    NRPN_TABLE_ITEM(  MidiVoice, PitchRange,         0, 0, 24, 0),
    NRPN_TABLE_ITEM(  MidiVoice, Ketrack,            0, 0, 255, 1),
    NRPN_TABLE_ITEM(  MidiVoice, TransposeOctave,    0, 0, 15, 1),
    NRPN_TABLE_ITEM(  MidiVoice, TransposeSemitone,  0, 0, 15, 1),
    NRPN_TABLE_ITEM(  MidiVoice, Finetune,           0, 0, 255, 1),
    NRPN_TABLE_ITEM(  MidiVoice, PortamentoRate,     0, 0, 255, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x040
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x050
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x060
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x070
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x080
    NRPN_TABLE_ITEM(  Arp, Enabled,                  0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, Dir,                      0, 0, 6, 0),
    NRPN_TABLE_ITEM(  Arp, HoldMode,                 0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, SortedNotes,              0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, SyncMode,                 0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, OneshotMode,              0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, ConstantCycle,            0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Arp, EasyChordMode,            0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x090
    NRPN_TABLE_ITEM(  Arp, Speed,                    0, 0, 63, 0),
    NRPN_TABLE_ITEM(  Arp, Gatelength,               0, 0, 63, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x0a0
    NRPN_TABLE_ITEM(  Seq, Enabled,          0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Seq, PatternNumber,    0, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, PatternLength,    0, 0, 31, 0),
    NRPN_TABLE_ITEM(  Seq, Resolution,       0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Seq, GateLength,       0, 0, 100, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x0a8
    NRPN_TABLE_ITEM(  Seq, EnvMod,           0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Seq, Accent,           0, 0, 255, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x0b0
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x0c0
    NRPN_TABLE_ITEM(  Seq, Key,              0, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              1, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              2, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              3, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              4, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              5, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              6, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              7, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              8, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,              9, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             10, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             11, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             12, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             13, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             14, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             15, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             16, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             17, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             18, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             19, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             20, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             21, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             22, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             23, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             24, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             25, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             26, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             27, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             28, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             29, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             30, 0, 127, 0),
    NRPN_TABLE_ITEM(  Seq, Key,             31, 0, 127, 0),

    // 0x0e0
    NRPN_TABLE_ITEM(  Seq, Args,             0, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             1, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             2, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             3, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             4, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             5, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             6, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             7, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             8, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,             9, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            10, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            11, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            12, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            13, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            14, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            15, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            16, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            17, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            18, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            19, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            20, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            21, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            22, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            23, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            24, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            25, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            26, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            27, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            28, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            29, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            30, 0, 7, 0),
    NRPN_TABLE_ITEM(  Seq, Args,            31, 0, 7, 0),

    // 0x100
    NRPN_TABLE_ITEM(  Lfo, Amplitude,                0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, Rate,                     0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Delay,                    0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Phase,                    0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Waveform,                 0, 0, 8, 0), // TODO: create enum for waveform
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x110
    NRPN_TABLE_ITEM(  Lfo, ModeClkSync,              0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeKeySync,              0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeOneshot,              0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeFast,                 0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x120
    NRPN_TABLE_ITEM(  Lfo, DepthPitch,               0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthLfoAmplitude,        0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthLfoRate,             0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthEnv1Rate,            0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthEnv2Rate,            0, 0, 255, 1),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x130
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x140
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x150
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x160
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x170
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x180
    NRPN_TABLE_ITEM(  Lfo, Amplitude,                1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, Rate,                     1, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Delay,                    1, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Phase,                    1, 0, 255, 0),
    NRPN_TABLE_ITEM(  Lfo, Waveform,                 1, 0, 8, 0), // TODO: create enum for waveform
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x190
    NRPN_TABLE_ITEM(  Lfo, ModeClkSync,              1, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeKeySync,              1, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeOneshot,              1, 0, 1, 0),
    NRPN_TABLE_ITEM(  Lfo, ModeFast,                 1, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x1a0
    NRPN_TABLE_ITEM(  Lfo, DepthPitch,               1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthLfoAmplitude,        1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthLfoRate,             1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthEnv1Rate,            1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Lfo, DepthEnv2Rate,            1, 0, 255, 1),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x1b0
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x1c0
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x1d0
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x1e0
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x1f0
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x200
    NRPN_TABLE_ITEM(  Env1, Amplitude,               0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env1, Curve,                   0, 0, 3, 0), // TODO: create enum for curve
    NRPN_TABLE_ITEM(  Env1, Delay,                   0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env1, Attack,                  0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env1, Decay,                   0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env1, Sustain,                 0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env1, Release,                 0, 0, 255, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x210
    NRPN_TABLE_ITEM(  Env1, ModeClkSync,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env1, ModeKeySync,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env1, ModeOneshot,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env1, ModeFast,                0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x220
    NRPN_TABLE_ITEM(  Env1, DepthPitch,              0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env1, DepthLfo1Amplitude,      0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env1, DepthLfo1Rate,           0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env1, DepthLfo2Amplitude,      0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env1, DepthLfo2Rate,           0, 0, 255, 1),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x230
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x240
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x250
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x260
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x270
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x280
    NRPN_TABLE_ITEM(  Env2, Amplitude,               0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env2, Curve,                   0, 0, 3, 0), // TODO: create enum for curve
    NRPN_TABLE_ITEM(  Env2, Offset,                  0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Rate,                    0, 0, 255, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x288
    NRPN_TABLE_ITEM(  Env2, LoopAttack,              0, 0, 16, 0),
    NRPN_TABLE_ITEM(  Env2, SustainStep,             0, 0, 16, 0),
    NRPN_TABLE_ITEM(  Env2, LoopRelease,             0, 0, 16, 0),
    NRPN_TABLE_ITEM(  Env2, LastStep,                0, 0, 16, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x290
    NRPN_TABLE_ITEM(  Env2, ModeClkSync,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env2, ModeKeySync,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env2, ModeOneshot,             0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Env2, ModeFast,                0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x2a0
    NRPN_TABLE_ITEM(  Env2, DepthPitch,              0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env2, DepthLfo1Amplitude,      0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env2, DepthLfo1Rate,           0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env2, DepthLfo2Amplitude,      0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Env2, DepthLfo2Rate,           0, 0, 255, 1),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY8(),

    // 0x2b0
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x2c0
    NRPN_TABLE_ITEM(  Env2, Level,                   0, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   1, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   2, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   3, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   4, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   5, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   6, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   7, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   8, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                   9, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  10, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  11, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  12, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  13, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  14, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  15, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  16, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  17, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  18, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  19, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  20, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  21, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  22, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  23, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  24, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  25, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  26, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  27, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  28, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  29, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  30, 0, 255, 0),
    NRPN_TABLE_ITEM(  Env2, Level,                  31, 0, 255, 0),

    // 0x2e0
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x2f0
    NRPN_TABLE_ITEM_EMPTY16(),

    // 0x300
    NRPN_TABLE_ITEM(  Mod, Depth,                    0, 0, 255, 1),
    NRPN_TABLE_ITEM(  Mod, Src1,                     0, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src1Chn,                  0, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2,                     0, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2Chn,                  0, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Op,                       0, 0, 14, 0), // TODO: create enum for operations
    NRPN_TABLE_ITEM(  Mod, Dst1,                     0, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst1Inv,                  0, 0, 1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2,                     0, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2Inv,                  0, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x310
    NRPN_TABLE_ITEM(  Mod, Depth,                    1, 0, 255, 1),
    NRPN_TABLE_ITEM(  Mod, Src1,                     1, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src1Chn,                  1, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2,                     1, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2Chn,                  1, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Op,                       1, 0, 14, 0), // TODO: create enum for operations
    NRPN_TABLE_ITEM(  Mod, Dst1,                     1, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst1Inv,                  1, 0, 1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2,                     1, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2Inv,                  1, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x320
    NRPN_TABLE_ITEM(  Mod, Depth,                    2, 0, 255, 1),
    NRPN_TABLE_ITEM(  Mod, Src1,                     2, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src1Chn,                  2, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2,                     2, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2Chn,                  2, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Op,                       2, 0, 14, 0), // TODO: create enum for operations
    NRPN_TABLE_ITEM(  Mod, Dst1,                     2, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst1Inv,                  2, 0, 1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2,                     2, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2Inv,                  2, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x330
    NRPN_TABLE_ITEM(  Mod, Depth,                    3, 0, 255, 1),
    NRPN_TABLE_ITEM(  Mod, Src1,                     3, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src1Chn,                  3, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2,                     3, 0, MBCV_NUM_MOD_SRC-1, 0),
    NRPN_TABLE_ITEM(  Mod, Src2Chn,                  3, 0, CV_SE_NUM-1, 0),
    NRPN_TABLE_ITEM(  Mod, Op,                       3, 0, 14, 0), // TODO: create enum for operations
    NRPN_TABLE_ITEM(  Mod, Dst1,                     3, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst1Inv,                  3, 0, 1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2,                     3, 0, MBCV_NUM_MOD_DST-1, 0),
    NRPN_TABLE_ITEM(  Mod, Dst2Inv,                  3, 0, 1, 0),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),
    NRPN_TABLE_ITEM_EMPTY(),

    // 0x340
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x350
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x360
    NRPN_TABLE_ITEM_EMPTY16(),
    // 0x370
    NRPN_TABLE_ITEM_EMPTY16(),

};


/////////////////////////////////////////////////////////////////////////////
// will set NRPN depending on first 10 bits
// MSBs already decoded in MbCvEnvironment
// returns false if parameter not mapped
/////////////////////////////////////////////////////////////////////////////
bool MbCv::setNRPN(u16 nrpnNumber, u16 value)
{
    u32 par = nrpnNumber & 0x3ff;
    if( par < MBCV_NRPN_TABLE_SIZE ) {
        MbCvTableEntry_t *t = (MbCvTableEntry_t *)&mbCvNrpnTable[par];
        if( t->setFunct != NULL ) {
            t->setFunct(this, t->arg, value);
            return true;
        }
    }

    return false; // parameter not mapped
}


/////////////////////////////////////////////////////////////////////////////
// returns NRPN value depending on first 10 bits
// MSBs already decoded in MbCvEnvironment
// returns false if parameter not mapped
/////////////////////////////////////////////////////////////////////////////
bool MbCv::getNRPN(u16 nrpnNumber, u16 *value)
{
    u32 par = nrpnNumber & 0x3ff;
    if( par < MBCV_NRPN_TABLE_SIZE ) {
        MbCvTableEntry_t *t = (MbCvTableEntry_t *)&mbCvNrpnTable[par];
        if( t->getFunct != NULL ) {
            t->getFunct(this, t->arg, value);
            return true;
        }
    }

    return false; // parameter not mapped
}

/////////////////////////////////////////////////////////////////////////////
// returns NRPN informations depending on first 10 bits
// MSBs already decoded in MbCvEnvironment
// returns false if parameter not mapped
/////////////////////////////////////////////////////////////////////////////
bool MbCv::getNRPNInfo(u16 nrpnNumber, MbCvNrpnInfoT *info)
{
    u32 par = nrpnNumber & 0x3ff;
    if( par < MBCV_NRPN_TABLE_SIZE ) {
        MbCvTableEntry_t *t = (MbCvTableEntry_t *)&mbCvNrpnTable[par];
        if( t->getFunct != NULL ) {
            t->getFunct(this, t->arg, &info->value);
            info->cv = cvNum;
            info->is_bidir = t->is_bidir;
            info->min = t->min;
            info->max = t->max;

            {
                char nameString[40];
                char nameString1[21];

                sprintf(nameString1, t->groupString, t->arg+1);
                sprintf(nameString, "CV%d %s", cvNum+1, nameString1);

                // 20 chars max; pad with spaces
                int len = strlen(nameString);
                for(int pos=len; pos<20; ++pos)
                    nameString[pos] = ' ';
                nameString[20] = 0;
                memcpy(info->nameString, nameString, 21);
            }

            {
                char valueString[40];
                char nameString1[21];

                sprintf(nameString1, t->nameString, t->arg+1);

                if( info->is_bidir ) {
                    int range = info->max - info->min + 1;
                    sprintf(valueString, "%s:%4d", nameString1, (int)info->value - (range/2));
                } else {
                    if( info->min == 0 && info->max == 1 ) {
                        sprintf(valueString, "%s: %s", nameString1, info->value ? "on " : "off");
                    } else {
                        sprintf(valueString, "%s:%4d", nameString1, info->value);
                    }
                }

                // 20 chars max; pad with spaces
                int len = strlen(valueString);
                for(int pos=len; pos<20; ++pos)
                    valueString[pos] = ' ';

                valueString[20] = 0;
                memcpy(info->valueString, valueString, 21);
            }

            return true;
        }
    }

    return false; // parameter not mapped
}

