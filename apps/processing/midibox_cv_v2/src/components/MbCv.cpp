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
    mbCvClockPtr = _mbCvClockPtr;
    mbCvVoice.init(_cvNum, &mbCvMidiVoice);
    mbCvMidiVoice.init();

    updatePatch(true);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbCv::tick(const u8 &updateSpeedFactor)
{
    // clear modulation destinations
    mbCvVoice.voicePitchModulation = 0;

    // clock arp
    if( mbCvClockPtr->eventClock ) {
        mbCvArp.clockReq = true;
    }

    // LFOs
    MbCvLfo *l = mbCvLfo.first();
    for(int lfo=0; lfo < mbCvLfo.size; ++lfo, ++l) {
        l->tick(updateSpeedFactor);

        mbCvVoice.voicePitchModulation += (l->lfoOut * (s32)l->lfoDepthPitch) / 128;
    }

    // ENVs
    MbCvEnv *e = mbCvEnv.first();
    for(int env=0; env < mbCvEnv.size; ++env, ++e) {
        e->tick(updateSpeedFactor);

        mbCvVoice.voicePitchModulation += (e->envOut * (s32)e->envDepthPitch) / 128;
    }

    // Voice handling
    MbCvVoice *v = &mbCvVoice; // allows to use multiple voices later
    if( mbCvArp.arpEnabled )
        mbCvArp.tick(v);

    if( v->gate(updateSpeedFactor) )
        v->pitch(updateSpeedFactor);

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
    // operation must be atomic!
    MIOS32_IRQ_Disable();

    // check if MIDI channel and splitzone matches
    if( !mbCvMidiVoice.isAssigned(chn, note) )
        return;

    // set note on/off
    if( velocity )
        noteOn(note, velocity, false);
    else
        noteOff(note, false);

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    // take over CC (valid CCs will be checked by MIDI voice)
    if( mbCvMidiVoice.isAssigned(chn) )
        mbCvMidiVoice.setCC(ccNumber, value);
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    if( mbCvMidiVoice.isAssigned(chn) ) {
        s16 pitchbendValueSigned = (s16)pitchbendValue14bit - 8192;
        mbCvMidiVoice.midivoicePitchbender = pitchbendValueSigned;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbCvMidiVoice.isAssigned(chn) )
        mbCvMidiVoice.midivoiceAftertouch = value;
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

    for(int lfo=0; lfo<mbCvLfo.size; ++lfo)
        mbCvLfo[lfo].init();

    for(int env=0; env<mbCvEnv.size; ++env)
        mbCvEnv[env].init();

    // enable interrupts again
    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to take over a received patch
// returns false if initialisation failed
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexSetPatch(cv_patch_t *p)
{
    mbCvPatch.copyToPatch(p);
    updatePatch(false);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to return the current patch
// returns false if patch not available
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexGetPatch(cv_patch_t *p)
{
    // TODO: bank read
    mbCvPatch.copyFromPatch(p);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to set a dedicated SysEx parameter
// returns false on invalid access
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexSetParameter(u16 addr, u8 data)
{
    // exit if address range not valid (just to ensure)
    if( addr >= sizeof(cv_patch_t) )
        return false;

    // change value in patch
    mbCvPatch.body.ALL[addr] = data;

    return false; // TODO
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
        if( l->lfoMode.KEYSYNC )
            l->restartReq = 1;

    MbCvEnv *e = mbCvEnv.first();
    for(int env=0; env < mbCvEnv.size; ++env, ++e)
        e->restartReq = 1;
}


void MbCv::triggerNoteOff(MbCvVoice *v)
{
    MbCvEnv *e = mbCvEnv.first();
    for(int env=0; env < mbCvEnv.size; ++env, ++e)
        e->releaseReq = 1;
}
