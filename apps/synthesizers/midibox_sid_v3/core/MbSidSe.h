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
#include "MbSidVoiceQueue.h"
#include "MbSidPatch.h"

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
    sid_se_clk_t *mbSidClkPtr;

    // Initialises the structures of a SID sound engine
    void initStructs(void);

    // sound engine update cycle
    void updateSe(void);

    // Trigger matrix
    void triggerNoteOn(sid_se_voice_t *v, u8 no_wt);
    void triggerNoteOff(sid_se_voice_t *v, u8 no_wt);
    void triggerLead(sid_se_trg_t *trg);

    // Modulation matrix
    void calcModulationLead(void);

    // SE handlers
    void seArp(sid_se_voice_t *v);
    bool seGate(sid_se_voice_t *v);
    void sePitch(sid_se_voice_t *v);
    void sePitchDrum(sid_se_voice_t *v);
    void sePw(sid_se_voice_t *v);
    void sePwDrum(sid_se_voice_t *v);
    void seFilterAndVolume(sid_se_filter_t *f);
    void seLfo(sid_se_lfo_t *l);
    void seEnv(sid_se_env_t *e);
    void seEnvLead(sid_se_env_t *e);
    bool seEnvStep(u16 *ctr, u16 target, u8 rate, u8 curve);
    void seWt(sid_se_wt_t *w);
    void seWtDrum(sid_se_wt_t *w);
    void seSeqBassline(sid_se_seq_t *s);
    void seSeqDrum(sid_se_seq_t *s);

    void seNoteOnDrum(u8 drum, u8 velocity);
    void seNoteOffDrum(u8 drum);
    void seAllNotesOff(void);

    void updatePatch(void);


    // has to be implemented outside this class
    void parSetWT(u8 par, u8 wt_value, u8 sidlr, u8 ins);

    // speed factor compared to MBSIDV2
    u8 updateSpeedFactor;

    // Values of modulation sources
    s16 modSrc[SID_SE_NUM_MOD_SRC];

    // Values of modulation destinations
    s32 modDst[SID_SE_NUM_MOD_DST];

    // flags modulation transitions
    u8 modTransition;

    // flags LFO overruns
    u8 lfoOverrun;

    // sound patch
    MbSidPatch mbSidPatch;

    // Sound Engine Structurs
    sid_se_voice_t sid_se_voice[SID_SE_NUM_VOICES];
    sid_se_midi_voice_t sid_se_midi_voice[SID_SE_NUM_MIDI_VOICES];
    sid_se_filter_t sid_se_filter[SID_SE_NUM_FILTERS];
    sid_se_lfo_t sid_se_lfo[SID_SE_NUM_LFO];
    sid_se_env_t sid_se_env[SID_SE_NUM_ENV];
    sid_se_wt_t sid_se_wt[SID_SE_NUM_WT];
    sid_se_seq_t sid_se_seq[SID_SE_NUM_SEQ];

    // voice queue
    MbSidVoiceQueue voiceQueue;
};

#endif /* _MB_SID_SE_H */
