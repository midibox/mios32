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

#include <string.h>
#include "MbSidPatch.h"
#include "MbSidTables.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidPatch::MbSidPatch()
{
    bankNum = 0;
    patchNum = 0;

    copyPreset(SID_SE_LEAD);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidPatch::~MbSidPatch()
{
}


/////////////////////////////////////////////////////////////////////////////
// copies a patch into body
/////////////////////////////////////////////////////////////////////////////
void MbSidPatch::copyToPatch(sid_patch_t *p)
{
    memcpy((u8 *)&body.ALL[0], (u8 *)&p->ALL[0], 512);
}


/////////////////////////////////////////////////////////////////////////////
// copies body into the given RAM location
/////////////////////////////////////////////////////////////////////////////
void MbSidPatch::copyFromPatch(sid_patch_t *p)
{
    memcpy((u8 *)&p->ALL[0], (u8 *)&body.ALL[0], 512);
}


/////////////////////////////////////////////////////////////////////////////
// Preloads a patch
/////////////////////////////////////////////////////////////////////////////
void MbSidPatch::copyPreset(sid_se_engine_t engine)
{
    switch( engine ) {
    case SID_SE_LEAD:
        if( sizeof(sid_patch_t) != sizeof(mbSidPatchPresetLead) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            DEBUG_MSG("[SID_PATCH_Preset:ERROR] lead patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(mbSidPatchPresetLead));
#endif
            return; // unexpected error: structure doesn't fit
        }
        memcpy((u8 *)&body.ALL[0], mbSidPatchPresetLead, sizeof(mbSidPatchPresetLead));
        break;

    case SID_SE_BASSLINE:
        if( sizeof(sid_patch_t) != sizeof(mbSidPatchPresetBassline) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            DEBUG_MSG("[SID_PATCH_Preset:ERROR] bassline patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(mbSidPatchPresetBassline));
#endif
            return; // unexpected error: structure doesn't fit
        }
        memcpy((u8 *)&body.ALL[0], mbSidPatchPresetBassline, sizeof(mbSidPatchPresetBassline));
        break;

    case SID_SE_DRUM:
        if( sizeof(sid_patch_t) != sizeof(mbSidPatchPresetDrum) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            DEBUG_MSG("[SID_PATCH_Preset:ERROR] drum patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(mbSidPatchPresetDrum));
#endif
            return; // unexpected error: structure doesn't fit
        }
        memcpy((u8 *)&body.ALL[0], mbSidPatchPresetDrum, sizeof(mbSidPatchPresetDrum));
        break;

    case SID_SE_MULTI:
        if( sizeof(sid_patch_t) != sizeof(mbSidPatchPresetMulti) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
            DEBUG_MSG("[SID_PATCH_Preset:ERROR] multi patch size mismatch (%d/%d)\n", sizeof(sid_patch_t), sizeof(mbSidPatchPresetMulti));
#endif
            return; // unexpected error: structure doesn't fit
        }
        memcpy((u8 *)&body.ALL[0], mbSidPatchPresetMulti, sizeof(mbSidPatchPresetMulti));
        break;

    default:
        return; // invalid engine
    }
}


/////////////////////////////////////////////////////////////////////////////
// Returns the patch name as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
void MbSidPatch::nameGet(char *buffer)
{
    int i;

    for(i=0; i<16; ++i)
        buffer[i] = body.name[i] >= 0x20 ? body.name[i] : ' ';
    buffer[i] = 0;
}
