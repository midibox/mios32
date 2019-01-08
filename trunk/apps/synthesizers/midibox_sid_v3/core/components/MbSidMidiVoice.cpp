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

#include <string.h>
#include "MbSidMidiVoice.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidMidiVoice::MbSidMidiVoice()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidMidiVoice::~MbSidMidiVoice()
{
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidMidiVoice::init(void)
{
    midivoiceChannel = 0;
    midivoiceSplitLower = 0x00;
    midivoiceSplitUpper = 0x7f;
    midivoiceTranspose = 0x40;
    midivoicePitchbender = 0x80;
    midivoiceLastVoice = 0;

    notestackReset();
    wtstackReset();
}


/////////////////////////////////////////////////////////////////////////////
// returns true if MIDI channel (and optionally split zone) matches
/////////////////////////////////////////////////////////////////////////////
bool MbSidMidiVoice::isAssigned(u8 chn)
{
    return chn == midivoiceChannel;
}

bool MbSidMidiVoice::isAssigned(u8 chn, u8 note)
{
    return chn == midivoiceChannel &&
        note >= midivoiceSplitLower &&
        (!midivoiceSplitUpper || note <= midivoiceSplitUpper);
}


/////////////////////////////////////////////////////////////////////////////
// Notestack Handling
/////////////////////////////////////////////////////////////////////////////
void MbSidMidiVoice::notestackReset(void)
{
    NOTESTACK_Init(&midivoiceNotestack,
                   NOTESTACK_MODE_PUSH_TOP,
                   &midivoiceNotestackItems[0],
                   SID_SE_NOTESTACK_SIZE);
}

void MbSidMidiVoice::notestackPush(u8 note, u8 velocity)
{
    NOTESTACK_Push(&midivoiceNotestack, note, velocity);
}

s32 MbSidMidiVoice::notestackPop(u8 note)
{
    return NOTESTACK_Pop(&midivoiceNotestack, note);
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable Notestack Handling
/////////////////////////////////////////////////////////////////////////////
void MbSidMidiVoice::wtstackReset(void)
{
    for(int i=0; i<4; ++i)
        midivoiceWtStack[i] = 0;
}

void MbSidMidiVoice::wtstackPush(u8 note)
{
    for(int i=0; i<4; ++i) {
        u8 stackNote = midivoiceWtStack[i] & 0x7f;
        bool pushStack = 0;

        if( !stackNote ) { // last entry?
            pushStack = 1;
        } else {
            // ignore if note is already in stack
            if( stackNote == note )
                return;
            // push into stack if note >= current note
            if( stackNote >= note )
                pushStack = 1;
        }

        if( pushStack ) {
            if( i != 3 ) { // max note: no shift required
                for(int j=3; j>i; --j)
                    midivoiceWtStack[j] = midivoiceWtStack[j-1];
            }

            // insert note
            midivoiceWtStack[i] = note;
            return;
        }
    }

    return;
}

void MbSidMidiVoice::wtstackPop(u8 note)
{
    // search for note entry with the same number, erase it and push the entries behind
    for(int i=0; i<4; ++i) {
        u8 stackNote = midivoiceWtStack[i] & 0x7f;

        if( note == stackNote ) {
            // push the entries behind the found entry
            if( i != 3 ) {
                for(int j=i; j<3; ++j)
                    midivoiceWtStack[j] = midivoiceWtStack[j+1];
            }

            // clear last entry
            midivoiceWtStack[3] = 0;
        }
    }
}
