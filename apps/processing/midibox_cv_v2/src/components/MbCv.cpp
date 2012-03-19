/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCv.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCv::MbCv()
{
    u8 cv = 0;
    init(cv, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCv::~MbCv()
{
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the sound engines
/////////////////////////////////////////////////////////////////////////////
void MbCv::init(u8 _cvNum, MbCvClock *_mbCvClockPtr)
{
    mbCvClockPtr = _mbCvClockPtr;
    mbCvVoice.init(_cvNum, &mbCvMidiVoice);
    mbCvMidiVoice.init();
    mbCvMod.init(_cvNum);

    updatePatch(true);
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbCv::tick(const u8 &updateSpeedFactor)
{
    // clock
    if( mbCvClockPtr->eventStart ) {
        mbCvSeqBassline.seqRestartReq = true;
    }

    if( mbCvClockPtr->eventStop ) {
        mbCvSeqBassline.seqStopReq = true;
    }

    // clock arp and sequencers
    if( mbCvClockPtr->eventClock ) {
        mbCvArp.clockReq = true;
        mbCvSeqBassline.seqClockReq = true;
    }

    // LFOs
    {
        MbCvLfo *l = mbCvLfo.first();
        for(int lfo=0; lfo < mbCvLfo.size; ++lfo, ++l) {
            if( mbCvClockPtr->eventClock )
                l->syncClockReq = 1;

            l->lfoAmplitudeModulation =  mbCvMod.takeDstValue(MBCV_MOD_DST_LFO1_A + lfo);
            l->lfoRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_LFO1_R + lfo);

            if( l->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_L1P+lfo];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_LFO1 + lfo] = l->lfoOut;
        }
    }

    // ENVs
    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e) {
            if( mbCvClockPtr->eventClock )
                e->syncClockReq = 1;

            e->envAmplitudeModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV1_A);
            e->envRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV1_R);

            if( e->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_E1S];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] = e->envOut;
        }
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e) {
            if( mbCvClockPtr->eventClock )
                e->syncClockReq = 1;

            e->envAmplitudeModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV2_A);
            e->envRateModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_ENV2_R);

            if( e->tick(updateSpeedFactor) ) {
                // trigger[MBCV_TRG_E2S];
            }

            mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] = e->envOut;
        }
    }

    // Modulation Matrix
    {
        // since this isn't done anywhere else:
        // convert linear frequency of voice1 to 15bit signed value (only positive direction)
        mbCvMod.modSrc[MBCV_MOD_SRC_KEY] = mbCvVoice.voiceLinearFrq >> 1;

        // do ModMatrix calculations
        mbCvMod.tick();

        // additional direct modulation paths
        { // Pitch
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_PITCH];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthPitch) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthPitch) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_PITCH] = mod;
        }

        { // LFO1 Amp
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO1_A];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthLfoAmplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo1Amplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo1Amplitude) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO1_A] = mod;
        }

        { // LFO2 Amp
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO2_A];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthLfoAmplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo2Amplitude) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo2Amplitude) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO2_A] = mod;
        }

        { // LFO1 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO1_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthLfoRate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo1Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo1Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO1_R] = mod;
        }

        { // LFO2 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_LFO2_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthLfoRate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV1] * (s32)mbCvEnv1[0].envDepthLfo2Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_ENV2] * (s32)mbCvEnv2[0].envDepthLfo2Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_LFO2_R] = mod;
        }

        { // ENV1 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_ENV1_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthEnv1Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthEnv1Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_ENV1_R] = mod;
        }

        { // ENV2 Rate
            s32 mod = mbCvMod.modDst[MBCV_MOD_DST_ENV2_R];
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO1] * (s32)mbCvLfo[0].lfoDepthEnv2Rate) / 128;
            mod += ((s32)mbCvMod.modSrc[MBCV_MOD_SRC_LFO2] * (s32)mbCvLfo[1].lfoDepthEnv2Rate) / 128;
            mbCvMod.modDst[MBCV_MOD_DST_ENV2_R] = mod;
        }
    }

    {
        // Voice handling
        MbCvVoice *v = &mbCvVoice; // allows to use multiple voices later

        v->voicePitchModulation = mbCvMod.takeDstValue(MBCV_MOD_DST_PITCH);

        if( mbCvArp.arpEnabled ) {
            mbCvArp.tick(v, this);
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ENVMOD] = mbCvSeqBassline.seqEnvMod << 7; // just pass current value
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ACCENT] = mbCvSeqBassline.seqAccent << 7; // just pass current value (not effective value)
        } else {
            mbCvSeqBassline.tick(v, this);
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ENVMOD] = mbCvSeqBassline.seqEnvMod << 7;
            mbCvMod.modSrc[MBCV_MOD_SRC_SEQ_ACCENT] = mbCvSeqBassline.seqAccentEffective << 7; // effective value is only != 0 when accent flag is set
        }

        if( v->gate(updateSpeedFactor) )
            v->pitch(updateSpeedFactor);
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    if( !mbCvMidiVoice.isAssignedPort(port) )
        return;

    switch( midi_package.event ) {
    case NoteOff:
        midiReceiveNote(midi_package.chn, midi_package.note, 0x00);
        break;

    case NoteOn:
        midiReceiveNote(midi_package.chn, midi_package.note, midi_package.velocity);
        break;

    case PolyPressure:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt2);
        break;

    case CC:
        midiReceiveCC(midi_package.chn, midi_package.cc_number, midi_package.value);
        break;

    case PitchBend: {
        u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
        midiReceivePitchBend(midi_package.chn, pitchbend_value_14bit);
    } break;

    case Aftertouch:
        midiReceiveAftertouch(midi_package.chn, midi_package.evnt1);
        break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    // check if MIDI channel and splitzone matches
    if( !mbCvMidiVoice.isAssigned(chn, note) )
        return; // note filtered

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    // set note on/off
    if( velocity ) {
        mbCvMod.modSrc[MBCV_MOD_SRC_VEL] = velocity << 8;
        noteOn(note, velocity, false);
    } else
        noteOff(note, false);

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveCC(u8 chn, u8 ccNumber, u8 value)
{
    // take over CC (valid CCs will be checked by MIDI voice)
    if( mbCvMidiVoice.isAssigned(chn) ) {
        if( ccNumber == 1 )
            mbCvMod.modSrc[MBCV_MOD_SRC_MDW] = value << 8;

        mbCvMidiVoice.setCC(ccNumber, value);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceivePitchBend(u8 chn, u16 pitchbendValue14bit)
{
    if( mbCvMidiVoice.isAssigned(chn) ) {
        s16 pitchbendValueSigned = (s16)pitchbendValue14bit - 8192;
        mbCvMidiVoice.midivoicePitchbender = pitchbendValueSigned;

        mbCvMod.modSrc[MBCV_MOD_SRC_PBN] = 2*pitchbendValueSigned;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives an Aftertouch Event
/////////////////////////////////////////////////////////////////////////////
void MbCv::midiReceiveAftertouch(u8 chn, u8 value)
{
    if( mbCvMidiVoice.isAssigned(chn) ) {
        mbCvMidiVoice.midivoiceAftertouch = value << 8;
        mbCvMod.modSrc[MBCV_MOD_SRC_ATH] = value;
    }
}


/////////////////////////////////////////////////////////////////////////////
// will set NRPN depending on first 10 bits
// MSBs already decoded in MbCvEnvironment
// returns false if parameter not mapped
/////////////////////////////////////////////////////////////////////////////
bool MbCv::setNRPN(u16 nrpnNumber, u16 value)
{
    u16 section = nrpnNumber & 0x3c0;
    u16 par = nrpnNumber & 0x7f;

    if( section < 0x080 ) {        // Main: 0x000..0x07f
        if( par < 0x10 ) {
            mbCvMidiVoice.setPortEnabled(par, value);
            return true;
        }
        switch( par ) {
        case 0x10: mbCvMidiVoice.midivoiceChannel = value; return true;
        case 0x11: mbCvVoice.voiceEventMode = (mbcv_midi_event_mode_t)value; return true;
        case 0x12: mbCvMidiVoice.midivoiceSplitLower = value; return true;
        case 0x13: mbCvMidiVoice.midivoiceSplitUpper = value; return true;
        case 0x14: mbCvMidiVoice.midivoiceCCNumber = value; return true;

        case 0x18: mbCvVoice.setAoutCurve(value); return true;
        case 0x19: mbCvVoice.setAoutSlewRate(value); return true;
        case 0x1a: mbCvVoice.voiceGateInverted = value; return true;

        case 0x20: mbCvVoice.voiceLegato = value; return true;
        case 0x21: mbCvVoice.voicePoly = value; return true;
        case 0x22: mbCvVoice.voiceSusKey = value; return true;
        case 0x23: mbCvVoice.setPortamentoMode(value); return true;

        case 0x30: mbCvVoice.voicePitchrange = value; return true;
        case 0x31: mbCvVoice.voiceKeytrack = (int)value - 0x80; return true;
        case 0x32: mbCvVoice.voiceTransposeOctave = (int)value - 8; return true;
        case 0x33: mbCvVoice.voiceTransposeSemitone = (int)value - 8; return true;
        case 0x34: mbCvVoice.voiceFinetune = (int)value - 0x80; return true;
        case 0x35: mbCvVoice.voicePortamentoRate = value; return true;
        }
    } else if( section < 0x100 ) { // ARP and SEQ:  0x080..0x0ff

        if( par >= 0x40 && par < (0x40+MBCV_SEQ_BASSLINE_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            mbCvSeqBassline.seqBasslineKey[mbCvSeqBassline.seqPatternNumber][step] = value;
            return true;
        } else if( par >= 0x60 && par < (0x60+MBCV_SEQ_BASSLINE_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            mbCvSeqBassline.seqBasslineArgs[mbCvSeqBassline.seqPatternNumber][step].ALL = value;
            return true;
        }

        switch( par ) {
        case 0x00: mbCvArp.arpEnabled = value; return true;
        case 0x01: mbCvArp.arpDirSet(value); return true;
        case 0x02: mbCvArp.arpHoldMode = value; return true;
        case 0x03: mbCvArp.arpSortedNotes = value; return true;
        case 0x04: mbCvArp.arpSyncMode = value; return true;
        case 0x05: mbCvArp.arpOneshotMode = value; return true;
        case 0x06: mbCvArp.arpConstantCycle = value; return true;
        case 0x07: mbCvArp.arpEasyChordMode = value; return true;
        case 0x10: mbCvArp.arpSpeed = value; return true;
        case 0x11: mbCvArp.arpGatelength = value; return true;

        case 0x20: mbCvSeqBassline.seqEnabled = value; return true;
        case 0x21: mbCvSeqBassline.seqPatternNumber = value; return true;
        case 0x22: mbCvSeqBassline.seqPatternLength = value; return true;
        case 0x23: mbCvSeqBassline.seqResolution = value; return true;
        case 0x24: mbCvSeqBassline.seqGateLength = value; return true;

        case 0x28: mbCvSeqBassline.seqEnvMod = value; return true;
        case 0x29: mbCvSeqBassline.seqAccent = value; return true;
        }
    } else if( section < 0x200 ) { // LFO1: 0x100..0x17f, LFO2: 0x180..0x1ff
        u8 lfo = (section >= 0x180) ? 1 : 0;
        MbCvLfo *l = &mbCvLfo[lfo];

        switch( par ) {
        case 0x00: l->lfoAmplitude = (int)value - 0x80; return true;
        case 0x01: l->lfoRate = value; return true;
        case 0x02: l->lfoDelay = value; return true;
        case 0x03: l->lfoPhase = value; return true;
        case 0x04: l->lfoWaveform = value; return true;

        case 0x10: l->lfoModeClkSync = value; return true;
        case 0x11: l->lfoModeKeySync = value; return true;
        case 0x12: l->lfoModeOneshot = value; return true;
        case 0x13: l->lfoModeFast = value; return true;

        case 0x20: l->lfoDepthPitch = (int)value - 0x80; return true;
        case 0x21: l->lfoDepthLfoAmplitude = (int)value - 0x80; return true;
        case 0x22: l->lfoDepthLfoRate = (int)value - 0x80; return true;
        case 0x23: l->lfoDepthEnv1Rate = (int)value - 0x80; return true;
        case 0x24: l->lfoDepthEnv2Rate = (int)value - 0x80; return true;
        }
    } else if( section < 0x280 ) { // ENV1: 0x200..0x27f
        MbCvEnv *e = &mbCvEnv1[0];

        switch( par ) {
        case 0x00: e->envAmplitude = (int)value - 0x80; return true;
        case 0x01: e->envCurve = (int)value; return true;
        case 0x02: e->envDelay = (int)value; return true;
        case 0x03: e->envAttack = (int)value; return true;
        case 0x04: e->envDecay = (int)value; return true;
        case 0x05: e->envSustain = (int)value; return true;
        case 0x06: e->envRelease = (int)value; return true;

        case 0x10: e->envModeClkSync = (int)value; return true;
        case 0x11: e->envModeKeySync = (int)value; return true;
        case 0x12: e->envModeOneshot = (int)value; return true;
        case 0x13: e->envModeFast = (int)value; return true;

        case 0x20: e->envDepthPitch = (int)value - 0x80; return true;
        case 0x21: e->envDepthLfo1Amplitude = (int)value - 0x80; return true;
        case 0x22: e->envDepthLfo1Rate = (int)value - 0x80; return true;
        case 0x23: e->envDepthLfo2Amplitude = (int)value - 0x80; return true;
        case 0x24: e->envDepthLfo2Rate = (int)value - 0x80; return true;
        }
    } else if( section < 0x300 ) { // ENV2: 0x280..0x2ff
        MbCvEnvMulti *e = &mbCvEnv2[0];

        if( par >= 0x40 && par < (0x40+MBCV_ENV_MULTI_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            e->envLevel[step] = value;
            return true;
        } else if( par >= 0x60 && par < (0x60+MBCV_ENV_MULTI_NUM_STEPS) ) {
            // spare
            return false;
        }

        switch( par ) {
        case 0x00: e->envAmplitude = (int)value - 0x80; return true;
        case 0x01: e->envCurve = (int)value; return true;
        case 0x02: e->envOffset = (int)value - 0x80; return true;
        case 0x03: e->envRate = (int)value; return true;

        case 0x08: e->envLoopAttack = (int)value; return true;
        case 0x09: e->envSustainStep = (int)value; return true;
        case 0x0a: e->envLoopRelease = (int)value; return true;
        case 0x0b: e->envLastStep = (int)value; return true;

        case 0x10: e->envModeClkSync = (int)value; return true;
        case 0x11: e->envModeKeySync = (int)value; return true;
        case 0x12: e->envModeOneshot = (int)value; return true;
        case 0x13: e->envModeFast = (int)value; return true;

        case 0x20: e->envDepthPitch = (int)value - 0x80; return true;
        case 0x21: e->envDepthLfo1Amplitude = (int)value - 0x80; return true;
        case 0x22: e->envDepthLfo1Rate = (int)value - 0x80; return true;
        case 0x23: e->envDepthLfo2Amplitude = (int)value - 0x80; return true;
        case 0x24: e->envDepthLfo2Rate = (int)value - 0x80; return true;
        }
    } else if( section < 0x380 ) { // MOD1: 0x300..0x30f, ... MOD4: 0x330..0x33f
        u8 mod = par >> 4;

        if( mod >= MBCV_NUM_MOD )
            return false; // just to ensure...

        MbCvMod::ModPatchT *mp = (MbCvMod::ModPatchT *)&mbCvMod.modPatch[mod];
        switch( par & 0x0f ) {
        case 0x00: mp->depth = (int)value - 0x80; return true;
        case 0x01: mp->src1 = value; return true;
        case 0x02: mp->src1_chn = value; return true;
        case 0x03: mp->src2 = value; return true;
        case 0x04: mp->src2_chn = value; return true;
        case 0x05: mp->op &= 0xc0; mp->op |= (value & 0x3f); return true;
        case 0x06: mp->dst1 = value; return true;
        case 0x07: mp->op &= ~(1 << 6); mp->op |= ((value&1) << 6); return true;
        case 0x08: mp->dst2 = value; return true;
        case 0x09: mp->op &= ~(1 << 7); mp->op |= ((value&1) << 7); return true;
        }
    }

    return false; // parameter not mapped
}

/////////////////////////////////////////////////////////////////////////////
// returns NRPN value depending on first 10 bits
// MSBs already decoded in MbCvEnvironment
// returns false if parameter not mapped
/////////////////////////////////////////////////////////////////////////////
bool MbCv::getNRPN(u16 nrpnNumber, u16 *value)
{
    u16 section = nrpnNumber & 0x3c0;
    u16 par = nrpnNumber & 0x7f;

    if( section < 0x080 ) {        // Main: 0x000..0x07f
        if( par < 0x10 ) {
            *value = mbCvMidiVoice.getPortEnabled(par & 0x0f);
            return true;
        }
        switch( par ) {
        case 0x10: *value = mbCvMidiVoice.midivoiceChannel; return true;
        case 0x11: *value = (u16)mbCvVoice.voiceEventMode; return true;
        case 0x12: *value = mbCvMidiVoice.midivoiceSplitLower; return true;
        case 0x13: *value = mbCvMidiVoice.midivoiceSplitUpper; return true;
        case 0x14: *value = mbCvMidiVoice.midivoiceCCNumber; return true;

        case 0x18: *value = mbCvVoice.getAoutCurve(); return true;
        case 0x19: *value = mbCvVoice.getAoutSlewRate(); return true;
        case 0x1a: *value = mbCvVoice.voiceGateInverted; return true;

        case 0x20: *value = mbCvVoice.voiceLegato; return true;
        case 0x21: *value = mbCvVoice.voicePoly; return true;
        case 0x22: *value = mbCvVoice.voiceSusKey; return true;
        case 0x23: *value = mbCvVoice.getPortamentoMode(); return true;

        case 0x30: *value = mbCvVoice.voicePitchrange; return true;
        case 0x31: *value = (int)mbCvVoice.voiceKeytrack + 0x80; return true;
        case 0x32: *value = (int)mbCvVoice.voiceTransposeOctave + 8; return true;
        case 0x33: *value = (int)mbCvVoice.voiceTransposeSemitone + 8; return true;
        case 0x34: *value = (int)mbCvVoice.voiceFinetune + 0x80; return true;
        case 0x35: *value = mbCvVoice.voicePortamentoRate; return true;
        }
    } else if( section < 0x100 ) { // ARP and SEQ:  0x080..0x0ff

        if( par >= 0x40 && par < (0x40+MBCV_SEQ_BASSLINE_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            *value = mbCvSeqBassline.seqBasslineKey[mbCvSeqBassline.seqPatternNumber][step];
            return true;
        } else if( par >= 0x60 && par < (0x60+MBCV_SEQ_BASSLINE_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            *value = mbCvSeqBassline.seqBasslineArgs[mbCvSeqBassline.seqPatternNumber][step].ALL;
            return true;
        }

        switch( par ) {
        case 0x00: *value = mbCvArp.arpEnabled; return true;
        case 0x01: *value = mbCvArp.arpDirGet(); return true;
        case 0x02: *value = mbCvArp.arpHoldMode; return true;
        case 0x03: *value = mbCvArp.arpSortedNotes; return true;
        case 0x04: *value = mbCvArp.arpSyncMode; return true;
        case 0x05: *value = mbCvArp.arpOneshotMode; return true;
        case 0x06: *value = mbCvArp.arpConstantCycle; return true;
        case 0x07: *value = mbCvArp.arpEasyChordMode; return true;
        case 0x10: *value = mbCvArp.arpSpeed; return true;
        case 0x11: *value = mbCvArp.arpGatelength; return true;

        case 0x20: *value = mbCvSeqBassline.seqEnabled; return true;
        case 0x21: *value = mbCvSeqBassline.seqPatternNumber; return true;
        case 0x22: *value = mbCvSeqBassline.seqPatternLength; return true;
        case 0x23: *value = mbCvSeqBassline.seqResolution; return true;
        case 0x24: *value = mbCvSeqBassline.seqGateLength; return true;

        case 0x28: *value = mbCvSeqBassline.seqEnvMod; return true;
        case 0x29: *value = mbCvSeqBassline.seqAccent; return true;
        }
    } else if( section < 0x200 ) { // LFO1: 0x100..0x17f, LFO2: 0x180..0x1ff
        u8 lfo = (section >= 0x180) ? 1 : 0;
        MbCvLfo *l = &mbCvLfo[lfo];

        switch( par ) {
        case 0x00: *value = (int)l->lfoAmplitude + 0x80; return true;
        case 0x01: *value = (int)l->lfoRate; return true;
        case 0x02: *value = (int)l->lfoDelay; return true;
        case 0x03: *value = (int)l->lfoPhase; return true;
        case 0x04: *value = (int)l->lfoWaveform; return true;

        case 0x10: *value = (int)l->lfoModeClkSync; return true;
        case 0x11: *value = (int)l->lfoModeKeySync; return true;
        case 0x12: *value = (int)l->lfoModeOneshot; return true;
        case 0x13: *value = (int)l->lfoModeFast; return true;

        case 0x20: *value = (int)l->lfoDepthPitch + 0x80; return true;
        case 0x21: *value = (int)l->lfoDepthLfoAmplitude + 0x80; return true;
        case 0x22: *value = (int)l->lfoDepthLfoRate + 0x80; return true;
        case 0x23: *value = (int)l->lfoDepthEnv1Rate + 0x80; return true;
        case 0x24: *value = (int)l->lfoDepthEnv2Rate + 0x80; return true;
        }
    } else if( section < 0x280 ) { // ENV1: 0x200..0x27f
        MbCvEnv *e = &mbCvEnv1[0];

        switch( par ) {
        case 0x00: *value = (int)e->envAmplitude + 0x80; return true;
        case 0x01: *value = (int)e->envCurve; return true;
        case 0x02: *value = (int)e->envDelay; return true;
        case 0x03: *value = (int)e->envAttack; return true;
        case 0x04: *value = (int)e->envDecay; return true;
        case 0x05: *value = (int)e->envSustain; return true;
        case 0x06: *value = (int)e->envRelease; return true;

        case 0x10: *value = (int)e->envModeClkSync; return true;
        case 0x11: *value = (int)e->envModeKeySync; return true;
        case 0x12: *value = (int)e->envModeOneshot; return true;
        case 0x13: *value = (int)e->envModeFast; return true;

        case 0x20: *value = (int)e->envDepthPitch + 0x80; return true;
        case 0x21: *value = (int)e->envDepthLfo1Amplitude + 0x80; return true;
        case 0x22: *value = (int)e->envDepthLfo1Rate + 0x80; return true;
        case 0x23: *value = (int)e->envDepthLfo2Amplitude + 0x80; return true;
        case 0x24: *value = (int)e->envDepthLfo2Rate + 0x80; return true;
        }
    } else if( section < 0x300 ) { // ENV2: 0x280..0x2ff
        MbCvEnvMulti *e = &mbCvEnv2[0];

        if( par >= 0x40 && par < (0x40+MBCV_ENV_MULTI_NUM_STEPS) ) {
            u8 step = par & 0x1f;
            *value = e->envLevel[step];
            return true;
        } else if( par >= 0x60 && par < (0x60+MBCV_ENV_MULTI_NUM_STEPS) ) {
            // spare
            return false;
        }

        switch( par ) {
        case 0x00: *value = (int)e->envAmplitude + 0x80; return true;
        case 0x01: *value = e->envCurve; return true;
        case 0x02: *value = (int)e->envOffset + 0x80; return true;
        case 0x03: *value = e->envRate; return true;

        case 0x08: *value = e->envLoopAttack; return true;
        case 0x09: *value = e->envSustainStep; return true;
        case 0x0a: *value = e->envLoopRelease; return true;
        case 0x0b: *value = e->envLastStep; return true;

        case 0x10: *value = e->envModeClkSync; return true;
        case 0x11: *value = e->envModeKeySync; return true;
        case 0x12: *value = e->envModeOneshot; return true;
        case 0x13: *value = e->envModeFast; return true;

        case 0x20: *value = (int)e->envDepthPitch + 0x80; return true;
        case 0x21: *value = (int)e->envDepthLfo1Amplitude + 0x80; return true;
        case 0x22: *value = (int)e->envDepthLfo1Rate + 0x80; return true;
        case 0x23: *value = (int)e->envDepthLfo2Amplitude + 0x80; return true;
        case 0x24: *value = (int)e->envDepthLfo2Rate + 0x80; return true;
        }
    } else if( section < 0x380 ) { // MOD1: 0x300..0x31f, ... MOD4: 0x360..0x37f
        u8 mod = par >> 4;

        if( mod >= MBCV_NUM_MOD )
            return false; // just to ensure...
        
        MbCvMod::ModPatchT *mp = (MbCvMod::ModPatchT *)&mbCvMod.modPatch[mod];
        switch( par & 0x0f ) {
        case 0x00: *value = mp->depth + 0x80; return true;
        case 0x01: *value = mp->src1; return true;
        case 0x02: *value = mp->src1_chn; return true;
        case 0x03: *value = mp->src2; return true;
        case 0x04: *value = mp->src2_chn; return true;
        case 0x05: *value = mp->op & 0x3f; return true;
        case 0x06: *value = mp->dst1; return true;
        case 0x07: *value = (mp->op & (1 << 6)) ? 1 : 0; return true;
        case 0x08: *value = mp->dst2; return true;
        case 0x09: *value = (mp->op & (1 << 7)) ? 1 : 0; return true;
        }
    }

    return false; // parameter not mapped
}



/////////////////////////////////////////////////////////////////////////////
// Should be called whenver the patch has been changed
/////////////////////////////////////////////////////////////////////////////
void MbCv::updatePatch(bool forceEngineInit)
{
    // disable interrupts to ensure atomic change
    MIOS32_IRQ_Disable();

    mbCvMidiVoice.init();
    mbCvVoice.init();
    mbCvArp.init();
    mbCvSeqBassline.init();
    
    for(int lfo=0; lfo<mbCvLfo.size; ++lfo)
        mbCvLfo[lfo].init();

    for(int env=0; env<mbCvEnv1.size; ++env)
        mbCvEnv1[env].init();

    // enable interrupts again
    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to take over a received patch
// returns false if initialisation failed
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexSetPatch(cv_patch_t *p)
{
    mbCvPatch.copyToPatch(p);
    updatePatch(false);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to return the current patch
// returns false if patch not available
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexGetPatch(cv_patch_t *p)
{
    // TODO: bank read
    mbCvPatch.copyFromPatch(p);
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Callback from MbCvSysEx to set a dedicated SysEx parameter
// returns false on invalid access
/////////////////////////////////////////////////////////////////////////////
bool MbCv::sysexSetParameter(u16 addr, u8 data)
{
    // exit if address range not valid (just to ensure)
    if( addr >= sizeof(cv_patch_t) )
        return false;

    // change value in patch
    mbCvPatch.body.ALL[addr] = data;

    return false; // TODO
}


/////////////////////////////////////////////////////////////////////////////
// Play a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteOn(u8 note, u8 velocity, bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    // branch depending on normal/arp mode
    if( mbCvArp.arpEnabled ) {
        mbCvArp.noteOn(v, note, velocity);
    } else {
        // push note into stack
        if( !bypassNotestack )
            mv->notestackPush(note, velocity);

        // switch off gate if not in legato or sequencer mode
        if( !v->voiceLegato && !v->voiceSequencerOnly )
            v->gateOff(note);

        // call Note On Handler
        if( v->noteOn(note, velocity) )
            triggerNoteOn(v);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Disable a note independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteOff(u8 note, bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;
    MbCvMidiVoice *mv = (MbCvMidiVoice *)v->midiVoicePtr;

    // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
    // if not in arp mode: sustain only relevant if only one active note in stack

    // branch depending on normal/arp mode
    if( mbCvArp.arpEnabled ) {
        mbCvArp.noteOff(v, note);
    } else {
        if( bypassNotestack ) {
            v->noteOff(note, 0);
            triggerNoteOff(v);
        } else {
            u8 lastFirstNote = mv->midivoiceNotestack.note_items[0].note;
            // pop note from stack
            if( mv->notestackPop(note) > 0 ) {
                // call Note Off Handler
                if( v->noteOff(note, lastFirstNote) > 0 ) { // retrigger requested?
                    if( v->noteOn(mv->midivoiceNotestack.note_items[0].note, mv->midivoiceNotestack.note_items[0].tag) ) // new note
                        triggerNoteOn(v);
                } else {
                    triggerNoteOff(v);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All notes off - independent from channel
/////////////////////////////////////////////////////////////////////////////
void MbCv::noteAllOff(bool bypassNotestack)
{
    MbCvVoice *v = &mbCvVoice;

    v->voiceActive = 0;
    v->voiceGateClrReq = 1;
    v->voiceGateSetReq = 0;
    triggerNoteOff(v);

    if( !bypassNotestack ) {
        v->midiVoicePtr->notestackReset();
    }
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbCv::triggerNoteOn(MbCvVoice *v)
{
    mbCvVoice.voiceNoteRestartReq = 1;

    MbCvLfo *l = mbCvLfo.first();
    for(int lfo=0; lfo < mbCvLfo.size; ++lfo, ++l)
        if( l->lfoModeKeySync )
            l->restartReq = 1;

    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e)
            if( e->envModeKeySync )
                e->restartReq = 1;
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e)
            if( e->envModeKeySync )
                e->restartReq = 1;
    }
}


void MbCv::triggerNoteOff(MbCvVoice *v)
{
    {
        MbCvEnv *e = mbCvEnv1.first();
        for(int env=0; env < mbCvEnv1.size; ++env, ++e)
            if( e->envModeKeySync )
                e->releaseReq = 1;
    }

    {
        MbCvEnvMulti *e = mbCvEnv2.first();
        for(int env=0; env < mbCvEnv2.size; ++env, ++e)
            if( e->envModeKeySync )
                e->releaseReq = 1;
    }
}
