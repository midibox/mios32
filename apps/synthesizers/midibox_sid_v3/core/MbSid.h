/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MB_SID_H
#define _MB_SID_H

#include <mios32.h>
#include "MbSidSe.h"
#include "MbSidKnob.h"
#include "MbSidPatch.h"

class MbSid : public MbSidSe
{
public:
    // Constructor
    MbSid();

    // Destructor
    ~MbSid();

    // test function
    void sendAlive(void);

    // bank access
    s32 bankWrite(MbSidPatch *p);
    s32 patchRead(MbSidPatch *p);
    s32 patchNameGet(u8 bank, u8 patch, char *buffer);

    // MIDI access
    void midiReceive(mios32_midi_package_t midi_package);
    void midiReceiveNote(u8 chn, u8 note, u8 velocity);
    void midiReceiveCC(u8 chn, u8 cc_number, u8 value);
    void midiReceivePitchBend(u8 chn, u16 pitchbend_value_14bit);
    void midiProgramChange(u8 chn, u8 patch);

    // knobs
    MbSidKnob knob[SID_SE_KNOB_NUM];


protected:
    // MIDI helpers
    void SID_MIDI_PushWT(sid_se_midi_voice_t *mv, u8 note);
    void SID_MIDI_PopWT(sid_se_midi_voice_t *mv, u8 note);
    void SID_MIDI_ArpNoteOn(MbSidVoice *v, u8 note, u8 velocity);
    void SID_MIDI_ArpNoteOff(MbSidVoice *v, u8 note);
    void SID_MIDI_NoteOn(MbSidVoice *v, u8 note, u8 velocity, sid_se_v_flags_t v_flags);
    bool SID_MIDI_NoteOff(MbSidVoice *v, u8 note, u8 last_first_note, sid_se_v_flags_t v_flags);
    void SID_MIDI_GateOn(MbSidVoice *v);
    void SID_MIDI_GateOff_SingleVoice(MbSidVoice *v);
    void SID_MIDI_GateOff(MbSidVoice *v, sid_se_midi_voice_t *mv, u8 note);
};

#endif /* _MB_SID_H */
