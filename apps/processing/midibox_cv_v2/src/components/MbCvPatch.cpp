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

#include <string.h>
#include "MbCvPatch.h"
#include "MbCvTables.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvPatch::MbCvPatch()
{
    bankNum = 0;
    patchNum = 0;

    synchedChange = false;
    synchedChangeStep = 0; // step number - 1

    reqChange = false;
    reqChangeAck = false;
    nextBankNum = 0;
    nextPatchNum = 0;

    copyPreset();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvPatch::~MbCvPatch()
{
}


/////////////////////////////////////////////////////////////////////////////
// copies a patch into body
/////////////////////////////////////////////////////////////////////////////
void MbCvPatch::copyToPatch(cv_patch_t *p)
{
    memcpy((u8 *)&body.ALL[0], (u8 *)&p->ALL[0], 512);
}


/////////////////////////////////////////////////////////////////////////////
// copies body into the given RAM location
/////////////////////////////////////////////////////////////////////////////
void MbCvPatch::copyFromPatch(cv_patch_t *p)
{
    memcpy((u8 *)&p->ALL[0], (u8 *)&body.ALL[0], 512);
}


/////////////////////////////////////////////////////////////////////////////
// Preloads a patch
/////////////////////////////////////////////////////////////////////////////
void MbCvPatch::copyPreset(void)
{
    if( sizeof(cv_patch_t) != sizeof(mbCvPatchPresetBassline) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[CV_PATCH_Preset:ERROR] bassline patch size mismatch (%d/%d)\n", sizeof(cv_patch_t), sizeof(mbCvPatchPresetBassline));
#endif
        return; // unexpected error: structure doesn't fit
    }
    memcpy((u8 *)&body.ALL[0], mbCvPatchPresetBassline, sizeof(mbCvPatchPresetBassline));
}


/////////////////////////////////////////////////////////////////////////////
// Returns the patch name as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
void MbCvPatch::nameGet(char *buffer)
{
    int i;

    for(i=0; i<16; ++i)
        buffer[i] = body.name[i] >= 0x20 ? body.name[i] : ' ';
    buffer[i] = 0;
}
