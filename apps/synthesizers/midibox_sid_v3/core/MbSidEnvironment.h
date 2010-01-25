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
#include "MbSidSysEx.h"
#include "MbSidAsid.h"
#include "MbSidClock.h"

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
    // returns true if SID registers have to be updated
    bool updateSe(void);

    // speed factor compared to MBSIDV2
    u8 updateSpeedFactor;

    // set/get BPM
    void bpmSet(float bpm);
    float bpmGet(void);

    // resets internal Tempo generator
    void bpmRestart(void);

    // MIDI
    void midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
    s32 midiReceiveSysEx(mios32_midi_port_t port, u8 midi_in);
    void midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in);
    void midiTimeOut(mios32_midi_port_t port);

protected:
    // SysEx parsers
    MbSidSysEx mbSidSysEx;
    MbSidAsid mbSidAsid;

    // Tempo Clock
    MbSidClock mbSidClock;
};

#endif /* _MB_SID_ENVIRONMENT_H */
