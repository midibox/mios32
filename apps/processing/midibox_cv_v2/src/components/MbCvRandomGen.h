/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Random Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_RANDOM_GEN_H
#define _MB_CV_RANDOM_GEN_H

#include <mios32.h>


class MbCvRandomGen
{
public:
    // Constructor
    MbCvRandomGen();

    // Destructor
    ~MbCvRandomGen();

    // random functions
    u32 value(void);
    u32 value(u32 max);
    u32 value(u32 min, u32 max);
};

#endif /* _MB_CV_RANDOM_GEN_H */
