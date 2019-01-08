/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Filter Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_FILTER_H
#define _MB_SID_FILTER_H

#include <mios32.h>
#include "MbSidStructs.h"


class MbSidFilter
{
public:
    // Constructor
    MbSidFilter();

    // Destructor
    ~MbSidFilter();

    // filter init function
    void init(sid_regs_t *_physSidRegs);
    void init(void);

    // input parameters
    u16 filterCutoff; // 12bit used
    u8 filterResonance;
    u8 filterKeytrack;
    u8 filterChannels;
    u8 filterMode;
    u8 filterVolume;

    s32 filterCutoffModulation;
    s32 filterVolumeModulation;
    u16 filterKeytrackFrequency;

    // filter handlers
    void tick(const u8 &updateSpeedFactor);

    // reference to SID registers
    sid_regs_t *physSidRegs;
};

#endif /* _MB_SID_FILTER_H */
