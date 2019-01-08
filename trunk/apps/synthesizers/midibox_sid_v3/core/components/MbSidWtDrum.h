/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Wavetable Sequencer for Drum Models
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_WT_DRUM_H
#define _MB_SID_WT_DRUM_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidWt.h"
#include "MbSidVoiceDrum.h"

class MbSidWtDrum: public MbSidWt
{
public:
    // Constructor
    MbSidWtDrum();

    // Destructor
    ~MbSidWtDrum();

    // WT init function
    void init(void);

    // drum WT replacement
    bool tick(const u8 &updateSpeedFactor, MbSidVoiceDrum *v);

    // variables are defined in MbSidWt.cpp
};

#endif /* _MB_SID_WT_DRUM_H */
