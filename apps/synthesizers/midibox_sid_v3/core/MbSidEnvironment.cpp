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

#include "MbSidEnvironment.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment::MbSidEnvironment()
{
    // initialize structures of each SID engine
    for(int sid=0; sid<SID_SE_NUM; ++sid) {
        mbSid[sid].sidNum = sid;
        mbSid[sid].physSidL = 2*sid+0;
        mbSid[sid].sidRegLPtr = &sid_regs[2*sid+0];
        mbSid[sid].physSidR = 2*sid+1;
        mbSid[sid].sidRegRPtr = &sid_regs[2*sid+1];
        mbSid[sid].mbSidClockPtr = &mbSidClock;

        mbSid[sid].initStructs();
#if 0
        mbSid[sid].patchChanged();
#endif

        mbSid[sid].sendAlive();
    }

    // sets the speed factor
    updateSpeedFactorSet(2);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment::~MbSidEnvironment()
{
}


/////////////////////////////////////////////////////////////////////////////
// Changes the update speed factor
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvironment::updateSpeedFactorSet(u8 _updateSpeedFactor)
{
    updateSpeedFactor = _updateSpeedFactor;

    for(int sid=0; sid<SID_SE_NUM; ++sid)
        mbSid[sid].updateSpeedFactor = updateSpeedFactor;
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engines Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvironment::updateSe(void)
{
    bool updateRequired = false;

    if( mbSidAsid.modeGet() != MBSID_ASID_MODE_OFF )
        return false;

    // Tempo Clock
    mbSidClock.tick();

    // Engines
    for(int sid=0; sid<SID_SE_NUM; ++sid)
        if( mbSid[sid].updateSe() )
            updateRequired = true;

    return updateRequired;
}


/////////////////////////////////////////////////////////////////////////////
// Forwards incoming MIDI events to all MBSIDs
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvironment::midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    for(int sid=0; sid<SID_SE_NUM; ++sid)
        mbSid[sid].midiReceive(midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// called on incoming SysEx bytes
// should return 1 if SysEx data has been taken (and should not be forwarded
// to midiReceive)
/////////////////////////////////////////////////////////////////////////////
s32 MbSidEnvironment::midiReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
    s32 status = 0;

    status |= mbSidAsid.parse(port, midi_in);
    status |= mbSidSysEx.parse(port, midi_in);

    return status;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a realtime event to service MbSidClock
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvironment::midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in)
{
    mbSidClock.midiReceiveRealTimeEvent(port, midi_in);
}


/////////////////////////////////////////////////////////////////////////////
// called on MIDI timeout
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvironment::midiTimeOut(mios32_midi_port_t port)
{
    mbSidAsid.timeOut(port);
    mbSidSysEx.timeOut(port);
}


/////////////////////////////////////////////////////////////////////////////
// Forwards to clock generator
/////////////////////////////////////////////////////////////////////////////

void MbSidEnvironment::bpmSet(float bpm)
{
    mbSidClock.bpmSet(bpm);
}

float MbSidEnvironment::bpmGet(void)
{
    return mbSidClock.bpmGet();
}

void MbSidEnvironment::bpmRestart(void)
{
    mbSidClock.bpmRestart();
}
