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

#ifndef _MB_SID_SE_MULTI_H
#define _MB_SID_SE_MULTI_H

#include <mios32.h>
#include "MbSidSe.h"

class MbSidSeMulti : public MbSidSe
{
public:
    // Constructor
    MbSidSeMulti();

    // Destructor
    ~MbSidSeMulti();

    // overloaded functions

    // initializes sound engine
    void init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr);

    // reference to first MIDI voice
    // has to be set by MbSid.cpp before voices are used!
    MbSidMidiVoice *midiVoicePtr;

    // Initialises the structures of a SID sound engine
    void initPatch(bool patchOnly);

    // sound engine update cycle
    // returns true if SID registers have to be updated
    bool tick(const u8 &updateSpeedFactor);

    // Trigger matrix
    void triggerNoteOn(MbSidVoice *v, u8 no_wt);
    void triggerNoteOff(MbSidVoice *v, u8 no_wt);

    // MIDI access
    void midiReceiveNote(u8 chn, u8 note, u8 velocity);
    void midiReceiveCC(u8 chn, u8 ccNumber, u8 value);
    void midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit);
    void midiReceiveAftertouch(u8 chn, u8 value);

    // Note access independent from channel
    void noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack);
    void noteOff(u8 instrument, u8 note, bool bypassNotestack);
    void noteAllOff(u8 instrument, bool bypassNotestack);

    // searches for next voice
    MbSidVoice *voiceGet(u8 instrument, sid_se_voice_patch_t *voicePatch, bool polyMode);

    // access knob values
    void knobSet(u8 knob, u8 value);
    u8 knobGet(u8 knob);

    // parameter access
    void parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit);
    void parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins);
    u16 parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit);


    // Callback from MbSidSysEx to set a dedicated SysEx parameter
    // (forwarded from MbSidEnvironment and MbSid)
    bool sysexSetParameter(u16 addr, u8 data); // returns false on invalid access

    // voices and filters
    array<MbSidVoice, 6> mbSidVoice;
    array<MbSidFilter, 2> mbSidFilter;

    // modulators
    array<MbSidLfo, 12> mbSidLfo;
    array<MbSidEnv, 6> mbSidEnv;
    array<MbSidWt, 6> mbSidWt;
    array<MbSidArp, 6> mbSidArp;

    // voice queue
    MbSidVoiceQueue voiceQueue;


protected:

    // Utility function to update a single voice
    void sysexSetParameterVoice(MbSidVoice *v, u8 voiceAddr, u8 data);

};

#endif /* _MB_SID_SE_MULTI_H */
