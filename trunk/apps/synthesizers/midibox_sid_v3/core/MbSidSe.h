/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Sound Engine Toplevel
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SE_H
#define _MB_SID_SE_H


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

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidVoiceQueue.h"
#include "MbSidPatch.h"
#include "MbSidClock.h"
#include "MbSidRandomGen.h"
#include "MbSidMidiVoice.h"
#include "MbSidVoice.h"
#include "MbSidVoiceDrum.h"
#include "MbSidFilter.h"
#include "MbSidLfo.h"
#include "MbSidEnv.h"
#include "MbSidEnvLead.h"
#include "MbSidWt.h"
#include "MbSidWtDrum.h"
#include "MbSidMod.h"
#include "MbSidArp.h"
#include "MbSidSeq.h"
#include "MbSidSeqBassline.h"
#include "MbSidSeqDrum.h"
#include "MbSidDrum.h"


class MbSidSe
{
public:
    // Constructor
    MbSidSe();

    // Destructor
    ~MbSidSe();

    // reference to clock generator
    MbSidClock *mbSidClockPtr;

    // sound patch (pointer has to be initialized externaly (MbSid.cpp))
    MbSidPatch *mbSidPatchPtr;

    // NOTE: we don't use "virtual ... = 0" constructs here,
    // since it increases the binary size and RAM consumption on STM32!

    // Initialises the structures of a SID sound engine
    virtual void init(sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *mbSidClockPtr, MbSidPatch *_mbSidPatchPtr) {};

    // Initialises the structures of a SID sound engine
    virtual void initPatch(bool patchOnly) {};

    // sound engine update cycle
    // returns true if SID registers have to be updated
    virtual bool tick(const u8 &updateSpeedFactor) { return false; };

    // Trigger matrix
    virtual void triggerNoteOn(MbSidVoice *v, u8 no_wt) {};
    virtual void triggerNoteOff(MbSidVoice *v, u8 no_wt) {};

    // Callback from MbSidSysEx to set a dedicated SysEx parameter
    // (forwarded from MbSidEnvironment and MbSid)
    virtual bool sysexSetParameter(u16 addr, u8 data) { return false; }; // has to be implemented in engine, returns false on invalid access

    // MIDI access
    virtual void midiReceiveNote(u8 chn, u8 note, u8 velocity) {};
    virtual void midiReceiveCC(u8 chn, u8 ccNumber, u8 value) {};
    virtual void midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit) {};
    virtual void midiReceiveAftertouch(u8 chn, u8 value) {};

    // Note access independent from channel
    virtual void noteOn(u8 instrument, u8 note, u8 velocity, bool bypassNotestack) {};
    virtual void noteOff(u8 instrument, u8 note, bool bypassNotestack) {};
    virtual void noteAllOff(u8 instrument, bool bypassNotestack) {};

    // access knob values
    virtual void knobSet(u8 knob, u8 value) {};
    virtual u8 knobGet(u8 knob) { return 0; };

    // parameter access
    virtual void parSet(u8 par, u16 value, u8 sidlr, u8 ins, bool scaleFrom16bit) {};
    virtual void parSetNRPN(u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins) {};
    virtual void parSetWT(u8 par, u8 wtValue, u8 sidlr, u8 ins) {};
    virtual u16 parGet(u8 par, u8 sidlr, u8 ins, bool scaleTo16bit) { return 0; };

};

#endif /* _MB_SID_SE_H */
