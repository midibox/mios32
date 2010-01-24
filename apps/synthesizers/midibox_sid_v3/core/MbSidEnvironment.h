/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Toplevel
 * Instantiates multiple MIDIbox SID Units
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_ENVIRONMENT_H
#define _MB_SID_ENVIRONMENT_H

#include <mios32.h>
#include "MbSid.h"
#include "MbSidStructs.h"

class MbSidEnvironment
{
public:
    // Constructor
    MbSidEnvironment();

    // Destructor
    ~MbSidEnvironment();

    MbSid mbSid[SID_SE_NUM];

    void test();

    // sets the update factor
    void updateSpeedFactorSet(u8 _updateSpeedFactor);

    // Sound Engines Update Cycle
    void updateSe(void);

    // speed factor compared to MBSIDV2
    u8 updateSpeedFactor;

    // tempo events
    sid_se_clk_t mbSidClk;

    // MIDI
    void midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package);

    // Clock Generator
    void incomingRealTimeEvent(u8 event);
    void bpmSet(float bpm);
    void bpmRestart(void);
    void clock(void);

};

#endif /* _MB_SID_ENVIRONMENT_H */
