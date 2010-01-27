/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Envelope Generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_ENV_H
#define _MB_SID_ENV_H

#include <mios32.h>
#include "MbSidStructs.h"


typedef enum {
    MBSID_ENV_STATE_IDLE = 0,
    MBSID_ENV_STATE_ATTACK1,
    MBSID_ENV_STATE_ATTACK2,
    MBSID_ENV_STATE_DECAY1,
    MBSID_ENV_STATE_DECAY2,
    MBSID_ENV_STATE_SUSTAIN,
    MBSID_ENV_STATE_RELEASE1,
    MBSID_ENV_STATE_RELEASE2
} mbsid_env_state_t;


class MbSidEnv
{
public:
    // Constructor
    MbSidEnv();

    // Destructor
    ~MbSidEnv();

    // ENV init function
    void init(sid_se_env_patch_t *_envPatch);

    // ENV handler (returns true when sustain phase reached)
    bool tick(const sid_se_engine_t &engine, const u8 &updateSpeedFactor);

    // requests a restart and release phase
    bool restartReq;
    bool releaseReq;

    // set by each 6th MIDI clock (if ENV in SYNC mode)
    bool syncClockReq;

    // requests to use accented parameters
    bool accentReq;

    // ENV Patch
    sid_se_env_patch_t *envPatch;

    // cross-references
    s16    *modSrcEnv;            // reference to SID_SE_MOD_SRC_ENVx
    s32    *modDstPitch;          // reference to SID_SE_MOD_DST_PITCHx
    s32    *modDstPw;             // reference to SID_SE_MOD_DST_PWx
    s32    *modDstFilter;         // reference to SID_SE_MOD_DST_FILx
    u8     *decayA;               // reference to alternative decay value (used on ACCENTed notes)
    u8     *accent;               // reference to accent value (used on ACCENTed notes)


protected:
    bool tickLead(const sid_se_engine_t &engine, const u8 &updateSpeedFactor);
    bool step(const u16 &target, const u8 &rate, const u8 &curve, const u8 &updateSpeedFactor);

    // internal variables
    mbsid_env_state_t envState;
    u16 envCtr;
    u16 envDelayCtr;

};

#endif /* _MB_SID_ENV_H */
