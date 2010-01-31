/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SE_DRUM_H
#define _MB_SID_SE_DRUM_H

#include <mios32.h>
#include "MbSidSe.h"

class MbSidSeDrum : public MbSidSe
{
public:
    // Constructor
    MbSidSeDrum();

    // Destructor
    ~MbSidSeDrum();

    // voices and filters
    array<MbSidVoiceDrum, 6> mbSidVoiceDrum;
    array<MbSidFilter, 2> mbSidFilter;

    MbSidSeqDrum mbSidSeqDrum;
};

#endif /* _MB_SID_SE_DRUM_H */
