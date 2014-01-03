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

#define CV_EXTCLK_NUM         7 // 7 external clock outputs


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
// NRPN Info struct
////////////////////////////////////////
typedef struct {
    u8 cv;
    u8 is_bidir;
    char nameString[21];
    char valueString[21];
    u16 value;
    u16 min;
    u16 max;
} MbCvNrpnInfoT;

////////////////////////////////////////
// Combined Patch Structure
////////////////////////////////////////
typedef union {
    u8 ALL[512];

    struct {
        u8 name[16]; // 16 chars w/o null termination

    };

} cv_patch_t;


#ifdef __cplusplus
// little helper
// a container class which defines an array leads to more robust code when iterating over the array
// will be transfered to a better location later
template <class T, int N> class array
{
protected:
    T element[N];

public:
    array() : size(N) {}

    const int size;

    inline T &operator[](int n)
    {
        if( n < 0 || n >= N ) {
#if 0
            DEBUG_MSG("trouble accessing element %d of array (max: %d)\n", n, N);
#endif
            return element[0];
        }
        return element[n];
    }

    inline T *first()
    {
        return &element[0];
    }

    inline T *last()
    {
        return &element[N-1];
    }

    inline T *next(T *prevElement)
    {
        return (++prevElement >= &element[N]) ? 0 : prevElement;
    }
};
#endif /* __cplusplus */


#endif /* _MBCV_STRUCTS_H */
