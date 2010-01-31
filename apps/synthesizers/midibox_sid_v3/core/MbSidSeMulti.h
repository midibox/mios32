/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Multi Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SE_MULTI_H
#define _MB_SID_SE_MULTI_H

#include <mios32.h>
#include "MbSidSe.h"

class MbSidSeMulti : public MbSidSe
{
public:
    // Constructor
    MbSidSeMulti();

    // Destructor
    ~MbSidSeMulti();

    // voices and filters
    array<MbSidVoice, 6> mbSidVoice;
    array<MbSidFilter, 2> mbSidFilter;

    // modulators
    array<MbSidLfo, 12> mbSidLfo;
    array<MbSidEnv, 6> mbSidEnvLead;
    array<MbSidWt, 6> mbSidWt;
    array<MbSidArp, 6> mbSidArp;
};

#endif /* _MB_SID_SE_MULTI_H */
