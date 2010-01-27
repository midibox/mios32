/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Sound Engine Toplevel
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_SE_H
#define _MB_SID_SE_H

#include <mios32.h>
#include "MbSidStructs.h"
#include "MbSidPar.h"
#include "MbSidVoiceQueue.h"
#include "MbSidPatch.h"
#include "MbSidClock.h"
#include "MbSidRandomGen.h"
#include "MbSidVoice.h"
#include "MbSidLfo.h"
#include "MbSidEnv.h"
#include "MbSidWt.h"
#include "MbSidMod.h"
#include "MbSidArp.h"

class MbSidSe
{
public:
    // Constructor
    MbSidSe();

    // Destructor
    ~MbSidSe();

    // my sid number
    u8 sidNum;

    // assigned physical SIDs
    u8 physSidL;
    u8 physSidR;

    // reference to SID registers
    sid_regs_t *sidRegLPtr;
    sid_regs_t *sidRegRPtr;


    // reference to clock generator
    MbSidClock *mbSidClockPtr;

    // Initialises the structures of a SID sound engine
    void initStructs(void);

    // sound engine update cycle
    // returns true if SID registers have to be updated
    bool updateSe(void);

    // Trigger matrix
    void triggerNoteOn(MbSidVoice *v, u8 no_wt);
    void triggerNoteOff(MbSidVoice *v, u8 no_wt);
    void triggerLead(sid_se_trg_t *trg);

    // SE handlers
    void seFilterAndVolume(sid_se_filter_t *f);
    void seSeqBassline(sid_se_seq_t *s);
    void seSeqDrum(sid_se_seq_t *s);

    void seNoteOnDrum(u8 drum, u8 velocity);
    void seNoteOffDrum(u8 drum);
    void seAllNotesOff(void);

    void updatePatch(void);


    // speed factor compared to MBSIDV2
    u8 updateSpeedFactor;

    // sound patch
    MbSidPatch mbSidPatch;

    // Sound Engine Structurs
    sid_se_midi_voice_t sid_se_midi_voice[SID_SE_NUM_MIDI_VOICES];
    sid_se_filter_t sid_se_filter[SID_SE_NUM_FILTERS];
    sid_se_seq_t sid_se_seq[SID_SE_NUM_SEQ];

    // Parameter Handler
    MbSidPar mbSidPar;

    // voice queue
    MbSidVoiceQueue voiceQueue;

    // random generator
    MbSidRandomGen randomGen;

    // voices
    MbSidVoice mbSidVoice[SID_SE_NUM_VOICES];

    // modulators
    MbSidLfo mbSidLfo[SID_SE_NUM_LFO];
    MbSidEnv mbSidEnv[SID_SE_NUM_ENV];
    MbSidWt  mbSidWt[SID_SE_NUM_WT];
    MbSidArp mbSidArp[SID_SE_NUM_ARP];

    // modulation matrix
    MbSidMod mbSidMod;
};

#endif /* _MB_SID_SE_H */
