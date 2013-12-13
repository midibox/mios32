/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Toplevel
 * Instantiates multiple MIDIbox CV Units
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ENVIRONMENT_H
#define _MB_CV_ENVIRONMENT_H

#include <mios32.h>
#include "MbCv.h"
#include "MbCvStructs.h"
//#include "MbCvSysEx.h"
#include "MbCvClock.h"
#include "MbCvPatch.h"
#include "MbCvScope.h"

class MbCvEnvironment
{
public:
    // Constructor
    MbCvEnvironment();

    // Destructor
    ~MbCvEnvironment();

    // instantiate the MbCv instances
    array<MbCv, CV_SE_NUM> mbCv;

    // CV Output value (will be mapped in MbCvEnvironment::tick() !!!)
    array<u16, CV_SE_NUM> cvOut;

    // CV Gates (will be mapped in MbCvEnvironment::tick() !!!)
    // up to 32 gates should be sufficient for future extensions? (currently we only use 8!)
    u32 cvGates;

    // sound patch
    MbCvPatch mbCvPatch;

    // instantiate the scopes for GLCD output
    array<MbCvScope, CV_SCOPE_NUM> mbCvScope;
    u8 scopeUpdateCtr;

    // sets the update factor
    void updateSpeedFactorSet(u8 _updateSpeedFactor);

    // Sound Engines Update Cycle
    // returns true if CV registers have to be updated
    bool tick(void);

    // Should be called each mS from a thread, e.g. for synchronized patch changes
    void tick_1mS(void);

    // Should be called each mS from a low-prio thread to update scope displays
    void tickScopes(void);

    // speed factor compared to MBCVV2
    u8 updateSpeedFactor;

    // set/get BPM
    void bpmSet(float bpm);
    float bpmGet(void);

    // resets internal Tempo generator
    void bpmRestart(void);

    // bank access
    s32 bankSave(u8 bank, u8 patch);
    s32 bankLoad(u8 bank, u8 patch, bool forceImmediateChange = false);
    s32 bankPatchNameGet(u8 bank, u8 patch, char *buffer);

    // MIDI
    void midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
    s32 midiReceiveSysEx(mios32_midi_port_t port, u8 midi_in);
    void midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in);
    void midiTimeOut(mios32_midi_port_t port);

    void midiSendNRPNDump(mios32_midi_port_t port, u16 cvChannels, bool seqOnly);
    void midiSendGlobalNRPNDump(mios32_midi_port_t port);
    void midiSendNRPN(mios32_midi_port_t port, u16 nrpnNumber, u16 value);

    // Copy/Paste/Clear operations
    void channelCopy(u8 channel, u16* buffer);
    void channelPaste(u8 channel, u16* buffer);
    void channelClear(u8 channel);

    // also used for load/store operations
    u16 copyBuffer[CV_PATCH_SIZE];

    // MIDI NRPN variables
    u16 nrpnAddress[16];
    u16 nrpnValue[16];
    u8  lastSentNrpnAddressMsb;
    u8  lastSentNrpnAddressLsb;

    // callbacks for MbCvSysEx
    bool sysexSetPatch(u8 cv, cv_patch_t *p, bool toBank, u8 bank, u8 patch); // returns false if CV not available
    bool sysexGetPatch(u8 cv, cv_patch_t *p, bool fromBank, u8 bank, u8 patch); // returns false if CV not available
    bool sysexSetParameter(u8 cv, u16 addr, u8 data); // returns false if CV not available
    
    // SysEx parsers
    //MbCvSysEx mbCvSysEx;

    // Tempo Clock
    MbCvClock mbCvClock;

    // last port and CV channels used for NRPN communication
    mios32_midi_port_t lastNrpnMidiPort;
    u16 lastNrpnCvChannels;

    // global timestamp (incremented with each tick())
    u32 timestamp;

};

#endif /* _MB_CV_ENVIRONMENT_H */
