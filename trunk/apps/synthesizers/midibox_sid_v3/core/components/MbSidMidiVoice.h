/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID MIDI Voice Handlers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_MIDI_VOICE_H
#define _MB_SID_MIDI_VOICE_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "notestack.h"


class MbSidMidiVoice
{
public:
    // Constructor
    MbSidMidiVoice();

    // Destructor
    ~MbSidMidiVoice();

    // voice init function
    void init(void);

    // channel/note range checks
    bool isAssigned(u8 chn);
    bool isAssigned(u8 chn, u8 note);

    // WT stack handling
    void wtstackReset(void);
    void wtstackPush(u8 note);
    void wtstackPop(u8 note);

    // Notestack handling
    void notestackReset(void);
    void notestackPush(u8 note, u8 velocity);
    s32 notestackPop(u8 note);

    // parameters
    u8 midivoiceChannel;
    u8 midivoiceSplitLower;
    u8 midivoiceSplitUpper;
    u8 midivoiceTranspose;
    u8 midivoiceLastVoice;
    u8 midivoicePitchbender;

    // stacks
    notestack_t midivoiceNotestack;
    notestack_item_t midivoiceNotestackItems[SID_SE_NOTESTACK_SIZE];
    u8 midivoiceWtStack[4];
};

#endif /* _MB_SID_MIDI_VOICE_H */
