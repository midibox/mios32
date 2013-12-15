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

#ifndef _MB_CV_H
#define _MB_CV_H

#include <mios32.h>
#include "MbCvClock.h"
#include "MbCvMidiVoice.h"
#include "MbCvVoice.h"
#include "MbCvLfo.h"
#include "MbCvEnv.h"
#include "MbCvEnvMulti.h"
#include "MbCvArp.h"
#include "MbCvSeqBassline.h"
#include "MbCvMod.h"


// little helper
// a container class which defines an array leads to more robust code when iterating over the array
// will be transfered to a better location later
template <class T, int N> class array
{
protected:
    T element[N];

public:
    array() : size(N) {}

    const int size;

    inline T &operator[](int n)
    {
        if( n < 0 || n >= N ) {
#if 0
            DEBUG_MSG("trouble accessing element %d of array (max: %d)\n", n, N);
#endif
            return element[0];
        }
        return element[n];
    }

    inline T *first()
    {
        return &element[0];
    }

    inline T *last()
    {
        return &element[N-1];
    }

    inline T *next(T *prevElement)
    {
        return (++prevElement >= &element[N]) ? 0 : prevElement;
    }
};


class MbCv
{
public:
    // Constructor
    MbCv();

    // Destructor
    ~MbCv();

    // reference to clock generator
    MbCvClock *mbCvClockPtr;

    // initializes the sound engines
    void init(u8 _cvNum, MbCvClock *_mbCvClockPtr);

    // sound engine update cycle
    // returns true if CV registers have to be updated
    bool tick(const u8 &updateSpeedFactor);

    // MIDI access
    void midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
    void midiReceiveNote(u8 chn, u8 note, u8 velocity);
    void midiReceiveCC(u8 chn, u8 ccNumber, u8 value);
    void midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit);
    void midiReceiveAftertouch(u8 chn, u8 value);

    bool setNRPN(u16 nrpnNumber, u16 value);
    bool getNRPN(u16 nrpnNumber, u16 *value);

    // should be called whenver the patch has been changed
    void updatePatch(bool forceEngineInit);

    // MIDI voice (only one per channel)
    MbCvMidiVoice mbCvMidiVoice;

    // CV voice
    MbCvVoice mbCvVoice;

    // modulators
    array<MbCvLfo, 2> mbCvLfo;
    array<MbCvEnv, 1> mbCvEnv1;
    array<MbCvEnvMulti, 1> mbCvEnv2;
    MbCvArp mbCvArp;
    MbCvSeqBassline mbCvSeqBassline;

    // modulation matrix
    MbCvMod mbCvMod;

    // scope parameters
    u8 scopeSelect;
    u8 scopeOversamplingFactor;
    u8 scopeTriggerLevelPercent;

    // note handling
    void noteOn(u8 note, u8 velocity, bool bypassNotestack);
    void noteOff(u8 note, bool bypassNotestack);
    void noteAllOff(bool bypassNotestack);

    // trigger handling
    void triggerNoteOn(MbCvVoice *v);
    void triggerNoteOff(MbCvVoice *v);

};

#endif /* _MB_CV_H */
