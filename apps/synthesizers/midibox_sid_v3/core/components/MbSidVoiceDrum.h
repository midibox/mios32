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


class MbSidVoiceDrum : public MbSidVoice
{
public:
    // Constructor
    MbSidVoiceDrum();

    // Destructor
    ~MbSidVoiceDrum();

    // voice init functions
    void init();

    // see MbSidVoice.h for input parameters

    // additional parameters for drum engine:
    u8  drumWaveform;
    u8  drumGatelength;
    sid_drum_model_t *drumModel;
};

#endif /* _MB_SID_VOICE_DRUM_H */
