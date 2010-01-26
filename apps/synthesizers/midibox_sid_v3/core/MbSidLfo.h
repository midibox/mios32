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

#ifndef _MB_SID_LFO_H
#define _MB_SID_LFO_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidRandomGen.h"
#include "MbSidClock.h"

class MbSidLfo
{
public:
    // Constructor
    MbSidLfo();

    // Destructor
    ~MbSidLfo();

    // LFO init function
    void init(sid_se_lfo_patch_t *_lfoPatch, MbSidClock *_mbSidClockPtr);

    // LFO handler (returns true on overrun)
    bool tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor);

    // requests a restart
    bool restartReq;

    // cross-references
    sid_se_lfo_patch_t *lfoPatch; // LFO-Patch
    MbSidClock *mbSidClockPtr;    // reference to clock generator
    s16    *modSrcLfo;            // reference to SID_SE_MOD_SRC_LFOx
    s32    *modDstLfoDepth;       // reference to SID_SE_MOD_DST_LDx
    s32    *modDstLfoRate;        // reference to SID_SE_MOD_DST_LRx
    s32    *modDstPitch;          // reference to SID_SE_MOD_DST_PITCHx
    s32    *modDstPw;             // reference to SID_SE_MOD_DST_PWx
    s32    *modDstFilter;         // reference to SID_SE_MOD_DST_FILx

    // random generator
    MbSidRandomGen randomGen;

protected:
    u16 ctr;
    u16 delayCtr;
};

#endif /* _MB_SID_LFO_H */
