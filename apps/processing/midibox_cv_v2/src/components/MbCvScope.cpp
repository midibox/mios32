/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Scope Display
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvScope.h"
#include <string.h>

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvScope::MbCvScope()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvScope::~MbCvScope()
{
}

/////////////////////////////////////////////////////////////////////////////
// initializes the scope
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::init(u8 _displayNum)
{
    displayNum = _displayNum;

    oversamplingFactor = 8;
    updatePeriod = 5000;
    triggerLevel = 0;
    triggerOnRisingEdge = true;
    clear();
}


/////////////////////////////////////////////////////////////////////////////
// clears the scope display
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::clear(void)
{
    displayBufferHead = 0;
    lastUpdateTimestamp = 0;
    lastValue = 0;
    displayUpdateReq = true;

    oversamplingCounter = 0;
    oversamplingValue = 0;

    u8 *bufferPtr = (u8 *)&displayBuffer[0];
    for(int i=0; i<MBCV_SCOPE_DISPLAY_BUFFER_SIZE; ++i)
        *(bufferPtr++) = 0x40; // mid value
}

/////////////////////////////////////////////////////////////////////////////
// Adds new value to the display
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::addValue(u32 timestamp, s16 value)
{
    bool ongoingCapture = displayBufferHead < MBCV_SCOPE_DISPLAY_BUFFER_SIZE;

    if( !ongoingCapture ) {
        bool reset = false;

        if( (timestamp - lastUpdateTimestamp) >= updatePeriod ) { // timeout
            reset = true;
        } else if( ( triggerOnRisingEdge && (lastValue < triggerLevel) && (value >= triggerLevel)) ||
                   (!triggerOnRisingEdge && (lastValue > triggerLevel) && (value <= triggerLevel)) ) {
            reset = true;
        }

        if( reset ) {
            lastUpdateTimestamp = timestamp;
            displayBufferHead = 0;
            ongoingCapture = 1;
        }
    }

    lastValue = value;

    if( ongoingCapture ) {
        oversamplingValue += (s32)value;

        if( ++oversamplingCounter >= oversamplingFactor ) {
            u16 displayHeight = 48;

            s32 value = (((oversamplingValue / oversamplingFactor) + 0x8000) * displayHeight) / 65535;
            if( value >= displayHeight )
                value = displayHeight-1;
            else if( value < 0 )
                value = 0;

            displayBuffer[displayBufferHead] = displayHeight - 1 - value;

            oversamplingValue = 0;
            oversamplingCounter = 0;

            if( ++displayBufferHead >= MBCV_SCOPE_DISPLAY_BUFFER_SIZE ) {
                displayUpdateReq = true;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Prints the display content on screen (should be called from a low-prio task!)
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::tick(void)
{
    bool ongoingCapture = displayBufferHead < MBCV_SCOPE_DISPLAY_BUFFER_SIZE;

    if( displayUpdateReq || ongoingCapture ) {
        displayUpdateReq = false;

        // change to scope display
        s32 prevDevice = MIOS32_LCD_DeviceGet();
        MIOS32_LCD_DeviceSet(displayNum);

        u16 displayHeight = 48;
        u16 displayWidth = 128;

        // create snapshot of buffer to ensure a consistent screen
        u8 tmpBuffer[MBCV_SCOPE_DISPLAY_BUFFER_SIZE];
        memcpy(tmpBuffer, displayBuffer, MBCV_SCOPE_DISPLAY_BUFFER_SIZE);

        for(int y=0; y<displayHeight; y+=8) {
            MIOS32_LCD_GCursorSet(0, y);

            u8 *bufferPtr = tmpBuffer;
            for(int x=0; x<displayWidth; ++x) {
                u8 value = *(bufferPtr++);

                if( value >= y && value <= (y+7) ) {
                    MIOS32_LCD_Data(1 << (value % 8));
                } else {
                    MIOS32_LCD_Data(0x00);
                }
            }
        }

        // change back to original display
        MIOS32_LCD_DeviceSet(prevDevice);
    }
}


/////////////////////////////////////////////////////////////////////////////
// control oversampling
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::setOversamplingFactor(u8 factor)
{
    if( factor < 1 )
        oversamplingFactor = 1;
    else
        oversamplingFactor = factor;
}

u8 MbCvScope::getOversamplingFactor(void)
{
    return oversamplingFactor;
}

/////////////////////////////////////////////////////////////////////////////
// control update period
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::setUpdatePeriod(u32 period)
{
    updatePeriod = period;
}

u32 MbCvScope::getUpdatePeriod(void)
{
    return updatePeriod;
}

/////////////////////////////////////////////////////////////////////////////
// control trigger level
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::setTriggerLevel(s16 level)
{
    triggerLevel = level;
}

s16 MbCvScope::getTriggerLevel(void)
{
    return triggerLevel;
}

