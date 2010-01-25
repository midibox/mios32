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
    init(SID_SE_LEAD, 1, NULL, NULL);
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
void MbSidLfo::init(sid_se_engine_t _engine, u8 _updateSpeedFactor, sid_se_lfo_patch_t *_lfoPatch, MbSidClock *_mbSidClockPtr)
{
    engine = _engine;
    updateSpeedFactor = _updateSpeedFactor;
    lfoPatch = _lfoPatch;
    mbSidClockPtr = _mbSidClockPtr;

    // clear flags
    restartReq = 0;

    // clear references
    modSrcLfo = 0;
    modDstLfoDepth = 0;
    modDstLfoRate = 0;
    modDstPitch = 0;
    modDstPw = 0;
    modDstFilter = 0;
}


/////////////////////////////////////////////////////////////////////////////
// LFO handler
/////////////////////////////////////////////////////////////////////////////
bool MbSidLfo::tick(void)
{
    bool overrun = false; // will be the return value

    if( !lfoPatch ) // exit if no patch reference initialized
        return false;

    sid_se_lfo_mode_t lfoMode;
    lfoMode.ALL = lfoPatch->mode;

    // LFO restart requested?
    if( restartReq ) {
        restartReq = 0;

        // reset counter (take phase into account)
        ctr = lfoPatch->phase << 8;

        // check if LFO should be delayed - set delay counter to 0x0001 in this case
        delayCtr = lfoPatch->delay ? 1 : 0;
    }

    // set wave register to initial value and skip LFO if not enabled
    if( !lfoMode.ENABLE ) {
        if( modSrcLfo )
            *modSrcLfo = 0;
    } else {
        if( delayCtr ) {
            int new_delayCtr = delayCtr + (mbSidEnvTable[lfoPatch->delay] / updateSpeedFactor);
            if( new_delayCtr > 0xffff )
                delayCtr = 0; // delay passed
            else
                delayCtr = new_delayCtr; // delay not passed
        }

        if( !delayCtr ) { // delay passed?
            u8 lfoStalled = 0;

            // in oneshot mode: check if counter already reached 0xffff
            if( lfoMode.ONESHOT && ctr >= 0xffff )
                lfoStalled = 1;

            // if clock sync enabled: only increment on each 16th clock event
            if( lfoMode.CLKSYNC && (!mbSidClockPtr->event.CLK || mbSidClockPtr->clkCtr6 != 0) )
                lfoStalled = 1;

            if( !lfoStalled ) {
                // increment 16bit counter by given rate
                // the rate can be modulated
                s32 lfo_rate;
                if( modDstLfoRate ) {
                    lfo_rate = lfoPatch->rate + (*modDstLfoRate / 256);
                    if( lfo_rate > 255 ) lfo_rate = 255; else if( lfo_rate < 0 ) lfo_rate = 0;
                } else {
                    lfo_rate = lfoPatch->rate;
                }

                // if LFO synched via clock, replace 245-255 by MIDI clock optimized incrementers
                u16 inc;
                if( lfoMode.CLKSYNC && lfo_rate >= 245 )
                    inc = mbSidLfoTableMclk[lfo_rate-245]; // / updateSpeedFactor;
                else
                    inc = mbSidLfoTable[lfo_rate] / updateSpeedFactor;

                // add to counter and check for overrun
                s32 new_ctr = ctr + inc;
                if( new_ctr > 0xffff ) {
                    overrun = true;

                    if( lfoMode.ONESHOT )
                        new_ctr = 0xffff; // stop at end position
                }

                ctr = (u16)new_ctr;
            }


            // map counter to waveform
            u8 lfo_waveform_skipped = 0;
            s16 wave;
            switch( lfoMode.WAVEFORM ) {
            case 0: { // Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = ctr >> 7;
                if( ctr & (1 << 14) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                wave = mbSidSinTable[ptr];
                if( ctr & (1 << 15) )
                    wave = -wave;
            } break;  

            case 1: { // Triangle
                // similar to sine, but linear waveform
                wave = (ctr & 0x3fff) << 1;
                if( ctr & (1 << 14) )
                    wave = 0x7fff - wave;
                if( ctr & (1 << 15) )
                    wave = -wave;
            } break;  

            case 2: { // Saw
                wave = ctr - 0x8000;
            } break;  

            case 3: { // Pulse
                wave = (ctr < 0x8000) ? -0x8000 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            case 4: { // Random
                // only on LFO overrun
                if( overrun )
                    wave = randomGen.value(0x0000, 0xffff);
                else
                    lfo_waveform_skipped = 1;
            } break;  

            case 5: { // Positive Sine
                // sine table contains a quarter of a sine
                // we have to negate/mirror it depending on the mapped counter value
                u8 ptr = ctr >> 8;
                if( ctr & (1 << 15) )
                    ptr ^= 0x7f;
                ptr &= 0x7f;
                wave = mbSidSinTable[ptr];
            } break;  

            case 6: { // Positive Triangle
                // similar to sine, but linear waveform
                wave = (ctr & 0x7fff);
                if( ctr & (1 << 15) )
                    wave = 0x7fff - wave;
            } break;  

            case 7: { // Positive Saw
                wave = ctr >> 1;
            } break;  

            case 8: { // Positive Pulse
                wave = (ctr < 0x8000) ? 0 : 0x7fff; // due to historical reasons it's inverted
            } break;  

            default: // take saw as default
                wave = ctr - 0x8000;
            }

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
                    s32 depth_p = (s32)lfoPatch->MINIMAL.depth_p - 0x80;
                    *modDstPitch += (wave * depth_p) / 128;

                    s32 depth_pw = (s32)lfoPatch->MINIMAL.depth_pw - 0x80;
                    *modDstPw += (wave * depth_pw) / 128;

                    s32 depth_f = (s32)lfoPatch->MINIMAL.depth_f - 0x80;
                    *modDstFilter += (wave * depth_f) / 128;
                }
            }
        }
    }

    return overrun;
}

