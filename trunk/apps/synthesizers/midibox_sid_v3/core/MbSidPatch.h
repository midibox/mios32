/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Patch
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_PATCH_H
#define _MB_SID_PATCH_H

#include <mios32.h>
#include "MbSidStructs.h"

class MbSidPatch
{
public:
    // Constructor
    MbSidPatch();

    // Destructor
    ~MbSidPatch();

    // patch functions
    void copyToPatch(sid_patch_t *p);
    void copyFromPatch(sid_patch_t *p);
    void copyPreset(sid_se_engine_t engine);
    void nameGet(char *buffer);

    // bank and patch number
    u8 bankNum;
    u8 patchNum;

    // the patch
    sid_patch_t body;
};

#endif /* _MB_SID_PATCH_H */
