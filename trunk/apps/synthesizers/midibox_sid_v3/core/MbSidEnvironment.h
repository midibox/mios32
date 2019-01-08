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

    // instantiate the MbSid instances
    array<MbSid, SID_SE_NUM> mbSid;

    // sets the update factor
    void updateSpeedFactorSet(u8 _updateSpeedFactor);

    // Sound Engines Update Cycle
    // returns true if SID registers have to be updated
    bool tick(void);

    // speed factor compared to MBSIDV2
    u8 updateSpeedFactor;

    // set/get BPM
    void bpmSet(float bpm);
    float bpmGet(void);

    // resets internal Tempo generator
    void bpmRestart(void);

    // bank access
    s32 bankSave(u8 sid, u8 bank, u8 patch);
    s32 bankLoad(u8 sid, u8 bank, u8 patch);
    s32 bankPatchNameGet(u8 bank, u8 patch, char *buffer);

    // MIDI
    void midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
    s32 midiReceiveSysEx(mios32_midi_port_t port, u8 midi_in);
    void midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in);
    void midiTimeOut(mios32_midi_port_t port);

    // callbacks for MbSidSysEx
    bool sysexSetPatch(u8 sid, sid_patch_t *p, bool toBank, u8 bank, u8 patch); // returns false if SID not available
    bool sysexGetPatch(u8 sid, sid_patch_t *p, bool fromBank, u8 bank, u8 patch); // returns false if SID not available
    bool sysexSetParameter(u8 sid, u16 addr, u8 data); // returns false if SID not available
    
    // SysEx parsers
    MbSidSysEx mbSidSysEx;
    MbSidAsid mbSidAsid;

    // Tempo Clock
    MbSidClock mbSidClock;
};

#endif /* _MB_SID_ENVIRONMENT_H */
