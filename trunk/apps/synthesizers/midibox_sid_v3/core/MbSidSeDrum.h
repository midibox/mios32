/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SE_DRUM_H
#define _MB_SID_SE_DRUM_H

#include <mios32.h>
#include "MbSidSe.h"

class MbSidSeDrum : public MbSidSe
{
public:
    // Constructor
    MbSidSeDrum();

    // Destructor
    ~MbSidSeDrum();

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

    // Drum Sequencer: change pattern on keypress
    void seqChangePattern(u8 instrument, u8 note);

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

    // drum instruments
    array<MbSidDrum, 16> mbSidDrum;
    
    // voices and filters
    array<MbSidVoiceDrum, 6> mbSidVoiceDrum;
    array<MbSidWtDrum, 6> mbSidWtDrum;
    array<MbSidFilter, 2> mbSidFilter;

    // sequencer
    MbSidSeqDrum mbSidSeqDrum;

    // voice queue
    MbSidVoiceQueue voiceQueue;
};

#endif /* _MB_SID_SE_DRUM_H */
