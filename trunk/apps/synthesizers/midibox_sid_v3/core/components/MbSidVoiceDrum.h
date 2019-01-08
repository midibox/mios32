/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Voice Handlers for Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_VOICE_DRUM_H
#define _MB_SID_VOICE_DRUM_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidVoice.h"
#include "MbSidDrum.h"

class MbSidWtDrum; // forward declaration

class MbSidVoiceDrum : public MbSidVoice
{
public:
    // Constructor
    MbSidVoiceDrum();

    // Destructor
    ~MbSidVoiceDrum();

    // voice init functions
    void init(u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice);
    void init();
    void init(MbSidDrum *d, u8 note, u8 velocity);

    void tick(const u8 &updateSpeedFactor, MbSidSe *mbSidSe, MbSidWtDrum *w);

    void pitch(const u8 &updateSpeedFactor, MbSidSe *mbSidSe);

    // see MbSidVoice.h for input parameters

    // additional parameters for drum engine:
    u8  drumGatelength;
    u8  drumSpeed;
    MbSidDrum *mbSidDrumPtr;
};

#endif /* _MB_SID_VOICE_DRUM_H */
