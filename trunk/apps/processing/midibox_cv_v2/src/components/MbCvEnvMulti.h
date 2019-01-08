/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Envelope Generator with multiple stages
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ENV_MULTI_H
#define _MB_CV_ENV_MULTI_H

#include <MbCvEnvBase.h>

// number of steps
#define MBCV_ENV_MULTI_NUM_STEPS 16


class MbCvEnvMulti
    : public MbCvEnvBase
{
public:
    // Constructor
    MbCvEnvMulti();

    // Destructor
    ~MbCvEnvMulti();

    // ENV init function
    virtual void init(void);

    // ENV handler (returns true when sustain phase reached)
    virtual bool tick(const u8 &updateSpeedFactor);

    // additional input parameters (see also MbCvEnvBase.h)
    u8  envLevel[MBCV_ENV_MULTI_NUM_STEPS];

    s8  envOffset;
    u8  envRate;

    u8  envLoopAttack;
    u8  envSustainStep;
    u8  envLoopRelease;
    u8  envLastStep;

protected:
    u8  envCurrentStep;
    u8  envNextStep;

    void recalc(const u8 &updateSpeedFactor);
};

#endif /* _MB_CV_ENV_MULTI_H */
