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

#ifndef _MB_CV_VOICE_H
#define _MB_CV_VOICE_H

#include <mios32.h>
#include "MbCvStructs.h"
#include "MbCvMidiVoice.h"

class MbCvVoice
{
public:
    // Constructor
    MbCvVoice();

    // Destructor
    ~MbCvVoice();

    // reference to assigned MIDI voice
    // has to be set by MbCv.cpp before voices are used!
    MbCvMidiVoice *midiVoicePtr;

    // voice init functions
    virtual void init(u8 _voiceNum, MbCvMidiVoice* _midiVoicePtr);
    virtual void init();

    // input parameters
    bool voiceLegato;
    bool voiceSusKey;
    bool voiceSequencerOnly;
    bool voicePoly;
    bool voiceGateInverted;
    bool voiceConstantTimeGlide;
    bool voiceGlissandoMode;
    u8 voiceAccentRate; // used by Bassline sequencer only
    s8 voiceKeytrack;
    s8 voiceTransposeOctave;
    s8 voiceTransposeSemitone;
    s8 voiceFinetune;
    u8 voicePitchrange;
    u8 voicePortamentoRate;

    // output
    s32 voicePitchModulation;

    // for AOUT mapping
    mbcv_midi_event_mode_t voiceEventMode;
    u16 voiceFrq;
    u8  voiceVelocity;

    // voice handlers
    virtual bool gate(const u8 &updateSpeedFactor);
    virtual void pitch(const u8 &updateSpeedFactor);

    bool noteOn(u8 note, u8 velocity);
    bool noteOff(u8 note, u8 lastFirstNote);
    void gateOn(void);
    void gateOff_SingleVoice(void);
    void gateOff(u8 note);

    // some flags
    bool voiceActive;
    bool voiceDisabled;
    bool voiceGateActive;
    bool voicePhysGateActive;
    bool voiceGateSetReq;
    bool voiceGateClrReq;
    bool voicePortamentoActive;
    bool voicePortamentoInitialized;
    bool voiceAccentActive;
    bool voiceSlideActive;
    bool voiceForceFrqRecalc; // forces CV frequency re-calculation (if CV frequency registers have been changed externally)
    bool voiceNoteRestartReq;

    // some variables
    u8  voiceNum; // number of assigned channel
    u8  voiceNote;
    u8  voiceForcedNote;
    u8  voicePlayedNote;
    u8  voicePrevTransposedNote;
    u8  voiceGlissandoNote;
    u16 voicePortamentoBeginFrq;
    u16 voicePortamentoEndFrq;
    u16 voicePortamentoCtr;
    u16 voiceLinearFrq;
    u8  voiceRetriggerDelay;
    u8  voiceSetDelayCtr;

    u8  voiceAssignedInstrument;

    // help functions for editor
    void setAoutCurve(u8 value);
    u8 getAoutCurve(void);
    const char* getAoutCurveName();

    void setAoutSlewRate(u8 value);
    u8 getAoutSlewRate(void);

    void setPortamentoMode(u8 value);
    u8 getPortamentoMode(void);

    // transpose any value (used by Event modes != Note)
    u16 transpose(u16 value);

protected:
};

#endif /* _MB_CV_VOICE_H */
