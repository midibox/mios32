/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Filter/Volume handler
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
#include "MbSidFilter.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidFilter::MbSidFilter()
{
    init(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidFilter::~MbSidFilter()
{
}


/////////////////////////////////////////////////////////////////////////////
// Filter init function with register assignments
// (usually only called once after startup)
/////////////////////////////////////////////////////////////////////////////
void MbSidFilter::init(sid_regs_t *_physSidRegs)
{
    physSidRegs = _physSidRegs;

    init();
}


/////////////////////////////////////////////////////////////////////////////
// Filter init function
/////////////////////////////////////////////////////////////////////////////
void MbSidFilter::init()
{
    filterCutoff = 0xfff; // maximum
    filterResonance = 0;
    filterKeytrack = 0;
    filterChannels = 0;
    filterMode = 0;
    filterVolume = 0;

    filterCutoffModulation = 0;
    filterVolumeModulation = 0;
    filterKeytrackFrequency = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Filter handler
/////////////////////////////////////////////////////////////////////////////
void MbSidFilter::tick(const u8 &updateSpeedFactor)
{
    int cutoff = filterCutoff;

    // cutoff modulation (/8 instead of /16 to simplify extreme modulation results)
    cutoff += filterCutoffModulation / 8;
    if( cutoff > 0xfff ) cutoff = 0xfff; else if( cutoff < 0 ) cutoff = 0;

    if( filterKeytrack ) {
        s32 delta = (filterKeytrackFrequency * filterKeytrack) / 0x1000; // 24bit -> 12bit
        // bias at C-3 (0x3c)
        delta -= (0x3c << 5);

        cutoff += delta;
        if( cutoff > 0xfff ) cutoff = 0xfff; else if( cutoff < 0 ) cutoff = 0;
    }

    // calibration
    // TODO: take calibration values from ensemble
    u16 cali_min = 0;
    u16 cali_max = 1536;
    cutoff = cali_min + ((cutoff * (cali_max-cali_min)) / 4096);

    // map 12bit value to 11 value of SID register
    physSidRegs->filter_l = (cutoff >> 1) & 0x7;
    physSidRegs->filter_h = (cutoff >> 4);

    // resonance (4bit only)
    physSidRegs->resonance = filterResonance >> 4;

    // filter channel/mode selection
    physSidRegs->filter_select = filterChannels;
    physSidRegs->filter_mode = filterMode;

    // volume
    int volume = filterVolume << 9;

    // volume modulation
    volume += filterVolumeModulation;
    if( volume > 0xffff ) volume = 0xffff; else if( volume < 0 ) volume = 0;

    physSidRegs->volume = volume >> 12;
}
