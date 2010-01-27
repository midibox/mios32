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
//#include "MbSidSe.h"
class MbSidSe; // forward declaration, see also http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.11


class MbSidVoice
{
public:
    // Constructor
    MbSidVoice();

    // Destructor
    ~MbSidVoice();

    // voice init function
    void init(sid_se_voice_patch_t *_voicePatch, u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice, sid_se_midi_voice_t *_midiVoice);

    // voice handlers
    bool gate(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);
    void pitch(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);
    void pw(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);

    void pitchDrum(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);
    void pwDrum(const sid_se_engine_t &engine, const u8 &updateSpeedFactor, MbSidSe *mbSidSe);

    // Voice Patch
    sid_se_voice_patch_t *voicePatch;

    // some flags
    bool voiceActive;
    bool voiceDisabled;
    bool gateActive;
    bool gateSetReq;
    bool gateClrReq;
    bool oscSyncInProgress;
    bool portaActive;
    bool portaInitialized;
    bool accent;
    bool slide;
    bool forceFrqRecalc; // forces SID frequency re-calculation (if SID frequency registers have been changed externally)
    bool noteRestartReq;

    // some variables
    u8  voiceNum; // number of assigned voice
    u8  physVoiceNum; // number of assigned physical SID voice
    sid_voice_t *physSidVoice; // reference to SID register
    sid_se_midi_voice_t *midiVoice; // reference to assigned MIDI voice
    u8  note;
    u8  arpNote;
    u8  playedNote;
    u8  prevTransposedNote;
    u8  glissandoNote;
    u16 portamentoBeginFrq;
    u16 portamentoEndFrq;
    u16 portamentoCtr;
    u16 linearFrq;
    u16 setDelayCtr;
    u16 clrDelayCtr;

    u8  assignedInstrument;

    u8  drumWaveform;
    u8  drumGatelength;

    // cross-references
    s32    *modDstPitch; // reference to SID_SE_MOD_DST_PITCHx
    s32    *modDstPw; // reference to SID_SE_MOD_DST_PWx
    sid_se_trg_t *trgMaskNoteOn;
    sid_se_trg_t *trgMaskNoteOff;
    sid_drum_model_t *drumModel; // reference to assigned drum model

protected:
};

#endif /* _MB_SID_VOICE_H */
