/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Sequencer Prototype
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SEQ_H
#define _MB_SID_SEQ_H

#include <mios32.h>
#include "MbSidStructs.h"


class MbSidSeq
{
public:
    // Constructor
    MbSidSeq();

    // Destructor
    ~MbSidSeq();

    // sequencer init function
    void init(void);

    // input parameters
    bool seqEnabled;
    bool seqClockReq;
    bool seqRestartReq;
    bool seqStopReq;

    u8 seqPatternNumber;
    u8 seqPatternLength;
    u8 *seqPatternMemory;
    u8 seqClockDivider;
    bool seqSynchToMeasure;
    u8 seqParameterAssign;

    // variables
    bool seqRunning;
    u8 seqPos;
    u8 seqDivCtr;
    u8 seqSubCtr;

protected:
    bool seqEnabledSaved;
};

#endif /* _MB_SID_SEQ_H */
