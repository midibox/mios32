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

typedef union {
    u8 ALL;
    struct {
        u8 SLAVE:1;
    };
} mbsid_clock_state_t;

typedef union {
    u8 ALL;

    struct {
        u8 CLK:1;
        u8 MIDI_START:1;
        u8 MIDI_CONTINUE:1;
        u8 MIDI_STOP:1;

        u8 MIDI_START_REQ:1;
        u8 MIDI_CONTINUE_REQ:1;
        u8 MIDI_STOP_REQ:1;
    };

    struct {
        u8 ALL_SE:4;
        u8 ALL_MIDI:3;
    };
} mbsid_clock_event_t;


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


    mbsid_clock_state_t state;
    mbsid_clock_event_t event;

    u8 updateSpeedFactor;

    u8 clkCtr6;
    u8 clkCtr24;

protected:
    mbsid_clock_mode_t clockMode;

    u16 incomingClkCtr;
    u16 incomingClkDelay;
    u16 sentClkDelay;
    u8 sentClkCtr;
    u8 clkReqCtr;

    u32 bpmCtr;
    u32 bpmCtrMod;
};

#endif /* _MB_SID_CLOCK_H */
