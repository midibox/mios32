/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Clock Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_CLOCK_H
#define _MB_CV_CLOCK_H

#include <mios32.h>
#include "MbCvStructs.h"


typedef enum {
    MBCV_CLOCK_MODE_AUTO,
    MBCV_CLOCK_MODE_MASTER,
    MBCV_CLOCK_MODE_SLAVE
} mbcv_clock_mode_t;


class MbCvClock
{
public:
    // Constructor
    MbCvClock();

    // Destructor
    ~MbCvClock();

    // should be called peridocally by updateSe
    void tick(void);

    // receives MIDI realtime events for external clocking
    void midiReceiveRealTimeEvent(mios32_midi_port_t port, u8 midi_in);

    // set/get BPM
    void bpmSet(float bpm);
    float bpmGet(void);

    // get/set clock mode
    void clockModeSet(mbcv_clock_mode_t mode);
    mbcv_clock_mode_t clockModeGet(void);

    // resets internal Tempo generator
    void bpmRestart(void);

    // current master/slave mode (will be changed depending on clockMode)
    bool clockSlaveMode;

    // event flags (valid for one cycle)
    bool eventClock;
    bool eventStart;
    bool eventContinue;
    bool eventStop;

    // Start/Stop
    bool isRunning;

    u8 updateSpeedFactor;

    u32 clkTickCtr;

    // external clocks
#if CV_EXTCLK_NUM > 8
# error "Please adapt variable type of externalClocks"
#endif
    u8 externalClocks;
    array<u8, CV_EXTCLK_NUM> externalClockDivider;
    array<u8, CV_EXTCLK_NUM> externalClockPulseWidth;
    array<u16, CV_EXTCLK_NUM> externalClockPulseWidthCounter;


protected:
    float effectiveBpm;

    // clock mode Master/Slave/Auto
    mbcv_clock_mode_t clockMode;

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

#endif /* _MB_CV_CLOCK_H */
