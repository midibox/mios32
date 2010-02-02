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

#ifndef _MB_SID_VOICE_H
#define _MB_SID_VOICE_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidMidiVoice.h"
//#include "MbSidSe.h"
class MbSidSe; // forward declaration, see also http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.11


class MbSidVoice
{
public:
    // Constructor
    MbSidVoice();

    // Destructor
    ~MbSidVoice();

    // reference to assigned MIDI voice
    // has to be set by MbSid.cpp before voices are used!
    MbSidMidiVoice *midiVoicePtr;

    // voice init functions
    virtual void init(u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice);
    virtual void init();

    // input parameters
    bool voiceLegato;
    bool voiceWavetableOnly;
    bool voiceSusKey;
    bool voicePoly;
    bool voiceConstantTimeGlide;
    bool voiceGlissandoMode;
    bool voiceGateStaysActive;
    bool voiceAdsrBugWorkaround;
    bool voiceWaveformOff;
    bool voiceWaveformSync;
    bool voiceWaveformRingmod;
    u8 voiceWaveform;
    sid_se_voice_ad_t voiceAttackDecay;
    sid_se_voice_sr_t voiceSustainRelease;
    u16 voicePulsewidth;
    u8 voiceAccentRate; // used by Bassline Engine only - Lead engine uses this to set SwinSID Phase (TODO: separate unions for Lead/Bassline?)
    u8 voiceDelay;
    u8 voiceTranspose;
    u8 voiceFinetune;
    s8 voiceDetuneDelta;
    u8 voicePitchrange;
    u8 voicePortamentoRate;
    u8 voiceSwinSidMode; // sid_se_voice_swinsid_mode_t
    u8 voiceOscPhase;

    s32 voicePitchModulation;
    s32 voicePulsewidthModulation;


    // voice handlers
    virtual bool gate(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);
    virtual void pitch(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);
    virtual void pw(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);

    bool noteOn(u8 note, u8 velocity);
    bool noteOff(u8 note, u8 lastFirstNote);
    void gateOn(void);
    void gateOff_SingleVoice(void);
    void gateOff(u8 note);

    void playWtNote(MbSidSe *se, MbSidVoice *v, u8 wtValue);

    // some flags
    bool voiceActive;
    bool voiceDisabled;
    bool voiceGateActive;
    bool voiceGateSetReq;
    bool voiceGateClrReq;
    bool voiceOscSyncInProgress;
    bool voicePortamentoActive;
    bool voicePortamentoInitialized;
    bool voiceAccentActive;
    bool voiceSlideActive;
    bool voiceForceFrqRecalc; // forces SID frequency re-calculation (if SID frequency registers have been changed externally)
    bool voiceNoteRestartReq;

    // some variables
    u8  voiceNum; // number of assigned voice
    u8  physVoiceNum; // number of assigned physical SID voice
    sid_voice_t *physSidVoice; // reference to SID register
    u8  voiceNote;
    u8  voiceForcedNote;
    u8  voicePlayedNote;
    u8  voicePrevTransposedNote;
    u8  voiceGlissandoNote;
    u16 voicePortamentoBeginFrq;
    u16 voicePortamentoEndFrq;
    u16 voicePortamentoCtr;
    u16 voiceLinearFrq;
    u16 voiceSetDelayCtr;
    u16 voiceClrDelayCtr;

    u8  voiceAssignedInstrument;

    // cross-references
    sid_se_trg_t *trgMaskNoteOn;
    sid_se_trg_t *trgMaskNoteOff;

protected:
};

#endif /* _MB_SID_VOICE_H */
