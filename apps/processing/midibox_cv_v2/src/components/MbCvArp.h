/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Arpeggiator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_CV_ARP_H
#define _MB_CV_ARP_H

#include <mios32.h>
#include "MbCvStructs.h"
#include "MbCvRandomGen.h"
#include "MbCvVoice.h"

class MbCv; // forward declaration, see also http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.11

class MbCvArp
{
public:
    // Constructor
    MbCvArp();

    // Destructor
    ~MbCvArp();

    // arp init function
    void init(void);

    // arpeggiator handler
    void tick(MbCvVoice *v, MbCv *mbCv);

    // Notestack handling
    void noteOn(MbCvVoice *v, u8 note, u8 velocity);
    void noteOff(MbCvVoice *v, u8 note);


    // input parameters
    bool arpEnabled;
    bool arpDown;
    bool arpUpAndDown;
    bool arpPingPong;
    bool arpRandomNotes;
    bool arpSortedNotes;
    bool arpHoldMode;
    bool arpSyncMode;
    bool arpConstantCycle;
    bool arpEasyChordMode;
    bool arpOneshotMode;
    u8 arpSpeed;
    u8 arpGatelength;
    u8 arpOctaveRange;

    // requests a restart and next clock (divided by Arp)
    bool restartReq;
    bool clockReq;

    // random generator
    MbCvRandomGen randomGen;

protected:
    // internal variables
    bool arpActive;
    bool arpUp;
    bool arpHoldModeSaved;

    u8 arpDivCtr;
    u8 arpGatelengthCtr;
    u8 arpNoteCtr;
    u8 arpOctaveCtr;

};

#endif /* _MB_CV_ARP_H */
