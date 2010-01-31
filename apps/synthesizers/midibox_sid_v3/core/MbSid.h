/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_H
#define _MB_SID_H

#include <mios32.h>
#include "MbSidSeLead.h"
#include "MbSidSeBassline.h"
#include "MbSidSeDrum.h"
#include "MbSidSeMulti.h"
#include "MbSidPatch.h"
#include "MbSidMidiVoice.h"

class MbSid
{
public:
    // Constructor
    MbSid();

    // Destructor
    ~MbSid();

    // initializes the sound engines
    void init(u8 _sidNum, sid_regs_t *sidRegLPtr, sid_regs_t *sidRegRPtr, MbSidClock *_mbSidClockPtr);

    // sound engine update cycle
    // returns true if SID registers have to be updated
    bool tick(const u8 &updateSpeedFactor);

    // MIDI access
    void midiReceive(mios32_midi_package_t midi_package);
    void midiReceiveNote(u8 chn, u8 note, u8 velocity);
    void midiReceiveCC(u8 chn, u8 ccNumber, u8 value);
    void midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit);
    void midiReceiveAftertouch(u8 chn, u8 value);

    // should be called whenver the patch has been changed
    void updatePatch(bool forceEngineInit);

    // callbacks for MbSidSysEx (forwarded from MbSidEnvironment)
    bool sysexSetPatch(sid_patch_t *p); // returns false if initialisation failed
    bool sysexGetPatch(sid_patch_t *p); // returns false if patch not available
    bool sysexSetParameter(u16 addr, u8 data); // returns false on invalid access

    // sound patch
    MbSidPatch mbSidPatch;

    // engines
    MbSidSeLead mbSidSeLead;
    MbSidSeBassline mbSidSeBassline;
    MbSidSeDrum mbSidSeDrum;
    MbSidSeMulti mbSidSeMulti;

    // pointer to current engine
    MbSidSe *currentMbSidSePtr;

    // MIDI voices
    array<MbSidMidiVoice, 6> mbSidMidiVoice;

protected:
    // previous engine (used by MbSid::updatePatch())
    sid_se_engine_t prevEngine;
};

#endif /* _MB_SID_H */
