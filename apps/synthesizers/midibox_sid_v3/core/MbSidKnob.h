/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Knob
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_KNOB_H
#define _MB_SID_KNOB_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidSe.h"

typedef struct mbsid_knob_t {
  u8 assign1;
  u8 assign2;
  u8 value;
  u8 min;
  u8 max;
} mbsid_knob_t;


class MbSidKnob
{
public:
    // Constructor
    MbSidKnob();

    // Destructor
    ~MbSidKnob();

    // knob number
    u8 knobNum;

    // references
    MbSidSe  *mbSidSePtr;

    // set functions
    void set(u8 value);

private:

};

#endif /* _MB_SID_KNOB_H */
