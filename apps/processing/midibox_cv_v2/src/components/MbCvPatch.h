/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Patch
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_PATCH_H
#define _MB_CV_PATCH_H

#include <mios32.h>
#include "MbCvStructs.h"

class MbCvPatch
{
public:
    // Constructor
    MbCvPatch();

    // Destructor
    ~MbCvPatch();

    // patch functions
    void copyToPatch(cv_patch_t *p);
    void copyFromPatch(cv_patch_t *p);
    void copyPreset(void);
    void nameGet(char *buffer);

    // bank and patch number
    u8 bankNum;
    u8 patchNum;

    // the patch
    cv_patch_t body;
};

#endif /* _MB_CV_PATCH_H */
