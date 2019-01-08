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

#include "MbCvRandomGen.h"

#include <jsw_rand.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvRandomGen::MbCvRandomGen()
{
    // initial seed
	jsw_seed(0xcafebabe);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvRandomGen::~MbCvRandomGen()
{
}


/////////////////////////////////////////////////////////////////////////////
// returns a 32bit random number
/////////////////////////////////////////////////////////////////////////////
u32 MbCvRandomGen::value(void)
{
    return jsw_rand();
}

/////////////////////////////////////////////////////////////////////////////
// returns random number between 0 and max
/////////////////////////////////////////////////////////////////////////////
u32 MbCvRandomGen::value(u32 max)
{
    // return result within the given range
    return max == 0xffffffff ? value() : (value() % (max+1));
}

/////////////////////////////////////////////////////////////////////////////
// returns random number in a given range (max has to be < 0xffffffff)
/////////////////////////////////////////////////////////////////////////////
u32 MbCvRandomGen::value(u32 min, u32 max)
{
    // values equal? -> no random number required
    if( min == max )
        return min;

    // swap min/max if reversed
    if( min > max ) {
        u32 tmp;
        tmp = min;
        min = max;
        max = tmp;
    }

    // return result within the given range
    return min + value(max-min);
}
