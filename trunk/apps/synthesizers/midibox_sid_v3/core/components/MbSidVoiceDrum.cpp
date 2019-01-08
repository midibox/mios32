/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Voice Handlers for Drum Engine
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
#include "MbSidVoiceDrum.h"
#include "MbSidMidiVoice.h"
#include "MbSidTables.h"
#include "MbSidSe.h"


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoiceDrum::MbSidVoiceDrum()
{
    MbSidVoice::init(0, 0, NULL);
    midiVoicePtr = NULL; // assigned by MbSid!
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidVoiceDrum::~MbSidVoiceDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function with voice and register assignments
// (usually only called once after startup)
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::init(u8 _voiceNum, u8 _physVoiceNum, sid_voice_t *_physSidVoice)
{
    voiceNum = _voiceNum;
    physVoiceNum = _physVoiceNum;
    physSidVoice = _physSidVoice;

    init();
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::init(void)
{
    MbSidVoice::init();

    drumGatelength = 0;
    drumSpeed = 0;
    mbSidDrumPtr = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes a voice depending on drum model, note and velocity
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::init(MbSidDrum *d, u8 note, u8 velocity)
{
    // save pointer to model
    mbSidDrumPtr = d;

    // gatelength
    drumGatelength = d->getGatelength();

    // speed
    drumSpeed = d->getSpeed();

    // initial waveform
    sid_se_voice_waveform_t waveform;
    waveform.ALL = d->getWaveform(0);
    voiceWaveformOff = waveform.VOICE_OFF;
    voiceWaveformSync = waveform.SYNC;
    voiceWaveformRingmod = waveform.RINGMOD;
    voiceWaveform = waveform.WAVEFORM;

    // initial note
    voiceNote = d->getNote(0);

    // initial pulsewidth
    voicePulsewidth = d->getPulsewidth();

    // ADSR
    voiceAttackDecay = d->drumAttackDecay;
    voiceSustainRelease = d->drumSustainRelease;

    // enable osc synchronisation (hard-sync)
    voiceOscPhase = 1;

    // set/clear ACCENT depending on velocity
    voiceAccentActive = velocity >= 64;

#if 0
    DEBUG_MSG("DrumForVoice%d: Wave:%02x Note:%02x PW:%03x, ADSR: %02x%02x, Accent: %d\n",
              voiceNum, waveform.ALL, voiceNote, voicePulsewidth,
              voiceAttackDecay.ALL, voiceSustainRelease.ALL, voiceAccentActive);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Updates the drum voice
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::tick(const u8 &updateSpeedFactor, MbSidSe *se, MbSidWtDrum *w)
{
    // exit if no drum model assigned
    if( mbSidDrumPtr == NULL )
        return;

    // handle drum model with wavetable sequencer
    // wait until initial gate delay has passed (for ABW feature)
    // TK: checking for voiceSetDelayCtr here is probably not a good idea,
    // it leads to jitter. However, we let it like it is for compatibility reasons with V2
    if( !voiceSetDelayCtr )
        w->tick(updateSpeedFactor, this);

    // handle gate and pitch
    if( gate(updateSpeedFactor, se) )
        pitch(updateSpeedFactor, se);

    // control the delayed gate clear request
    if( voiceGateActive && !voiceSetDelayCtr && voiceClrDelayCtr ) {
        int clrDelayCtr = voiceClrDelayCtr + mbSidEnvTable[mbSidDrumPtr->getGatelength()] / updateSpeedFactor;
        if( clrDelayCtr >= 0xffff ) {
            // overrun: clear counter to disable delay
            voiceClrDelayCtr = 0;
            // request gate clear and deactivate voice active state (new note can be played)
            voiceGateSetReq = 0;
            voiceGateClrReq = 1;
            voiceActive = 0;
        } else
            voiceClrDelayCtr = clrDelayCtr;
    }

    // Pulsewidth handler
    pw(updateSpeedFactor, se);

    physSidVoice->waveform = voiceWaveform;
    physSidVoice->sync = voiceWaveformSync;
    physSidVoice->ringmod = voiceWaveformRingmod;

    // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
    if( !voiceSetDelayCtr ) {
        physSidVoice->ad = voiceAttackDecay.ALL;

        // force sustain to maximum if accent flag active
        u8 sr = voiceSustainRelease.ALL;
        if( voiceAccentActive )
            sr |= 0xf0;
        physSidVoice->sr = sr;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch for Drum Instruments
/////////////////////////////////////////////////////////////////////////////
void MbSidVoiceDrum::pitch(const u8 &updateSpeedFactor, MbSidSe *mbSidSe)
{
    // get frequency depending on base note
    u8 frqIx = voiceNote + 21;
    if( frqIx > 127 )
        frqIx = 127;
    s32 targetFrq = mbSidFrqTable[frqIx];

    // increase/decrease target frequency by pitchrange depending on finetune value
    s8 tune = mbSidDrumPtr->drumTuneModifier;
    if( tune ) {
        u8 pitchrange = 24;
        if( tune > 0 ) {
            int dstNote = voiceNote + pitchrange + 21;
            if( dstNote > 127 ) dstNote = 127;
            u16 dstFrq = mbSidFrqTable[dstNote];
            targetFrq += ((dstFrq-targetFrq) * tune) / 128;
            if( targetFrq >= 0xffff ) targetFrq = 0xffff;
        } else {
            int dstNote = voiceNote - pitchrange + 21;
            if( dstNote < 0 ) dstNote = 0;
            u16 dstFrq = mbSidFrqTable[dstNote];
            targetFrq += ((targetFrq-dstFrq) * tune) / 128;
            if( targetFrq < 0 ) targetFrq = 0;
        }
    }

    // copy target frequency into SID registers
    physSidVoice->frq_l = targetFrq & 0xff;
    physSidVoice->frq_h = targetFrq >> 8;
}
