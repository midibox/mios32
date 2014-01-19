/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Scope Display
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_SCOPE_H
#define _MB_CV_SCOPE_H

#include <mios32.h>
#include "MbCv.h"

#define MBCV_SCOPE_DISPLAY_BUFFER_SIZE 128
#define MBCV_SCOPE_DISPLAY_MIN_RESET_VALUE    32767
#define MBCV_SCOPE_DISPLAY_MAX_RESET_VALUE   -32768

// --- and CV1..8
#define MBCV_SCOPE_NUM_SOURCES (1+CV_SE_NUM)

// 0..100% rising edge, 0..100% falling edge, PGate, NGate, 64th, 32th, 16th, 8th, 1beat..8beat
#define MBCV_SCOPE_NUM_TRIGGERS (101+101+14)

class MbCvScope
{
public:
    // Constructor
    MbCvScope();

    // Destructor
    ~MbCvScope();

    // initializes the scope
    void init(u8 _displayNum);

    // Clears the scope display
    void clear(void);

    // Adds new value to the display
    void addValue(s16 value, u8 gate, u32 clkTickCtr);

    // Prints the display content on screen (should be called from a low-prio task!)    
    void tick(void);

    // Source Channel
    void setSource(u8 source);
    u8   getSource(void);

    // control trigger level (and other functions)
    void setTrigger(u8 trigger);
    u8   getTrigger(void);

    // control oversampling
    void setOversamplingFactor(u8 factor);
    u8   getOversamplingFactor(void);

    // control update period
    void setUpdatePeriod(u32 period);
    u32  getUpdatePeriod(void);

    // display mapping
    void setShowOnMainScreen(bool showOnMainScreen);
    bool getShowOnMainScreen(void);

protected:
    // display number
    u8 displayNum;

    // shows on main screen or alt screen?
    bool showOnMainScreen;

    // the display buffer
    u8 displayBuffer[MBCV_SCOPE_DISPLAY_BUFFER_SIZE];

    // FIFO pointer
    u8 displayBufferHead;
#if MBCV_SCOPE_DISPLAY_BUFFER_SIZE > 256
# error "FIFO pointers only prepared for up to 256 byte; please change variable types"
#endif

    // oversampling
    s32 oversamplingValue;
    u8 oversamplingFactor;
    u8 oversamplingCounter;

    // trigger
    bool displayUpdateReq;
    u8 trigger;
    s16 lastValue;
    u8 lastGate;
    u32 lastClkTickCtr;
    u32 lastUpdateTimestamp;
    u32 updatePeriod;

    // source assignment (currently only CV channels)
    u8 source;

    // statistics
    s16 minValue;
    s16 capturedMinValue;
    s16 maxValue;
    s16 capturedMaxValue;
    
};

#endif /* _MB_CV_SCOPE_H */
