/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Wavetable Sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_WT_H
#define _MB_SID_WT_H

#include <mios32.h>
#include "MbSidStructs.h"


class MbSidWt
{
public:
    // Constructor
    MbSidWt();

    // Destructor
    ~MbSidWt();

    // WT init function
    void init(void);

    // Wavetable handler (returns true on loop)
    bool tick(s32 step, const u8 &updateSpeedFactor);

    // input parameters
    u8 wtBegin;
    u8 wtEnd;
    u8 wtLoop;
    u8 wtSpeed;
    u8 wtAssign;
    u8 wtAssignLeftRight;
    bool wtOneshotMode;
    bool wtKeyControlMode;
    bool wtModControlMode;

    // output parameter
    s16 wtOut;

    // requests a restart and next clock (divided by WT)
    bool restartReq;
    bool clockReq;

    // position
    u8 wtPos;

protected:
    // internal variables
    u16 wtDivCtr; // should be >8bit for Drum mode (WTs clocked independent from BPM generator)
};

#endif /* _MB_SID_WT_H */
