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
#include <glcd_font.h>
#include <tasks.h>
#include <app.h>

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
    updatePeriod = 3000; // 3 seconds
    setTrigger(10); // P10%
    setSource(_displayNum + 1); // CV1..4
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
    lastGate = 0;
    lastClkTickCtr = 0;
    displayUpdateReq = true;

    oversamplingCounter = 0;
    oversamplingValue = 0;

    capturedMinValue = minValue = MBCV_SCOPE_DISPLAY_MIN_RESET_VALUE;
    capturedMaxValue = maxValue = MBCV_SCOPE_DISPLAY_MAX_RESET_VALUE;

    u8 *bufferPtr = (u8 *)&displayBuffer[0];
    for(int i=0; i<MBCV_SCOPE_DISPLAY_BUFFER_SIZE; ++i)
        *(bufferPtr++) = 0x40; // mid value
}

/////////////////////////////////////////////////////////////////////////////
// Adds new value to the display
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::addValue(s16 value, u8 gate, u32 clkTickCtr)
{
    bool ongoingCapture = displayBufferHead < MBCV_SCOPE_DISPLAY_BUFFER_SIZE;

    if( ongoingCapture ) {
        if( value > maxValue )
            maxValue = value;
        if( value < minValue )
            minValue = value;
    }

    if( !ongoingCapture ) {
        bool reset = false;
        u32 timestamp = MIOS32_TIMESTAMP_Get();

        if( (timestamp - lastUpdateTimestamp) >= updatePeriod ) { // timeout
            reset = true;
        } else {
            if( trigger <= 100 ) {
                s16 triggerLevel = (((s32)trigger - 50) * 65535) / 100;
                reset = (lastValue < triggerLevel) && (value >= triggerLevel);
            } else if( trigger <= 201 ) {
                s16 triggerLevel = (((s32)trigger - 50-101) * 65535) / 100;
                reset = (lastValue > triggerLevel) && (value <= triggerLevel);
            } else if( trigger == 202 ) { // PGate
                reset = !lastGate && gate;
            } else if( trigger == 203 ) { // NGate
                reset = lastGate && !gate;
            } else if( trigger == 204 ) { // 64th
                reset = lastClkTickCtr != clkTickCtr && (clkTickCtr % (6*1)) == 0;
            } else if( trigger == 205 ) { // 32th
                reset = lastClkTickCtr != clkTickCtr && (clkTickCtr % (6*2)) == 0;
            } else if( trigger == 206 ) { // 16th
                reset = lastClkTickCtr != clkTickCtr && (clkTickCtr % (6*4)) == 0;
            } else if( trigger == 207 ) { // 8th
                reset = lastClkTickCtr != clkTickCtr && (clkTickCtr % (6*8)) == 0;
            } else if( trigger <= 215 ) { // 208..215 = 1beat..8beat
                u32 beats = trigger - 208 + 1;
                u32 ticks = 4*6*4*beats;
                reset = lastClkTickCtr != clkTickCtr && (clkTickCtr % ticks) == 0;
            } else {
                // no reset
            }
        }

        if( reset ) {
            lastUpdateTimestamp = timestamp;
            displayBufferHead = 0;
            ongoingCapture = 1;

            minValue = MBCV_SCOPE_DISPLAY_MIN_RESET_VALUE;
            maxValue = MBCV_SCOPE_DISPLAY_MAX_RESET_VALUE;
        }
    }

    lastValue = value;
    lastGate = gate;
    lastClkTickCtr = clkTickCtr;

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

        capturedMinValue = minValue;
        capturedMaxValue = maxValue;
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

        u16 displayHeight = 48;
        u16 displayWidth = 128;

        // create snapshot of buffer to ensure a consistent screen
        u8 tmpBuffer[MBCV_SCOPE_DISPLAY_BUFFER_SIZE];
        memcpy(tmpBuffer, displayBuffer, MBCV_SCOPE_DISPLAY_BUFFER_SIZE);

        mios32_lcd_bitmap_t bitmap;
        bitmap.height = 8;
        bitmap.width = 128;
        bitmap.line_offset = 0;
        bitmap.colour_depth = 1;
        u8 bitmapMemory[128];
        bitmap.memory = (u8 *)&bitmapMemory[0];

        for(int y=0; y<displayHeight; y+=8) {
            u8 *bufferPtr = tmpBuffer;
            u8 *bitmapPtr = bitmap.memory;
            for(int x=0; x<displayWidth; ++x) {
                u8 value = *(bufferPtr++);

                if( value >= y && value <= (y+7) ) {
                    *(bitmapPtr++) = 1 << (value % 8);
                } else {
                    *(bitmapPtr++) = 0x00;
                }
            }

            {
                MUTEX_LCD_TAKE;
                APP_SelectScopeLCDs();

                MIOS32_LCD_DeviceSet(displayNum);
                MIOS32_LCD_GCursorSet(0, y);
                MIOS32_LCD_BitmapPrint(bitmap);

                APP_SelectMainLCD();
                MUTEX_LCD_GIVE;
            }
        }

        {
            MUTEX_LCD_TAKE;
            APP_SelectScopeLCDs();

            MIOS32_LCD_DeviceSet(displayNum);
            MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);    
            MIOS32_LCD_CursorSet(0, 7);
            if( source > 0 ) {
                MIOS32_LCD_PrintFormattedString("CV%d ", source);
            } else {
                MIOS32_LCD_PrintFormattedString("--- ");
            }

            if( capturedMinValue == MBCV_SCOPE_DISPLAY_MIN_RESET_VALUE ) {
                MIOS32_LCD_PrintFormattedString("Min:???%%");
            } else {
                u32 percent = ((capturedMinValue + 0x8000) * 100) / 65535;
                MIOS32_LCD_PrintFormattedString("Min:%3d%%", percent);
            }

            if( capturedMaxValue == MBCV_SCOPE_DISPLAY_MAX_RESET_VALUE ) {
                MIOS32_LCD_PrintFormattedString(" Max:???%%");
            } else {
                u32 percent = ((capturedMaxValue + 0x8000) * 100) / 65535;
                MIOS32_LCD_PrintFormattedString(" Max:%3d%%", percent);
            }

            APP_SelectMainLCD();
            MUTEX_LCD_GIVE;
        }
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

    clear();
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
// control trigger
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::setTrigger(u8 _trigger)
{
    if( _trigger < MBCV_SCOPE_NUM_TRIGGERS )
        trigger = _trigger;
}

u8 MbCvScope::getTrigger(void)
{
    return trigger;
}

/////////////////////////////////////////////////////////////////////////////
// Source Assignment
/////////////////////////////////////////////////////////////////////////////
void MbCvScope::setSource(u8 _source)
{
    if( source < MBCV_SCOPE_NUM_SOURCES ) {
        clear();
        source = _source;
    }
}

u8 MbCvScope::getSource(void)
{
    return source;
}



