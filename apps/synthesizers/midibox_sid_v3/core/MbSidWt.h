/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Wavetable Sequencer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_WT_H
#define _MB_SID_WT_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidPar.h"


class MbSidWt
{
public:
    // Constructor
    MbSidWt();

    // Destructor
    ~MbSidWt();

    // WT init function
    void init(sid_se_wt_patch_t *_wtPatch, u8 _wtNum, MbSidPar *_mbSidParPtr);

    // Wavetable handler (returns true on loop)
    bool tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor);
    bool tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor,
              sid_drum_model_t *drumModel, u8 &voiceNote, u8 &voiceWaveform);

    // reference to parameter handler
    MbSidPar *mbSidParPtr;
    
    // my number (for multi engine)
    u8 wtNum;

    // requests a restart and next clock (divided by WT)
    bool restartReq;
    bool clockReq;

    // speed and parameter for drums
    u8 wtDrumSpeed;
    u8 wtDrumPar;

    // position
    u8 wtPos;

    // Wavetable Patch
    sid_se_wt_patch_t *wtPatch;

    // cross-references
    s16    *modSrcWt;             // reference to SID_SE_MOD_SRC_WTx
    s32    *modDstWt;             // reference to SID_SE_MOD_DST_WTx
    u8     *wtMemory;             // reference to wavetable memory

protected:
    // internal variables
    u16 wtDivCtr; // should be >8bit for Drum mode (WTs clocked independent from BPM generator)
};

#endif /* _MB_SID_WT_H */
