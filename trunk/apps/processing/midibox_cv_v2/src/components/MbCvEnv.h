/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ENV_H
#define _MB_CV_ENV_H

#include <MbCvEnvBase.h>

class MbCvEnv
    : public MbCvEnvBase
{
public:
    // Constructor
    MbCvEnv();

    // Destructor
    ~MbCvEnv();

    // ENV init function
    virtual void init(void);

    // ENV handler (returns true when sustain phase reached)
    virtual bool tick(const u8 &updateSpeedFactor);

    // additional input parameters (see also MbCvEnvBase.h)
    u8 envDelay;
    u8 envAttack;
    u8 envDecay;
    u8 envDecayAccented;
    u8 envSustain;
    u8 envRelease;


    // requests to use accented parameters
    bool accentReq;

protected:
    u16 envInitialLevel;

};

#endif /* _MB_CV_ENV_H */
