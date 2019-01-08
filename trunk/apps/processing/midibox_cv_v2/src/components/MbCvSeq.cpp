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

#include <string.h>
#include "MbCvSeq.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvSeq::MbCvSeq()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvSeq::~MbCvSeq()
{
}


/////////////////////////////////////////////////////////////////////////////
// Sequencer init function
/////////////////////////////////////////////////////////////////////////////
void MbCvSeq::init(void)
{
    seqEnabled = false;
    seqClockReq = false;
    seqRestartReq = false;
    seqStopReq = false;

    seqPatternNumber = 0;
    seqPatternLength = 15;
    seqResolution = 28; // 1/16th
    seqSynchToMeasure = false;

    seqRunning = 0;
    seqPos = 0;
    seqDivCtr = 0;

    seqEnabledSaved = false;
}
