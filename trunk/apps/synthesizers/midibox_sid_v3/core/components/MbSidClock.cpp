/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Clock Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidClock.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidClock::MbSidClock()
{
    updateSpeedFactor = 2;

    // set default clock mode
    clockModeSet(MBSID_CLOCK_MODE_AUTO);

    // set default BPM rate
    bpmCtr = 0;
    bpmSet(120.0);

    clockSlaveMode = false;

    midiStartReq = false;
    midiContinueReq = false;
    midiStopReq = false;

    clkCtr6 = 0;
    clkCtr24 = 0;

    incomingClkCtr = 0;
    incomingClkDelay = 0;
    sentClkDelay = 0;
    sentClkCtr = 0;
    clkReqCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidClock::~MbSidClock()
{
}


/////////////////////////////////////////////////////////////////////////////
// should be called peridocally by updateSe
/////////////////////////////////////////////////////////////////////////////
void MbSidClock::tick(void)
{
    // clear previous clock events
    eventClock = false;
    eventStart = false;
    eventStop = false;

    // increment the clock counter, used to measure the delay between two F8 events
    // see also SID_SE_IncomingClk()
    if( incomingClkCtr != 0xffff )
        ++incomingClkCtr;

    // now determine master/slave flag depending on ensemble setup
    switch( clockMode ) {
    case MBSID_CLOCK_MODE_MASTER:
        clockSlaveMode = false;
        break;

    case MBSID_CLOCK_MODE_SLAVE:
        clockSlaveMode = true;
        break;

    default: // MBSID_CLOCK_MODE_AUTO
        // slave mode so long incoming clock counter < 0xfff
        clockSlaveMode = incomingClkCtr < 0xfff;
        break;
    }

    // only used in slave mode:
    // decrement sent clock delay, send interpolated clock events 3 times
    if( !--sentClkDelay && sentClkCtr < 3 ) {
        ++sentClkCtr;
        ++clkReqCtr;
        sentClkDelay = incomingClkDelay >> 2; // to generate clock 4 times
    }

    // handle MIDI Start Event
    if( midiStartReq ) {
        // take over
        midiStartReq = false;
        eventStart = true;

        // reset counters
        clkCtr6 = 0;
        clkCtr24 = 0;
    }

    // handle MIDI Stop Event
    if( midiStopReq ) {
        // take over
        midiStopReq = false;
        eventStop = true;
    }

    // handle MIDI continue event
    if( midiContinueReq ) {
        // take over
        midiContinueReq = false;
        eventContinue = true;
    }

    // handle MIDI Clock Event
    if( clkReqCtr ) {
        // decrement counter by one (if there are more requests, they will be handled on next handler invocation)
        --clkReqCtr;

        if( clockSlaveMode )
            eventClock = 1;
    } else {
        if( !clockSlaveMode ) {
            // TODO: check timer overrun flag
            bpmCtr += 1000000;
            if( bpmCtr > bpmCtrMod ) {
                bpmCtr -= bpmCtrMod;
                eventClock = 1;
            }
        }
    }

    if( eventClock ) {
        // increment clock counter (for Clk/6 and Clk/24 divider)
        if( ++clkCtr6 >= 6 ) {
            clkCtr6 = 0;
            if( ++clkCtr24 >= 4 )
                clkCtr24 = 0;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// receives MIDI realtime events for external clocking
/////////////////////////////////////////////////////////////////////////////
void MbSidClock::midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in)
{
    // TMP:
    // we filter out USB2..15 to ensure that USB interface won't clock multiple times
    if( (port & 0xf0) == USB0 && (port & 0x0f) != 0 )
        return;

    // ensure that following operations are atomic
    MIOS32_IRQ_Disable();

    switch( midi_in ) {
    case 0xf8: // MIDI Clock
        // we've measure a new delay between two F8 events
        incomingClkDelay = incomingClkCtr;
        incomingClkCtr = 0;

        // increment clock counter by 4
        clkReqCtr += 4 - sentClkCtr;
        sentClkCtr = 0;

        // determine new clock delay
        sentClkDelay = incomingClkDelay >> 2;
        break;

    case 0xfa: // MIDI Clock Start
        midiStartReq = true;

        // Auto Mode: immediately switch to slave mode
        // TODO

        // ensure that incoming clock counter < 0xfff (means: no clock received for long time)
        if( incomingClkCtr > 0xfff )
            incomingClkCtr = 0;

        // cancel all requested clocks
        clkReqCtr = 0;
        sentClkCtr = 3;
        break;

    case 0xfb: // MIDI Clock Continue
        midiContinueReq = true;
        break;

    case 0xfc: // MIDI Clock Stop
        midiStopReq = true;
        break;
    }

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// set/get BPM
/////////////////////////////////////////////////////////////////////////////
void MbSidClock::bpmSet(float bpm)
{
    // ensure that BPM doesn't lead to integer overflow
    if( bpm < 1 )
        bpm = 120;

    // how many SID SE ticks for next 96ppqn event?
    // result is fixed point arithmetic * 10000000 (for higher accuracy)
    bpmCtrMod = (u32)(((60.0/bpm) / (2E-3 / (float)updateSpeedFactor)) * (1000000.0/96.0));
}

float MbSidClock::bpmGet(void)
{
    return 0.0;
}


/////////////////////////////////////////////////////////////////////////////
// resets internal Tempo generator
/////////////////////////////////////////////////////////////////////////////
void MbSidClock::bpmRestart(void)
{
    midiStartReq = true;
}


/////////////////////////////////////////////////////////////////////////////
// set/get clock mode
/////////////////////////////////////////////////////////////////////////////
void MbSidClock::clockModeSet(mbsid_clock_mode_t mode)
{
    clockMode = mode;
}

mbsid_clock_mode_t MbSidClock::clockModeGet(void)
{
    return clockMode;
}

