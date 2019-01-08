/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * This file defines some tables used by MIDIbox CV
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_TABLES_H
#define _MBCV_TABLES_H

#include "MbCvStructs.h"

extern const u16 mbCvFrqTable[128];
extern const u16 mbCvEnvTable[256];
extern const u16 mbCvLfoTable[256];
extern const u16 mbCvMclkTable[32];
extern const u16 mbCvSinTable[128];
extern const u8  mbCvPatchPresetBassline[512];

#endif /* _MBCV_TABLES_H */
