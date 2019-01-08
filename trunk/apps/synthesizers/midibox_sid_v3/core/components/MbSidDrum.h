/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Instrument
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_DRUM_H
#define _MB_SID_DRUM_H

#include <mios32.h>
#include "MbSidStructs.h"


class MbSidDrum
{
public:
    // Constructor
    MbSidDrum();

    // Destructor
    ~MbSidDrum();

    // init functions
    void init();

    // access functions to drum model
    u8 getNote(u8 pos);
    u8 getWaveform(u8 pos);
    u8 getGatelength(void);
    u8 getSpeed(void);
    u16 getPulsewidth(void);
    u8 getLoop(void);


    // parameters of a drum 
    u8 drumVoiceAssignment;
    u8 drumModel;
    sid_se_voice_ad_t drumAttackDecay;
    sid_se_voice_sr_t drumSustainRelease;
    s8 drumTuneModifier;
    s8 drumGatelengthModifier;
    s8 drumSpeedModifier;
    s8 drumParameter;
    u8 drumVelocityAssignment;

};

#endif /* _MB_SID_DRUM_H */
