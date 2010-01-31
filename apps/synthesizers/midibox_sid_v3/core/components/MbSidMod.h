/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Modulation Matrix
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_MOD_H
#define _MB_SID_MOD_H

#include <mios32.h>
#include "MbSidStructs.h"


class MbSidMod
{
public:
    // Constructor
    MbSidMod();

    // Destructor
    ~MbSidMod();

    // MOD init function
    void init(sid_se_mod_patch_t *_modPatch);

    // clears all destinations
    void clearDestinations(void);

    // Modulation Matrix handler
    void tick(void);

    // first MOD Patch entry
    sid_se_mod_patch_t *modPatch;

    // Values of modulation sources
    s16 modSrc[SID_SE_NUM_MOD_SRC];

    // Values of modulation destinations
    s32 modDst[SID_SE_NUM_MOD_DST];

protected:
    // flags modulation transitions
    u8 modTransition;
};

#endif /* _MB_SID_MOD_H */
