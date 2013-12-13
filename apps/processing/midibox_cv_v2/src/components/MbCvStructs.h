/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * This file defines all structures used by MIDIbox CV
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBCV_STRUCTS_H
#define _MBCV_STRUCTS_H

#include <mios32.h>
#include <notestack.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define CV_SE_NUM             8 // CV Channels

#define CV_SE_NOTESTACK_SIZE 10

#define CV_PATCH_SIZE        0x400

#define CV_SCOPE_NUM          4 // 4 scopes



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
    MBCV_MIDI_EVENT_MODE_NOTE        = 0,
    MBCV_MIDI_EVENT_MODE_VELOCITY    = 1,
    MBCV_MIDI_EVENT_MODE_AFTERTOUCH  = 2,
    MBCV_MIDI_EVENT_MODE_CC          = 3,
    MBCV_MIDI_EVENT_MODE_NRPN        = 4,
    MBCV_MIDI_EVENT_MODE_PITCHBENDER = 5,
    MBCV_MIDI_EVENT_MODE_CONST_MIN   = 6,
    MBCV_MIDI_EVENT_MODE_CONST_MID   = 7,
    MBCV_MIDI_EVENT_MODE_CONST_MAX   = 8,
} mbcv_midi_event_mode_t;
#define MBCV_MIDI_EVENT_MODE_NUM       9


////////////////////////////////////////
// Combined Patch Structure
////////////////////////////////////////
typedef union {
    u8 ALL[512];

    struct {
        u8 name[16]; // 16 chars w/o null termination

    };

} cv_patch_t;


#endif /* _MBCV_STRUCTS_H */
