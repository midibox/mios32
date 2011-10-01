/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV LFO
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbCvLfo.h"
#include "MbCvTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvLfo::MbCvLfo()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvLfo::~MbCvLfo()
{
}



/////////////////////////////////////////////////////////////////////////////
// LFO init function
/////////////////////////////////////////////////////////////////////////////
void MbCvLfo::init()
{
    // clear flags
    restartReq = false;
    syncClockReq = false;

    // clear variables
    lfoMode.ALL = 0;
    lfoMode.ENABLE = 1;
    lfoAmplitude = 0;
    lfoRate = 128;
    lfoDelay = 0;
    lfoPhase = 0;
    lfoRateModulation = 0;
    lfoAmplitudeModulation = 0;

    lfoDepthPitch = 127;
    lfoDepthLfoAmplitude = 0;
    lfoDepthLfoRate = 0;
    lfoDepthEnvAmplitude = 0;
    lfoDepthEnvDecay = 0;

    lfoOut = 0;

    lfoCtr = 0;
    lfoDelayCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// LFO handler
/////////////////////////////////////////////////////////////////////////////
bool MbCvLfo::tick(const u8 &updateSpeedFactor)
{
    bool overrun = false; // will be the return value

    // LFO restart requested?
    if( restartReq ) {
        restartReq = false;

        // reset counter (take phase into account)
        lfoCtr = lfoPhase << 8;

        // check if LFO should be delayed - set delay counter to 0x0001 in this case
        lfoDelayCtr = lfoDelay ? 1 : 0;
    }

    // set wave register to initial value and skip LFO if not enabled
    if( !lfoMode.ENABLE ) {
        lfoOut = 0;
    } else {
        if( lfoDelayCtr ) {
            int newDelayCtr = lfoDelayCtr + (mbCvEnvTable[lfoDelay] / updateSpeedFactor);
            if( newDelayCtr > 0xffff )
                lfoDelayCtr = 0; // delay passed
            else
                lfoDelayCtr = newDelayCtr; // delay not passed
        }

        if( !lfoDelayCtr ) { // delay passed?
            bool lfoStalled = false;

            // in oneshot mode: check if counter already reached 0xffff
            if( lfoMode.ONESHOT && lfoCtr >= 0xffff )
                lfoStalled = true;

            // if clock sync enabled: only increment on each 16th clock event
            if( lfoMode.CLKSYNC && !syncClockReq )
                lfoStalled = true;
            syncClockReq = false;

            if( !lfoStalled ) {
                // increment 16bit counter by given rate

                // the rate can be modulated
                s32 rate = lfoRate + (lfoRateModulation / 256);
                if( rate > 255 ) rate = 255; else if( rate < 0 ) rate = 0;

                // if LFO synched via clock, replace 245-255 by MIDI clock optimized incrementers
                u16 inc;
                if( lfoMode.CLKSYNC && rate >= 245 )
                    inc = mbCvLfoTableMclk[rate-245]; // / updateSpeedFactor;
                else
                    inc = mbCvLfoTable[rate] / updateSpeedFactor;

                // add to counter and check for overrun
                s32 newCtr = lfoCtr + inc;
                if( newCtr > 0xffff ) {
                    overrun = true;

                    if( lfoMode.ONESHOT )
                        newCtr = 0xffff; // stop at end position
                }
                lfoCtr = (u16)newCtr;
            }

            // map counter to waveform
            s16 out;
            switch( lfoMode.WAVEFORM ) {
            case 0: { // Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = lfoCtr >> 7;
                if( lfoCtr & (1 << 14) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                out = mbCvSinTable[ptr];
                if( lfoCtr & (1 << 15) )
                    out = -out;
            } break;  

            case 1: { // Triangle
                // similar to sine, but linear waveform
                out = (lfoCtr & 0x3fff) << 1;
                if( lfoCtr & (1 << 14) )
                    out = 0x7fff - out;
                if( lfoCtr & (1 << 15) )
                    out = -out;
            } break;  

            case 2: { // Saw
                out = lfoCtr - 0x8000;
            } break;  

            case 3: { // Pulse
                out = (lfoCtr < 0x8000) ? -0x8000 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            case 4: { // Random
                // only on LFO overrun
                if( overrun )
                    out = randomGen.value(0x0000, 0xffff);
            } break;  

            case 5: { // Positive Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = lfoCtr >> 8;
                if( lfoCtr & (1 << 15) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                out = mbCvSinTable[ptr];
            } break;  

            case 6: { // Positive Triangle
                // similar to sine, but linear waveform
                out = (lfoCtr & 0x7fff);
                if( lfoCtr & (1 << 15) )
                    out = 0x7fff - out;
            } break;  

            case 7: { // Positive Saw
                out = lfoCtr >> 1;
            } break;  

            case 8: { // Positive Pulse
                out = (lfoCtr < 0x8000) ? 0 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            default: // take saw as default
                out = lfoCtr - 0x8000;
            }

            // the amplitude can be modulated
            s32 amplitude = lfoAmplitude + (lfoAmplitudeModulation / 512);
            if( amplitude > 127 ) amplitude = 127; else if( amplitude < -128 ) amplitude = -128;

            // final output value
            lfoOut = ((s32)out * (s32)amplitude) / 128;
        }
    }

    return overrun;
}

