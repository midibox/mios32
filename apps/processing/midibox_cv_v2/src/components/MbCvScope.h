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
    void addValue(u32 timestamp, s16 value);

    // Prints the display content on screen (should be called from a low-prio task!)    
    void tick(void);

    // control oversampling
    void setOversamplingFactor(u8 factor);
    u8   getOversamplingFactor(void);

    // control update period
    void setUpdatePeriod(u32 period);
    u32  getUpdatePeriod(void);

    // control trigger level
    void setTriggerLevel(s16 level);
    s16  getTriggerLevel(void);

protected:
    // display number
    u8 displayNum;

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
    bool triggerOnRisingEdge;
    bool displayUpdateReq;
    s16 triggerLevel;
    s16 lastValue;
    u32 lastUpdateTimestamp;
    u32 updatePeriod;

};

#endif /* _MB_CV_SCOPE_H */
