/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Sequencer Prototype
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_SEQ_H
#define _MB_CV_SEQ_H

#include <mios32.h>
#include "MbCvStructs.h"


class MbCvSeq
{
public:
    // Constructor
    MbCvSeq();

    // Destructor
    ~MbCvSeq();

    // sequencer init function
    void init(void);

    // input parameters
    bool seqEnabled;
    bool seqClockReq;
    bool seqRestartReq;
    bool seqStopReq;

    u8 seqPatternNumber;
    u8 seqPatternLength;
    u8 seqResolution;
    bool seqSynchToMeasure;

    // variables
    bool seqRunning;
    u8 seqPos;
    u8 seqCurrentPattern;
    u8 seqDivCtr;
    u8 seqSubCtr;

protected:
    bool seqEnabledSaved;
};

#endif /* _MB_CV_SEQ_H */
