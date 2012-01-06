/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV MIDI Voice Handlers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_MIDI_VOICE_H
#define _MB_CV_MIDI_VOICE_H

#include <mios32.h>
#include "MbCvStructs.h"
#include "notestack.h"


class MbCvMidiVoice
{
public:
    // Constructor
    MbCvMidiVoice();

    // Destructor
    ~MbCvMidiVoice();

    // voice init function
    void init(void);

    // MIDI port check
    bool isAssignedPort(mios32_midi_port_t port);

    // channel/note range checks
    bool isAssigned(u8 chn);
    bool isAssigned(u8 chn, u8 note);

    // Notestack handling
    void notestackReset(void);
    void notestackPush(u8 note, u8 velocity);
    s32 notestackPop(u8 note);

    // CC/NRPN handling
    void setCC(u8 ccNumber, u8 value);
    void setNRPN(u16 value);

    // parameters
    u16 midivoiceEnabledPorts;
    u8 midivoiceChannel;
    u8 midivoiceSplitLower;
    u8 midivoiceSplitUpper;
    s8 midivoiceTranspose;
    u8 midivoiceLastVoice;
    s16 midivoicePitchbender;
    u8 midivoiceAftertouch;
    u8 midivoiceCCNumber;
    u8 midivoiceCCValue;
    u8 midivoiceNRPNValue;

    // stacks
    notestack_t midivoiceNotestack;
    notestack_item_t midivoiceNotestackItems[CV_SE_NOTESTACK_SIZE];

    // help functions for editor
    void setPortEnabled(const u8& portIx, const bool& enable);
    bool getPortEnabled(const u8& portIx);

};

#endif /* _MB_CV_MIDI_VOICE_H */
