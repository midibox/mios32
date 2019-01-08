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
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define SID_BANK_NUM 1  // currently only a single bank is available in ROM
#include "sid_bank_preset_a.inc"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment::MbSidEnvironment()
{
    mbSidSysEx.mbSidEnvironmentPtr = this;

    // initialize structures of each SID engine
    MbSid *s = mbSid.first();
    for(int sid=0; sid < mbSid.size; ++sid, ++s) {
        sid_regs_t *sidRegLPtr = &sid_regs[2*sid+0];
        sid_regs_t *sidRegRPtr = &sid_regs[2*sid+1];

        s->init(sid, sidRegLPtr, sidRegRPtr, &mbSidClock);
    }

    // sets the default speed factor
    updateSpeedFactorSet(2);

    // restart clock generator
    bpmRestart();
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
    mbSidClock.updateSpeedFactor = _updateSpeedFactor;
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engines Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvironment::tick(void)
{
    bool updateRequired = false;

    if( mbSidAsid.modeGet() != MBSID_ASID_MODE_OFF )
        return false;

    // Tempo Clock
    mbSidClock.tick();

    // Engines
    for(MbSid *s = mbSid.first(); s != NULL ; s=mbSid.next(s)) {
        if( s->tick(updateSpeedFactor) )
            updateRequired = true;
    }

    return updateRequired;
}


/////////////////////////////////////////////////////////////////////////////
// Write a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbSidEnvironment::bankSave(u8 sid, u8 bank, u8 patch)
{
    if( sid >= mbSid.size )
        return -1; // SID not available

    return -2; // not supported yet
}

/////////////////////////////////////////////////////////////////////////////
// Read a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbSidEnvironment::bankLoad(u8 sid, u8 bank, u8 patch)
{
    if( sid >= mbSid.size )
        return -1; // SID not available

    MbSid *s = &mbSid[sid];

    if( bank >= SID_BANK_NUM )
        return -2; // invalid bank

    if( patch >= 128 )
        return -3; // invalid patch

#if 0
    DEBUG_MSG("[SID_BANK_PatchRead] SID %d reads patch %c%03d\n", sid, 'A'+bank, patch);
#endif

    switch( bank ) {
    case 0: {
        sid_patch_t *bankPatch = (sid_patch_t *)sid_bank_preset_0[patch];
        s->mbSidPatch.copyToPatch(bankPatch);
    } break;

    default:
        return -4; // no bank in ROM
    }

    s->updatePatch(false);

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns the name of a preset patch as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
s32 MbSidEnvironment::bankPatchNameGet(u8 bank, u8 patch, char *buffer)
{
    int i;
    sid_patch_t *p;

    if( bank >= SID_BANK_NUM ) {
        sprintf(buffer, "<Invalid Bank %c>", 'A'+bank);
        return -1; // invalid bank
    }

    if( patch >= 128 ) {
        sprintf(buffer, "<WrongPatch %03d>", patch);
        return -2; // invalid patch
  }

    switch( bank ) {
    case 0:
        p = (sid_patch_t *)&sid_bank_preset_0[patch];
        break;

    default:
        sprintf(buffer, "<Empty Bank %c>  ", 'A'+bank);
        return -3; // no bank in ROM
    }

    for(i=0; i<16; ++i)
        buffer[i] = p->name[i] >= 0x20 ? p->name[i] : ' ';
    buffer[i] = 0;

    return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Forwards incoming MIDI events to all MBSIDs
/////////////////////////////////////////////////////////////////////////////
void MbSidEnvironment::midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    if( midi_package.event == ProgramChange ) {
        // TODO: check port and channel for program change
        int sid = 0;
        for(MbSid *s = mbSid.first(); s != NULL ; s=mbSid.next(s), ++sid)
            bankLoad(sid, s->mbSidPatch.bankNum, midi_package.evnt1);
    } else {
        for(MbSid *s = mbSid.first(); s != NULL ; s=mbSid.next(s))
            s->midiReceive(midi_package);
    }
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


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to take over a received patch
// returns false if SID not available
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvironment::sysexSetPatch(u8 sid, sid_patch_t *p, bool toBank, u8 bank, u8 patch)
{
    if( sid >= mbSid.size )
        return false;

    if( toBank ) {
        bankSave(sid, bank, patch);
    } else {
        // forward to selected SID
        return mbSid[sid].sysexSetPatch(p);
    }
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to return the current patch
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvironment::sysexGetPatch(u8 sid, sid_patch_t *p, bool fromBank, u8 bank, u8 patch)
{
    if( sid >= mbSid.size )
        return false;

    if( fromBank )
        bankLoad(sid, bank, patch);

    return mbSid[sid].sysexGetPatch(p);
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbSidSysEx to set a dedicated SysEx parameter
// returns false if selected SID or parameter not available
/////////////////////////////////////////////////////////////////////////////
bool MbSidEnvironment::sysexSetParameter(u8 sid, u16 addr, u8 data)
{
    if( sid >= mbSid.size )
        return false;

    // forward to selected SID (no [sid] range checking required, this is done by array template)
    return mbSid[sid].sysexSetParameter(addr, data);
}
