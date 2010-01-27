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

#ifndef _MB_SID_CLOCK_H
#define _MB_SID_CLOCK_H

#include <mios32.h>


typedef enum {
    MBSID_CLOCK_MODE_MASTER,
    MBSID_CLOCK_MODE_SLAVE,
    MBSID_CLOCK_MODE_AUTO
} mbsid_clock_mode_t;


class MbSidClock
{
public:
    // Constructor
    MbSidClock();

    // Destructor
    ~MbSidClock();

    // should be called peridocally by updateSe
    void tick(void);

    // receives MIDI realtime events for external clocking
    void midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in);

    // set/get BPM
    void bpmSet(float bpm);
    float bpmGet(void);

    // get/set clock mode
    void clockModeSet(mbsid_clock_mode_t mode);
    mbsid_clock_mode_t clockModeGet(void);

    // resets internal Tempo generator
    void bpmRestart(void);

    // event flags (valid for one cycle)
    bool eventClock;
    bool eventStart;
    bool eventContinue;
    bool eventStop;

    u8 updateSpeedFactor;

    u8 clkCtr6;
    u8 clkCtr24;

protected:
    // clock mode Master/Slave/Auto
    mbsid_clock_mode_t clockMode;

    // current master/slave mode (will be changed depending on clockMode)
    bool slaveMode;

    // MIDI requests
    bool midiStartReq;
    bool midiContinueReq;
    bool midiStopReq;

    // counter stuff
    u16 incomingClkCtr;
    u16 incomingClkDelay;
    u16 sentClkDelay;
    u8 sentClkCtr;
    u8 clkReqCtr;

    u32 bpmCtr;
    u32 bpmCtrMod;
};

#endif /* _MB_SID_CLOCK_H */
