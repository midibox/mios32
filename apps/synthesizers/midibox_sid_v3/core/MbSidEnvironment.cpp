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
// this function is called from app.c to instantiate the environment
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment *mbSidEnvironment;

extern "C" s32 MbSidEnvironment_Init(u32 mode)
{
    return (mbSidEnvironment = new MbSidEnvironment()) ? 0 : -1; // initialisation failed
}


/////////////////////////////////////////////////////////////////////////////
// sets the update cycle factor
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MbSidEnvironment_updateSpeedFactorSet(u32 factor)
{
    if( mbSidEnvironment ) {
        mbSidEnvironment->updateSpeedFactorSet(factor);
        return 0;
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////////////
// called from NOTIFY_MIDI_Rx() in app.c
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MbSidEnvironment_IncomingRealTimeEvent(u8 event)
{
    if( mbSidEnvironment ) {
        mbSidEnvironment->incomingRealTimeEvent(event);
        return 0; // no error
    }

    return -1; // object not available
}

/////////////////////////////////////////////////////////////////////////////
// called from SID_TIMER_SE_Update() in app.c
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MbSidEnvironment_UpdateSe(void)
{
    if( mbSidEnvironment ) {
        mbSidEnvironment->updateSe();
        return 0; // no error
    }

    return -1; // object not available
}

/////////////////////////////////////////////////////////////////////////////
// called from APP_MIDI_NotifyPackage() in app.c
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 MbSidEnvironment_midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    if( mbSidEnvironment ) {
        mbSidEnvironment->midiReceive(port, midi_package);
        return 0; // no error
    }

    return -1; // object not available
}


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment::MbSidEnvironment()
{
    // ensure that clock generator structure is cleared
    memset(&mbSidClk, 0, sizeof(sid_se_clk_t));
    bpmSet(120.0);

    // initialize structures of each SID engine
    for(int sid=0; sid<SID_SE_NUM; ++sid) {
        mbSid[sid].sidNum = sid;
        mbSid[sid].physSidL = 2*sid+0;
        mbSid[sid].sidRegLPtr = &sid_regs[2*sid+0];
        mbSid[sid].physSidR = 2*sid+1;
        mbSid[sid].sidRegRPtr = &sid_regs[2*sid+1];
        mbSid[sid].mbSidClkPtr = &mbSidClk;

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
void MbSidEnvironment::updateSe(void)
{
    // Clock
    clock();

    // Engines
    for(int sid=0; sid<SID_SE_NUM; ++sid)
        mbSid[sid].updateSe();
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
// Clock
/////////////////////////////////////////////////////////////////////////////

void MbSidEnvironment::incomingRealTimeEvent(u8 event)
{
    // ensure that following operations are atomic
    MIOS32_IRQ_Disable();

    switch( event ) {
    case 0xf8: // MIDI Clock
        // we've measure a new delay between two F8 events
        mbSidClk.incoming_clk_delay = mbSidClk.incoming_clk_ctr;
        mbSidClk.incoming_clk_ctr = 0;

        // increment clock counter by 4
        mbSidClk.clk_req_ctr += 4 - mbSidClk.sent_clk_ctr;
        mbSidClk.sent_clk_ctr = 0;

        // determine new clock delay
        mbSidClk.sent_clk_delay = mbSidClk.incoming_clk_delay >> 2;
        break;

    case 0xfa: // MIDI Clock Start
        mbSidClk.event.MIDI_START_REQ = 1;

        // Auto Mode: immediately switch to slave mode
        // TODO

        // ensure that incoming clock counter < 0xfff (means: no clock received for long time)
        if( mbSidClk.incoming_clk_ctr > 0xfff )
            mbSidClk.incoming_clk_ctr = 0;

        // cancel all requested clocks
        mbSidClk.clk_req_ctr = 0;
        mbSidClk.sent_clk_ctr = 3;
        break;

    case 0xfb: // MIDI Clock Continue
        mbSidClk.event.MIDI_CONTINUE_REQ = 1;
        break;

    case 0xfc: // MIDI Clock Stop
        mbSidClk.event.MIDI_STOP_REQ = 1;
        break;
    }

    MIOS32_IRQ_Enable();
}


// temporary
void MbSidEnvironment::bpmSet(float bpm)
{
    // ensure that BPM doesn't lead to integer overflow
    if( bpm < 1 )
        bpm = 120;

    // how many SID SE ticks for next 96ppqn event?
    // result is fixed point arithmetic * 10000000 (for higher accuracy)
    mbSidClk.tmp_bpm_ctr_mod = (u32)(((60.0/bpm) / (2E-3 / (float)updateSpeedFactor)) * (1000000.0/96.0));
}

// temporary
void MbSidEnvironment::bpmRestart(void)
{
    mbSidClk.event.MIDI_START_REQ = 1;
}

void MbSidEnvironment::clock(void)
{
    // clear previous clock events
    mbSidClk.event.ALL_SE = 0;

    // increment the clock counter, used to measure the delay between two F8 events
    // see also SID_SE_IncomingClk()
    if( mbSidClk.incoming_clk_ctr != 0xffff )
        ++mbSidClk.incoming_clk_ctr;

    // now determine master/slave flag depending on ensemble setup
    u8 clk_mode = 2; // TODO: selection via ensemble
    switch( clk_mode ) {
    case 0: // master
        mbSidClk.state.SLAVE = 0;
        break;

    case 1: // slave
        mbSidClk.state.SLAVE = 1;
        break;

    default: // auto
        // slave mode so long incoming clock counter < 0xfff
        mbSidClk.state.SLAVE = mbSidClk.incoming_clk_ctr < 0xfff;
        break;
    }

    // only used in slave mode:
    // decrement sent clock delay, send interpolated clock events 3 times
    if( !--mbSidClk.sent_clk_delay && mbSidClk.sent_clk_ctr < 3 ) {
        ++mbSidClk.sent_clk_ctr;
        ++mbSidClk.clk_req_ctr;
        mbSidClk.sent_clk_delay = mbSidClk.incoming_clk_delay >> 2; // to generate clock 4 times
    }

    // handle MIDI Start Event
    if( mbSidClk.event.MIDI_START_REQ ) {
        // take over
        mbSidClk.event.MIDI_START_REQ = 0;
        mbSidClk.event.MIDI_START = 1;

        // reset counters
        mbSidClk.global_clk_ctr6 = 0;
        mbSidClk.global_clk_ctr24 = 0;
    }


    // handle MIDI Clock Event
    if( mbSidClk.clk_req_ctr ) {
        // decrement counter by one (if there are more requests, they will be handled on next handler invocation)
        --mbSidClk.clk_req_ctr;

        if( mbSidClk.state.SLAVE )
            mbSidClk.event.CLK = 1;
    } else {
        if( !mbSidClk.state.SLAVE ) {
            // TODO: check timer overrun flag
            // temporary solution: use BPM counter
            mbSidClk.tmp_bpm_ctr += 1000000;
            if( mbSidClk.tmp_bpm_ctr > mbSidClk.tmp_bpm_ctr_mod ) {
                mbSidClk.tmp_bpm_ctr -= mbSidClk.tmp_bpm_ctr_mod;
                mbSidClk.event.CLK = 1;
            }
        }
    }
    if( mbSidClk.event.CLK ) {
        // increment global clock counter (for Clk/6 and Clk/24 divider)
        if( ++mbSidClk.global_clk_ctr6 >= 6 ) {
            mbSidClk.global_clk_ctr6 = 0;
            if( ++mbSidClk.global_clk_ctr24 >= 4 )
                mbSidClk.global_clk_ctr24 = 0;
        }
    }
}
