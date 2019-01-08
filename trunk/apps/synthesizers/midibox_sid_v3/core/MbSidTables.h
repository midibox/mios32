/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * This file defines some tables used by MIDIbox SID
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBSID_TABLES_H
#define _MBSID_TABLES_H

#include "MbSidStructs.h"

extern const u16 mbSidFrqTable[128];
extern const u16 mbSidEnvTable[256];
extern const u16 mbSidLfoTable[256];
extern const u16 mbSidLfoTableMclk[11];
extern const u16 mbSidSinTable[128];
extern const sid_drum_model_t mbSidDrumModel[SID_SE_DRUM_MODEL_NUM];
extern const u8 mbSidPatchPresetLead[512];
extern const u8 mbSidPatchPresetBassline[512];
extern const u8 mbSidPatchPresetDrum[512];
extern const u8 mbSidPatchPresetMulti[512];

#endif /* _MBSID_TABLES_H */
