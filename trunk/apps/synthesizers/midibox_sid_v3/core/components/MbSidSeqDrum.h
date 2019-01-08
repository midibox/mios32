/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Bassline Sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SEQ_DRUM_H
#define _MB_SID_SEQ_DRUM_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidSeq.h"
#include "MbSidVoice.h"

class MbSidSe; // forward declaration

class MbSidSeqDrum : public MbSidSeq
{
public:
    // Constructor
    MbSidSeqDrum();

    // Destructor
    ~MbSidSeqDrum();

    // sequencer init function
    void init(void);

    // sequencer handler
    void tick(MbSidSe *se);

    // input parameters: see also MbSidSeq.h

};

#endif /* _MB_SID_SEQ_DRUM_H */
