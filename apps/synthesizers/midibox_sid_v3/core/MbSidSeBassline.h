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

#ifndef _MB_SID_SE_BASSLINE_H
#define _MB_SID_SE_BASSLINE_H

#include <mios32.h>
#include "MbSidSe.h"

class MbSidSeBassline : public MbSidSe
{
public:
    // Constructor
    MbSidSeBassline();

    // Destructor
    ~MbSidSeBassline();

    // overloaded functions

    // initializes sound engine
    void init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr, MbSidPatch *_mbSidPatchPtr);

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
    array<MbSidLfo, 4> mbSidLfo;
    array<MbSidEnv, 2> mbSidEnv;
    array<MbSidArp, 2> mbSidArp;
    array<MbSidSeqBassline, 2> mbSidSeqBassline;
};

#endif /* _MB_SID_SE_BASSLINE_H */
