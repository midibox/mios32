/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID LFO
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
#include "MbSidLfo.h"
#include "MbSidTables.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidLfo::MbSidLfo()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidLfo::~MbSidLfo()
{
}



/////////////////////////////////////////////////////////////////////////////
// LFO init function
/////////////////////////////////////////////////////////////////////////////
void MbSidLfo::init()
{
    // clear flags
    restartReq = false;
    syncClockReq = false;

    // clear variables
    lfoMode.ALL = 0;
    lfoDepth = 0;
    lfoRate = 0;
    lfoDelay = 0;
    lfoPhase = 0;
    lfoRateModulation = 0;

    lfoDepthPitch = 0;
    lfoDepthPulsewidth = 0;
    lfoDepthFilter = 0;
    lfoKeySync = 0;

    lfoOut = 0;

    lfoCtr = 0;
    lfoDelayCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// LFO handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidLfo::tick(const u8 &updateSpeedFactor)
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
            int newDelayCtr = lfoDelayCtr + (mbSidEnvTable[lfoDelay] / updateSpeedFactor);
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
                    inc = mbSidLfoTableMclk[rate-245]; // / updateSpeedFactor;
                else
                    inc = mbSidLfoTable[rate] / updateSpeedFactor;

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
            switch( lfoMode.WAVEFORM ) {
            case 0: { // Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = lfoCtr >> 7;
                if( lfoCtr & (1 << 14) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                lfoOut = mbSidSinTable[ptr];
                if( lfoCtr & (1 << 15) )
                    lfoOut = -lfoOut;
            } break;  

            case 1: { // Triangle
                // similar to sine, but linear waveform
                lfoOut = (lfoCtr & 0x3fff) << 1;
                if( lfoCtr & (1 << 14) )
                    lfoOut = 0x7fff - lfoOut;
                if( lfoCtr & (1 << 15) )
                    lfoOut = -lfoOut;
            } break;  

            case 2: { // Saw
                lfoOut = lfoCtr - 0x8000;
            } break;  

            case 3: { // Pulse
                lfoOut = (lfoCtr < 0x8000) ? -0x8000 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            case 4: { // Random
                // only on LFO overrun
                if( overrun )
                    lfoOut = randomGen.value(0x0000, 0xffff);
            } break;  

            case 5: { // Positive Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = lfoCtr >> 8;
                if( lfoCtr & (1 << 15) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                lfoOut = mbSidSinTable[ptr];
            } break;  

            case 6: { // Positive Triangle
                // similar to sine, but linear waveform
                lfoOut = (lfoCtr & 0x7fff);
                if( lfoCtr & (1 << 15) )
                    lfoOut = 0x7fff - lfoOut;
            } break;  

            case 7: { // Positive Saw
                lfoOut = lfoCtr >> 1;
            } break;  

            case 8: { // Positive Pulse
                lfoOut = (lfoCtr < 0x8000) ? 0 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            default: // take saw as default
                lfoOut = lfoCtr - 0x8000;
            }

#if 0
            if( !lfo_waveform_skipped ) {
                if( engine == SID_SE_LEAD ) {
                    // scale to LFO depth
                    // the depth can be modulated
                    s32 lfoDepth;
                    if( modDstLfoDepth ) {
                        lfoDepth = ((s32)lfoPatch->depth - 0x80) + (*modDstLfoDepth / 256);
                        if( lfoDepth > 127 ) lfoDepth = 127; else if( lfoDepth < -128 ) lfoDepth = -128;
                    } else {
                        lfoDepth = (s32)lfoPatch->depth - 0x80;
                    }
    
                    // final LFO value
                    if( modSrcLfo )
                        *modSrcLfo = (wave * lfoDepth) / 128;
                } else {
                    // directly write to modulation destinations depending on depths
                    if( modDstPitch ) {
                        s32 depth_p = (s32)lfoPatch->MINIMAL.depth_p - 0x80;
                        *modDstPitch += (wave * depth_p) / 128;
                    }

                    if( modDstPw ) {
                        s32 depth_pw = (s32)lfoPatch->MINIMAL.depth_pw - 0x80;
                        *modDstPw += (wave * depth_pw) / 128;
                    }

                    if( modDstFilter ) {
                        s32 depth_f = (s32)lfoPatch->MINIMAL.depth_f - 0x80;
                        *modDstFilter += (wave * depth_f) / 128;
                    }
                }
            }
#endif
        }
    }

    return overrun;
}

