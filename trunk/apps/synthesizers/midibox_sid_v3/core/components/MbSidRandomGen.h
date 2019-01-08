/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Random Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_RANDOM_GEN_H
#define _MB_SID_RANDOM_GEN_H

#include <mios32.h>


class MbSidRandomGen
{
public:
    // Constructor
    MbSidRandomGen();

    // Destructor
    ~MbSidRandomGen();

    // random functions
    u32 value(void);
    u32 value(u32 max);
    u32 value(u32 min, u32 max);
};

#endif /* _MB_SID_RANDOM_GEN_H */
